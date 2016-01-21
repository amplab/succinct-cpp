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

#include "logger.h"
#include "succinct_shard.h"
#include "configuration_manager.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class QueryServiceHandler : virtual public QueryServiceIf {
 public:
  QueryServiceHandler(const std::string& filename) {
    succinct_shard_ = NULL;
    filename_ = filename;
    is_init_ = false;
    num_keys_ = 0;
    conf_ = ConfigurationManager();
    logger_ = Logger();
  }

  int32_t Initialize(int32_t id) {
    logger_.Info("Initializing data structures.");
    logger_.Info("Construct is set to %d.", construct_);

    // Read configuration parameters
    SuccinctMode mode = conf_.GetInt("LOAD_MODE");
    int32_t sa_sampling_rate = conf_.GetInt("SA_SAMPLING_RATE");
    int32_t isa_sampling_rate = conf_.GetInt("ISA_SAMPLING_RATE");
    int32_t npa_sampling_rate = conf_.GetInt("NPA_SAMPLING_RATE");
    SamplingScheme sa_sampling_scheme = SamplingSchemeFromOption(conf_.GetInt("SA_SAMPLING_SCHEME"));
    SamplingScheme isa_sampling_scheme = SamplingSchemeFromOption(conf_.GetInt("ISA_SAMPLING_SCHEME"));
    NPA::NPAEncodingScheme npa_scheme = EncodingSchemeFromOption(conf_.GetInt("NPA_SCHEME"));

    succinct_shard_ = new SuccinctShard(id, filename_, mode,
        sa_sampling_rate, isa_sampling_rate, npa_sampling_rate, sa_sampling_scheme,
        isa_sampling_scheme, npa_scheme);
    if (construct_) {
      logger_.Info("Serializing data structures for file %s.", filename_.c_str());
      succinct_shard_->Serialize();
    } else {
      logger_.Info("Read data structures from file %s.", filename_.c_str());
    }
    logger_.Info("Initialized Succinct data structures with original size = %llu", succinct_shard_->GetOriginalSize());
    logger_.Info("Initialization successful.");
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

  void RegexSearch(std::set<int64_t> &_return, const std::string &query) {
    std::set<std::pair<size_t, size_t>> results;
    succinct_shard_->RegexSearch(results, query, regex_opt_);
    for (auto res : results) {
      _return.insert((int64_t) res.first);
    }
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

  SuccinctShard *succinct_shard_;
  const std::string filename_;
  int64_t num_keys_;
  bool is_init_;
  ConfigurationManager conf_;
  Logger logger_;
};

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-i <id>] file\n", exec);
}

int main(int argc, char **argv) {

  if (argc != 2) {
    print_usage(argv[0]);
    return -1;
  }

  std::string filename = std::string(argv[1]);
  shared_ptr<QueryServiceHandler> handler(new QueryServiceHandler(filename));
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
