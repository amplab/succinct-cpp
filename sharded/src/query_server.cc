#include "QueryService.h"
#include "succinct_constants.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include <cstdio>
#include <fstream>
#include <cstdint>

#include "succinct_file.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class QueryServiceHandler : virtual public QueryServiceIf {
 public:
  QueryServiceHandler(std::string filename, int mode, uint32_t sa_sampling_rate,
                      uint32_t isa_sampling_rate,
                      SamplingScheme sampling_scheme,
                      NPA::NPAEncodingScheme npa_scheme) {
    succinct_file_ = NULL;
    mode_ = mode;
    filename_ = filename;
    is_init_ = false;
    sa_sampling_rate_ = sa_sampling_rate;
    isa_sampling_rate_ = isa_sampling_rate;
    sampling_scheme_ = sampling_scheme;
    npa_scheme_ = npa_scheme;
  }

  int32_t Initialize(int32_t id) {
    fprintf(stderr, "Received INIT signal, initializing data structures...\n");

    if (!is_init_) {
      SuccinctMode mode;
      switch (mode_) {
        case 0: {
          mode = SuccinctMode::CONSTRUCT_IN_MEMORY;
          break;
        }
        case 1: {
          mode = SuccinctMode::LOAD_IN_MEMORY;
          break;
        }
        case 2: {
          mode = SuccinctMode::LOAD_MEMORY_MAPPED;
          break;
        }
      }

      succinct_file_ = new SuccinctFile(filename_, mode, sa_sampling_rate_,
                                        isa_sampling_rate_, 128,
                                        sampling_scheme_, sampling_scheme_,
                                        npa_scheme_);
      if (mode_ == 0) {
        fprintf(stderr, "Serializing data structures for file %s\n",
                filename_.c_str());
        succinct_file_->Serialize(filename_ + ".succinct");
      } else {
        fprintf(stderr, "Read data structures from file %s\n",
                filename_.c_str());
      }
      fprintf(stderr, "Initialized shard with original size = %llu\n",
              succinct_file_->GetOriginalSize());
      is_init_ = true;
      fprintf(stderr, "Waiting for queries...\n");
      return 0;
    } else {
      // Attempting to initialize already initialized shard.
      fprintf(stderr,
              "Failed to initialize shard: shard already initialized.\n");
      return -1;
    }
  }

  void Regex(std::set<int64_t> &_return, const std::string &query) {
    std::set<std::pair<size_t, size_t>> results;
    succinct_file_->RegexSearch(results, query);
    for (auto res : results) {
      _return.insert((int64_t) res.first);
    }
  }

  void Extract(std::string& _return, const int64_t offset,
               const int64_t length) {
    succinct_file_->Extract(_return, offset, length);
  }

  int64_t Count(const std::string& query) {
    return succinct_file_->Count(query);
  }

  void Search(std::vector<int64_t>& _return, const std::string& query) {
    succinct_file_->Search(_return, query);
  }

  int64_t GetShardSize() {
    return succinct_file_->GetOriginalSize();
  }

 private:
  SuccinctFile *succinct_file_;
  int mode_;
  std::string filename_;
  bool is_init_;
  uint32_t sa_sampling_rate_;
  uint32_t isa_sampling_rate_;
  SamplingScheme sampling_scheme_;
  NPA::NPAEncodingScheme npa_scheme_;
};

SamplingScheme SamplingSchemeFromOption(int opt) {
  switch (opt) {
    case 0:
      return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
    case 1:
      return SamplingScheme::FLAT_SAMPLE_BY_VALUE;
    case 2:
      return SamplingScheme::LAYERED_SAMPLE_BY_INDEX;
    case 3:
      return SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX;
    default:
      return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
  }
}

NPA::NPAEncodingScheme EncodingSchemeFromOption(int opt) {
  switch (opt) {
    case 0:
      return NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED;
    case 1:
      return NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
    case 2:
      return NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED;
    default:
      return NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
  }
}

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-m mode] [-p port] [-s sa_sampling_rate_] [-i isa_sampling_rate_] [-x sampling_scheme_] [-r npa_encoding_scheme] [-o] [file]\n",
      exec);
}

int main(int argc, char **argv) {

  if (argc < 2 || argc > 14) {
    print_usage(argv[0]);
    return -1;
  }

  fprintf(stderr, "Command line: ");
  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "%s ", argv[i]);
  }
  fprintf(stderr, "\n");

  int c;
  uint32_t mode = 0, port = SERVER_PORT, sa_sampling_rate = 32,
      isa_sampling_rate = 32;
  SamplingScheme scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX;
  NPA::NPAEncodingScheme npa_scheme =
      NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;

  while ((c = getopt(argc, argv, "m:p:s:i:r:x:")) != -1) {
    switch (c) {
      case 'm':
        mode = atoi(optarg);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 's':
        sa_sampling_rate = atoi(optarg);
        break;
      case 'i':
        isa_sampling_rate = atoi(optarg);
        break;
      case 'x':
        scheme = SamplingSchemeFromOption(atoi(optarg));
        break;
      case 'r':
        npa_scheme = EncodingSchemeFromOption(atoi(optarg));
        break;
      default:
        fprintf(stderr, "Error parsing command line arguments.\n");
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string filename = std::string(argv[optind]);

  shared_ptr<QueryServiceHandler> handler(
      new QueryServiceHandler(filename, mode, sa_sampling_rate,
                              isa_sampling_rate, scheme, npa_scheme));
  shared_ptr<TProcessor> processor(new QueryServiceProcessor(handler));

  try {
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(processor, server_transport, transport_factory,
                           protocol_factory);
    server.serve();
  } catch (std::exception& e) {
    fprintf(stderr, "Exception at query_server.cc:main(): %s\n", e.what());
  }
  return 0;
}
