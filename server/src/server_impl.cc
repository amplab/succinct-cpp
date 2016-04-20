#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <future>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "heartbeat_manager.h"
#include "Server.h"
#include "succinct_shard_async.h"
#include "constants.h"
#include "configuration_manager.h"
#include "logger.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace succinct {

class ServerImpl : virtual public ServerIf {
 public:
  typedef std::future<std::set<int64_t>> future_search_result;

  ServerImpl(const uint32_t host_id, SuccinctShardAsync **shards,
             ConfigurationManager& conf, Logger& logger,
             HeartbeatManager& hb_manager)
      : host_id_(host_id),
        shards_(shards),
        conf_(conf),
        logger_(logger),
        hb_manager_(hb_manager) {

    num_shards_ = conf_.GetInt("SHARDS_PER_SERVER");
    ReadHostNames(conf_.Get("HOSTS_LIST"));

    logger_.Info("Initialized server.");
  }

  void Get(std::string& _return, const int64_t key) {
    uint32_t shard_id = key / Constants::kShardKeysMax;
    uint32_t host_id = shard_id % hostnames_.size();
    uint32_t local_shard_idx = shard_id / hostnames_.size();

    if (host_id == host_id_) {
      GetLocal(_return, local_shard_idx, key % Constants::kShardKeysMax);
    } else {
      qservers_.at(host_id).GetLocal(_return, local_shard_idx,
                                     key % Constants::kShardKeysMax);
    }
  }

  void GetLocal(std::string& _return, const int32_t shard_idx,
                const int64_t key) {
    shards_[shard_idx]->Get(_return, key);
  }

  void Search(std::set<int64_t> & _return, const std::string& query) {

    std::vector<future_search_result> results;
    for (size_t i = 0; i < hostnames_.size(); i++) {
      if (i == host_id_) {
        for (size_t j = 0; j < num_shards_; j++) {
          results.push_back(shards_[j]->SearchAsync(query));
        }
      } else {
        qservers_.at(i).send_SearchLocal(query);
      }
    }

    for (size_t i = 0; i < hostnames_.size(); i++) {
      if (i == host_id_) {
        for (auto &result : results) {
          std::set<int64_t> keys = result.get();
          _return.insert(keys.begin(), keys.end());
        }
      } else {
        std::set<int64_t> keys;
        qservers_.at(i).recv_SearchLocal(keys);
        _return.insert(keys.begin(), keys.end());
      }
    }
  }

  void SearchLocal(std::set<int64_t> & _return, const std::string& query) {
    std::vector<future_search_result> results;
    for (size_t j = 0; j < num_shards_; j++) {
      results.push_back(shards_[j]->SearchAsync(query));
    }

    for (auto &result : results) {
      std::set<int64_t> keys = result.get();
      _return.insert(keys.begin(), keys.end());
    }
  }

  int32_t ConnectToServers() {
    // Get handler port
    uint16_t handler_port = conf_.GetInt("SERVER_PORT");

    // Create connections to all handlers
    for (size_t i = 0; i < hostnames_.size(); i++) {
      try {
        if (i != host_id_) {
          logger_.Info("Connecting to %s:%d...", hostnames_[i].c_str(),
                       handler_port);

          boost::shared_ptr<TSocket> socket(
              new TSocket(hostnames_[i], handler_port));
          boost::shared_ptr<TTransport> transport(
              new TBufferedTransport(socket));
          boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

          ServerClient client(protocol);
          transport->open();

          logger_.Info("Connection successful.");

          qserver_transports_.push_back(transport);
          qservers_.insert(std::pair<int, ServerClient>(i, client));
        }
      } catch (std::exception& e) {
        logger_.Error("Could not connect to handler at %s:%d. Reason: %s ",
                      hostnames_[i].c_str(), handler_port, e.what());
        return -1;
      }
    }

    logger_.Info("All connections successful.");
    logger_.Info("Current #connections: %zu.", qservers_.size());

    return 0;
  }

  int32_t DisconnectFromServers() {
    // Get handler port
    uint16_t handler_port = conf_.GetInt("SERVER_PORT");

    // Destroy connections to all handlers
    for (size_t i = 0; i < hostnames_.size(); i++) {
      try {
        if (i != host_id_) {
          logger_.Info("Disconnecting from handler at %s:%d...",
                       hostnames_[i].c_str(), handler_port);

          qserver_transports_[i]->close();

          logger_.Info("Disconnected.");
        }
      } catch (std::exception& e) {
        logger_.Info(
            "Could not close connection to handler at %s:%d. Reason: %s",
            hostnames_[i].c_str(), handler_port, e.what());
        return -1;
      }
    }

    qserver_transports_.clear();
    qservers_.clear();

    return 0;
  }

 private:
  void ReadHostNames(const std::string& hostsfile) {
    std::ifstream hosts(hostsfile);
    if (!hosts.is_open()) {
      logger_.Error("Could not open hosts file at %s.", hostsfile.c_str());
      exit(1);
    }
    std::string host;
    while (std::getline(hosts, host, '\n')) {
      hostnames_.push_back(host);
      logger_.Info("Read host: %s.", host.c_str());
    }
  }

  const uint32_t host_id_;
  std::vector<std::string> hostnames_;

  uint32_t num_shards_;
  SuccinctShardAsync** shards_;

  ConfigurationManager conf_;
  Logger logger_;
  HeartbeatManager hb_manager_;

  std::map<int, ServerClient> qservers_;
  std::vector<boost::shared_ptr<TTransport>> qserver_transports_;
};

class ServerImplProcessorFactory : public TProcessorFactory {
 public:
  ServerImplProcessorFactory(uint32_t host_id, SuccinctShardAsync **shards,
                             ConfigurationManager& conf, Logger& logger,
                             HeartbeatManager& hb_manager)
      : host_id_(host_id),
        shards_(shards),
        conf_(conf),
        logger_(logger),
        hb_manager_(hb_manager) {
    logger_.Info("***Creating server processor factory***");
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    logger_.Info("===Creating new processor for server===");
    boost::shared_ptr<ServerImpl> handler(
        new ServerImpl(host_id_, shards_, conf_, logger_, hb_manager_));
    boost::shared_ptr<TProcessor> handlerProcessor(
        new ServerProcessor(handler));
    return handlerProcessor;
  }

 private:
  uint32_t host_id_;
  SuccinctShardAsync **shards_;
  ConfigurationManager conf_;
  Logger logger_;
  HeartbeatManager hb_manager_;
};

}

SuccinctShardAsync** InitShards(const int32_t host_id, Logger& logger,
                                ConfigurationManager& conf) {
  logger.Info("Initializing local shards for host %d", host_id);

  // Read configuration parameters
  SuccinctMode mode = static_cast<SuccinctMode>(conf.GetInt("LOAD_MODE"));
  int32_t sa_sampling_rate = conf.GetInt("SA_SAMPLING_RATE");
  int32_t isa_sampling_rate = conf.GetInt("ISA_SAMPLING_RATE");
  int32_t npa_sampling_rate = conf.GetInt("NPA_SAMPLING_RATE");
  SamplingScheme sa_sampling_scheme = static_cast<SamplingScheme>(conf.GetInt(
      "SA_SAMPLING_SCHEME"));
  SamplingScheme isa_sampling_scheme = static_cast<SamplingScheme>(conf.GetInt(
      "ISA_SAMPLING_SCHEME"));
  NPA::NPAEncodingScheme npa_scheme = static_cast<NPA::NPAEncodingScheme>(conf
      .GetInt("NPA_SCHEME"));
  int32_t shards_per_server = conf.GetInt("SHARDS_PER_SERVER");
  std::string data_path = conf.Get("DATA_PATH");

  SuccinctShardAsync** shards = new SuccinctShardAsync*[shards_per_server];

  logger.Info("Load mode is set to %d.", mode);
  logger.Info("Data path is set to %s.", data_path.c_str());

  for (uint32_t i = 0; i < shards_per_server; i++) {
    std::string filename = data_path + "/data_" + std::to_string(i);

    // Initialize data structures
    int32_t shard_id = host_id * shards_per_server + i;
    logger.Info("Creating shard id = %d", shard_id);
    shards[i] = new SuccinctShardAsync(shard_id, filename, mode,
                                       sa_sampling_rate, isa_sampling_rate,
                                       npa_sampling_rate, sa_sampling_scheme,
                                       isa_sampling_scheme, npa_scheme);

    if (mode == SuccinctMode::CONSTRUCT_IN_MEMORY
        || mode == SuccinctMode::CONSTRUCT_MEMORY_MAPPED) {
      logger.Info("Serializing data structures for file %s.", filename.c_str());
      shards[i]->Serialize();
    } else {
      logger.Info("Read data structures from file %s.", filename.c_str());
    }
    logger.Info(
        "Initialized Succinct data structures with original size = %llu",
        shards[i]->GetOriginalSize());
    logger.Info("Initialization successful for shard id = %d.", shard_id);
  }

  return shards;
}

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [hostid]", exec);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    print_usage(argv[0]);
    return -1;
  }

  int32_t host_id = atoi(argv[1]);
  ConfigurationManager conf;
  const std::string handler_log_output = conf.Get("SERVER_LOG_PATH_PREFIX")
      + "_" + std::to_string(host_id) + ".log";

  FILE *desc = fopen(handler_log_output.c_str(), "a");
  if (desc == NULL) {
    fprintf(stderr, "Could not obtain descriptor for %s, logging to stderr.\n",
            handler_log_output.c_str());
    desc = stderr;
  }
  Logger logger(static_cast<Logger::Level>(conf.GetInt("SERVER_LOG_LEVEL")),
                desc);

  SuccinctShardAsync** shards = InitShards(host_id, logger, conf);

  succinct::HeartbeatManager hb_manager(host_id, conf, logger);

  int port = conf.GetInt("SERVER_PORT");
  try {
    shared_ptr<succinct::ServerImplProcessorFactory> handlerFactory(
        new succinct::ServerImplProcessorFactory(host_id, shards, conf, logger,
                                                 hb_manager));
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(handlerFactory, server_transport, transport_factory,
                           protocol_factory);

    logger.Info("Listening for connections on port %d...", port);
    server.serve();
  } catch (std::exception& e) {
    logger.Error("Could not create server listening on port %d. Reason: %s",
                 port, e.what());
  }

  return 0;
}

