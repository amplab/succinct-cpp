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

#include "succinct_shard.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class QueryServiceHandler : virtual public QueryServiceIf {
 public:
  QueryServiceHandler(std::string filename, bool construct,
                      uint32_t sa_sampling_rate, uint32_t isa_sampling_rate,
                      SamplingScheme sampling_scheme,
                      NPA::NPAEncodingScheme npa_scheme,
                      bool regex_opt = true) {
    succinct_shard_ = NULL;
    construct_ = construct;
    filename_ = filename;
    is_init_ = false;
    num_keys_ = 0;
    sa_sampling_rate_ = sa_sampling_rate;
    isa_sampling_rate_ = isa_sampling_rate;
    regex_opt_ = regex_opt;
    sampling_scheme_ = sampling_scheme;
    npa_scheme_ = npa_scheme;
  }

  int32_t Initialize(int32_t id) {
    fprintf(stderr, "Received INIT signal, initializing data structures...\n");
    fprintf(stderr, "Construct is set to %d\n", construct_);

    succinct_shard_ = new SuccinctShard(
        id,
        filename_,
        construct_ ?
            SuccinctMode::CONSTRUCT_IN_MEMORY : SuccinctMode::LOAD_IN_MEMORY,
        sa_sampling_rate_, isa_sampling_rate_, 128, sampling_scheme_,
        sampling_scheme_, npa_scheme_);
    if (construct_) {
      fprintf(stderr, "Serializing data structures for file %s\n",
              filename_.c_str());
      succinct_shard_->Serialize();
    } else {
      fprintf(stderr, "Read data structures from file %s\n", filename_.c_str());
    }
    fprintf(stderr, "Succinct data structures with original size = %llu\n",
            succinct_shard_->GetOriginalSize());
    fprintf(stderr, "Done initializing...\n");
    fprintf(stderr, "Waiting for queries...\n");
    is_init_ = true;
    num_keys_ = succinct_shard_->GetNumKeys();
    return 0;
  }

  void Get(std::string& _return, const int64_t key) {
    succinct_shard_->Get(_return, key);
  }

  void Access(std::string& _return, const int64_t key, const int32_t offset,
              const int32_t len) {
    succinct_shard_->Access(_return, key, offset, len);
  }

  void Search(std::set<int64_t>& _return, const std::string& query) {
    succinct_shard_->Search(_return, query);
  }

  void Regex(std::set<int64_t> &_return, const std::string &query) {
    std::set<std::pair<size_t, size_t>> results;
    succinct_shard_->RegexSearch(results, query, regex_opt_);
    for (auto res : results) {
      _return.insert((int64_t) res.first);
    }
  }

  void FlatExtract(std::string& _return, const int64_t offset,
                    const int64_t length) {
    succinct_shard_->FlatExtract(_return, offset, length);
  }

  int64_t FlatCount(const std::string& query) {
    return succinct_shard_->FlatCount(query);
  }

  void FlatSearch(std::vector<int64_t>& _return, const std::string& query) {
    succinct_shard_->FlatSearch(_return, query);
  }

  int64_t Count(const std::string& query) {
    return succinct_shard_->Count(query);
  }

  int32_t GetNumKeys() {
    return succinct_shard_->GetNumKeys();
  }

  int64_t GetShardSize() {
    return succinct_shard_->GetOriginalSize();
  }

 private:
  SuccinctShard *succinct_shard_;
  bool construct_;
  std::string filename_;
  bool is_init_;
  uint32_t num_keys_;
  uint32_t sa_sampling_rate_;
  uint32_t isa_sampling_rate_;
  SamplingScheme sampling_scheme_;
  NPA::NPAEncodingScheme npa_scheme_;
  bool regex_opt_;
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

  if (argc < 2 || argc > 15) {
    print_usage(argv[0]);
    return -1;
  }

  fprintf(stderr, "Command line: ");
  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "%s ", argv[i]);
  }
  fprintf(stderr, "\n");

  int c;
  uint32_t mode = 0, port = QUERY_SERVER_PORT, sa_sampling_rate = 32,
      isa_sampling_rate = 32;
  bool regex_opt = false;
  SamplingScheme scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX;
  NPA::NPAEncodingScheme npa_scheme =
      NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;

  while ((c = getopt(argc, argv, "m:p:s:i:r:x:o")) != -1) {
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
      case 'o':
        regex_opt = true;
        break;
      default:
        mode = 0;
        port = QUERY_SERVER_PORT;
        isa_sampling_rate = 32;
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string filename = std::string(argv[optind]);
  bool construct = (mode == 0) ? true : false;

  shared_ptr<QueryServiceHandler> handler(
      new QueryServiceHandler(filename, construct, sa_sampling_rate,
                              isa_sampling_rate, scheme, npa_scheme,
                              regex_opt));
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
    fprintf(stderr, "Exception at SuccinctServer:main(): %s\n", e.what());
  }
  return 0;
}
