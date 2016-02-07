#include <ctime>
#include "Server.h"

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
#include "heartbeat_types.h"
#include "configuration_manager.h"
#include "logger.h"
#include "server_heartbeat_manager.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace succinct {

class ServerImpl : virtual public ServerIf {
 public:
  ServerImpl(int32_t id, const std::string& filename,
             ConfigurationManager &conf, Logger &logger,
             ServerHeartbeatManager &hb_manager)
      : filename_(filename),
        id_(id),
        conf_(conf),
        logger_(logger),
        hb_manager_(hb_manager) {

    succinct_shard_ = NULL;
    is_init_ = false;
    num_keys_ = 0;

  }

  int32_t Initialize() {
    // Don't initialize if already initialized
    if (is_init_) {
      logger_.Warn("Data structures already initialized.");
      return -1;
    }

    logger_.Info("Initializing data structures...");

    // Read configuration parameters
    SuccinctMode mode = static_cast<SuccinctMode>(conf_.GetInt("LOAD_MODE"));
    int32_t sa_sampling_rate = conf_.GetInt("SA_SAMPLING_RATE");
    int32_t isa_sampling_rate = conf_.GetInt("ISA_SAMPLING_RATE");
    int32_t npa_sampling_rate = conf_.GetInt("NPA_SAMPLING_RATE");
    SamplingScheme sa_sampling_scheme =
        static_cast<SamplingScheme>(conf_.GetInt("SA_SAMPLING_SCHEME"));
    SamplingScheme isa_sampling_scheme = static_cast<SamplingScheme>(conf_
        .GetInt("ISA_SAMPLING_SCHEME"));
    NPA::NPAEncodingScheme npa_scheme =
        static_cast<NPA::NPAEncodingScheme>(conf_.GetInt("NPA_SCHEME"));

    logger_.Info("Load mode is set to %d.", mode);

    // Initialize data structures
    succinct_shard_ = new SuccinctShard(id_, filename_, mode, sa_sampling_rate,
                                        isa_sampling_rate, npa_sampling_rate,
                                        sa_sampling_scheme, isa_sampling_scheme,
                                        npa_scheme);
    if (mode == SuccinctMode::CONSTRUCT_IN_MEMORY
        || mode == SuccinctMode::CONSTRUCT_MEMORY_MAPPED) {
      logger_.Info("Serializing data structures for file %s.",
                   filename_.c_str());
      succinct_shard_->Serialize();
    } else {
      logger_.Info("Read data structures from file %s.", filename_.c_str());
    }
    logger_.Info(
        "Initialized Succinct data structures with original size = %llu",
        succinct_shard_->GetOriginalSize());
    logger_.Info("Initialization successful.");
    is_init_ = true;
    num_keys_ = succinct_shard_->GetNumKeys();

    return 0;
  }

  void Get(std::string& _return, const int64_t key) {
    succinct_shard_->Get(_return, key);
  }

  void Search(std::set<int64_t>& _return, const std::string& query) {
    succinct_shard_->Search(_return, query);
  }

  void RegexSearch(std::set<int64_t> &_return, const std::string &query) {
    std::set<std::pair<size_t, size_t>> results;
    succinct_shard_->RegexSearch(results, query, true);
    for (auto res : results) {
      _return.insert((int64_t) res.first);
    }
  }

 private:
  const int32_t id_;
  const std::string filename_;
  ConfigurationManager conf_;
  ServerHeartbeatManager hb_manager_;
  Logger logger_;

  SuccinctShard *succinct_shard_;
  int64_t num_keys_;
  bool is_init_;

};

}

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [id] [file]\n", exec);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    print_usage(argv[0]);
    return -1;
  }

  int32_t id = atoi(argv[1]);
  std::string filename = std::string(argv[2]);
  ConfigurationManager conf;
  std::string server_log_output = conf.Get("SERVER_LOG_PATH_PREFIX") + "_"
      + std::to_string(id) + ".log";
  FILE *desc = fopen(server_log_output.c_str(), "a");
  if (desc == NULL) {
    fprintf(stderr, "Could not obtain descriptor for %s, logging to stderr.\n",
            server_log_output.c_str());
    desc = stderr;
  }

  Logger logger(static_cast<Logger::Level>(conf.GetInt("SERVER_LOG_LEVEL")),
                desc);

  succinct::ServerHeartbeatManager hb_manager(id, conf, logger);

  shared_ptr<succinct::ServerImpl> handler(
      new succinct::ServerImpl(id, filename, conf, logger, hb_manager));
  shared_ptr<TProcessor> processor(new succinct::ServerProcessor(handler));

  try {
    shared_ptr<TServerSocket> server_transport(
        new TServerSocket(conf.GetInt("SERVER_PORT") + id));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(processor, server_transport, transport_factory,
                           protocol_factory);

    logger.Info("Starting server on port %d...",
                conf.GetInt("SERVER_PORT") + id);
    server.serve();
  } catch (std::exception& e) {
    logger.Error("Could not create server listening on port %d. Reason: %s",
                 conf.GetInt("SERVER_PORT") + id, e.what());
  }
  return 0;
}
