#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "Handler.h"
#include "Server.h"
#include "constants.h"
#include "configuration_manager.h"
#include "logger.h"
#include "handler_heartbeat_manager.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace succinct {

class HandlerImpl : virtual public HandlerIf {
 public:
  HandlerImpl(const uint32_t host_id, ConfigurationManager& conf,
              Logger& logger, HandlerHeartbeatManager& hb_manager)
      : host_id_(host_id),
        conf_(conf),
        logger_(logger),
        hb_manager_(hb_manager) {

    num_shards_ = conf_.GetInt("SHARDS_PER_SERVER");
    ReadHostNames(conf_.Get("HOSTS_LIST"));

    logger_.Info("Initialized handler.");
  }

  int32_t StartLocalServers() {
    // Get base server port
    uint16_t server_base_port = conf_.GetInt("SERVER_PORT");

    // Connect to query servers and start initialization
    for (uint32_t i = 0; i < num_shards_; i++) {
      int port = server_base_port + i;

      boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
      boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
      boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

      ServerClient client(protocol);
      transport->open();

      logger_.Info("Connection to local server %u successful.", i);
      logger_.Info("Sending initialization signal...");

      client.send_Initialize();
      qservers_.push_back(client);
      qserver_transports_.push_back(transport);
    }

    for (auto client : qservers_) {
      int32_t status = client.recv_Initialize();
      if (status) {
        logger_.Error("Initialization failed, status = %d!", status);
        return status;
      }
    }

    logger_.Info("Initialization successful.");

    // hb_manager_.Start();

    return 0;
  }

  void Get(std::string& _return, const int64_t key) {
    uint32_t shard_id = key / Constants::kShardKeysMax;
    uint32_t host_id = shard_id % hostnames_.size();
    uint32_t qserver_id = shard_id / hostnames_.size();

    if (host_id == host_id_) {
      GetLocal(_return, qserver_id, key % Constants::kShardKeysMax);
    } else {
      qhandlers_.at(host_id).GetLocal(_return, qserver_id,
                                      key % Constants::kShardKeysMax);
    }
  }

  void GetLocal(std::string& _return, const int32_t qserver_id,
                const int64_t key) {
    qservers_.at(qserver_id).Get(_return, key);
  }

  void Search(std::set<int64_t> & _return, const std::string& query) {

    for (size_t i = 0; i < hostnames_.size(); i++) {
      if (i == host_id_) {
        for (size_t j = 0; j < qservers_.size(); j++) {
          qservers_[j].send_Search(query);
        }
      } else {
        qhandlers_.at(i).send_SearchLocal(query);
      }
    }

    for (size_t i = 0; i < hostnames_.size(); i++) {
      if (i == host_id_) {
        for (size_t j = 0; j < qservers_.size(); j++) {
          std::set<int64_t> keys;
          qservers_[j].recv_Search(keys);
          _return.insert(keys.begin(), keys.end());
        }
      } else {
        std::set<int64_t> keys;
        qhandlers_.at(i).recv_SearchLocal(keys);
        _return.insert(keys.begin(), keys.end());
      }
    }
  }

  void SearchLocal(std::set<int64_t> & _return, const std::string& query) {
    for (size_t j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_Search(query);
    }

    for (size_t j = 0; j < qservers_.size(); j++) {
      std::set<int64_t> keys;
      qservers_[j].recv_Search(keys);
      _return.insert(keys.begin(), keys.end());
    }
  }

//  void RegexSearchLocal(std::set<int64_t> & _return, const std::string& query) {
//    for (int j = 0; j < qservers_.size(); j++) {
//      qservers_[j].send_RegexSearch(query);
//    }
//
//    for (int j = 0; j < qservers_.size(); j++) {
//      std::set<int64_t> results;
//      qservers_[j].recv_RegexSearch(results);
//      _return.insert(results.begin(), results.end());
//    }
//  }
//
//  void RegexSearch(std::set<int64_t> & _return, const std::string& query) {
//
//    for (int i = 0; i < hostnames_.size(); i++) {
//      if (i == local_host_id_) {
//        for (int j = 0; j < qservers_.size(); j++) {
//          qservers_[j].send_RegexSearch(query);
//        }
//      } else {
//        qhandlers_.at(i).send_RegexSearchLocal(query);
//      }
//    }
//
//    for (int i = 0; i < hostnames_.size(); i++) {
//      if (i == local_host_id_) {
//        for (int j = 0; j < qservers_.size(); j++) {
//          std::set<int64_t> results;
//          qservers_[j].recv_RegexSearch(results);
//          _return.insert(results.begin(), results.end());
//        }
//      } else {
//        std::set<int64_t> results;
//        qhandlers_.at(i).recv_RegexSearch(results);
//        _return.insert(results.begin(), results.end());
//      }
//    }
//  }

  int32_t ConnectToHandlers() {
    // Get handler port
    uint16_t handler_port = conf_.GetInt("HANDLER_PORT");

    // Create connections to all handlers
    for (size_t i = 0; i < hostnames_.size(); i++) {
      try {
        if (i == host_id_) {
          logger_.Info("Connecting to local servers...");
          ConnectToLocalServers();
        } else {
          logger_.Info("Connecting to %s:%d...", hostnames_[i].c_str(),
                       handler_port);

          boost::shared_ptr<TSocket> socket(
              new TSocket(hostnames_[i], handler_port));
          boost::shared_ptr<TTransport> transport(
              new TBufferedTransport(socket));
          boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

          HandlerClient client(protocol);
          transport->open();

          logger_.Info("Connection successful.");

          qhandler_transports_.push_back(transport);
          qhandlers_.insert(std::pair<int, HandlerClient>(i, client));

          logger_.Info("Asking %s to connect to local servers...",
                       hostnames_[i].c_str());
          client.ConnectToLocalServers();
        }
      } catch (std::exception& e) {
        logger_.Error("Could not connect to handler at %s:%d. Reason: %s ",
                      hostnames_[i].c_str(), handler_port, e.what());
        return -1;
      }
    }

    logger_.Info("All connections successful.");
    logger_.Info("Current #connections: %zu.", qhandlers_.size());

    return 0;
  }

  int32_t ConnectToLocalServers() {
    // Get base server port
    uint16_t server_base_port = conf_.GetInt("SERVER_PORT");

    for (size_t i = 0; i < num_shards_; i++) {
      try {
        logger_.Info("Connecting to local server %d...", i);

        boost::shared_ptr<TSocket> socket(
            new TSocket("localhost", server_base_port + i));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

        ServerClient qsclient(protocol);
        transport->open();

        qservers_.push_back(qsclient);
        qserver_transports_.push_back(transport);

        logger_.Info("Connection successful.");
      } catch (std::exception& e) {
        logger_.Error("Could not connect to server %d. Reason: %s", i,
                      e.what());
        return -1;
      }
    }

    logger_.Info("All connections successful.");
    logger_.Info("Current #local connections: %zu.", qservers_.size());

    return 0;
  }

  int32_t DisconnectFromHandlers() {
    // Get handler port
    uint16_t handler_port = conf_.GetInt("HANDLER_PORT");

    // Destroy connections to all handlers
    for (size_t i = 0; i < hostnames_.size(); i++) {
      try {
        if (i == host_id_) {
          logger_.Info("Killing local server connections...");
          DisconnectFromLocalServers();
        } else {
          logger_.Info(
              "Asking handler at %s:%d to kill local server connections...",
              hostnames_[i].c_str(), handler_port);
          qhandlers_.at(i).DisconnectFromLocalServers();

          logger_.Info("Disconnecting from handler at %s:%d...",
                       hostnames_[i].c_str(), handler_port);
          qhandler_transports_[i]->close();

          logger_.Info("Disconnected.");
        }
      } catch (std::exception& e) {
        logger_.Info(
            "Could not close connection to handler at %s:%d. Reaon: %s",
            hostnames_[i].c_str(), handler_port, e.what());
        return -1;
      }
    }

    qhandler_transports_.clear();
    qhandlers_.clear();

    return 0;
  }

  int32_t DisconnectFromLocalServers() {
    for (size_t i = 0; i < qservers_.size(); i++) {
      try {
        logger_.Info("Disconnecting from local server %d", i);
        qserver_transports_[i]->close();

        logger_.Info("Disconnected.");
      } catch (std::exception& e) {
        logger_.Error(
            "Could not close local connection to server %d. Reason: %s", i,
            e.what());
        return -1;
      }
    }

    qserver_transports_.clear();
    qhandlers_.clear();

    return 0;
  }

  void Ping(const HeartBeat& _return) {
    logger_.Info("Received heartbeat from server %d with timestamp %ld.",
                 _return.sender_id, _return.timestamp);
    // TODO: Update server metadata based on heartbeat message
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
  uint32_t num_shards_;
  std::vector<std::string> hostnames_;

  ConfigurationManager conf_;
  Logger logger_;
  HandlerHeartbeatManager hb_manager_;

  std::vector<ServerClient> qservers_;
  std::vector<boost::shared_ptr<TTransport>> qserver_transports_;

  std::map<int, HandlerClient> qhandlers_;
  std::vector<boost::shared_ptr<TTransport>> qhandler_transports_;
};

class HandlerImplProcessorFactory : public TProcessorFactory {
 public:
  HandlerImplProcessorFactory(uint32_t host_id, ConfigurationManager& conf,
                              Logger& logger,
                              HandlerHeartbeatManager& hb_manager)
      : host_id_(host_id),
        conf_(conf),
        logger_(logger),
        hb_manager_(hb_manager) {
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    logger_.Info("===Creating new processor for handler===");
    boost::shared_ptr<HandlerImpl> handler(
        new HandlerImpl(host_id_, conf_, logger_, hb_manager_));
    boost::shared_ptr<TProcessor> handlerProcessor(
        new HandlerProcessor(handler));
    return handlerProcessor;
  }

 private:
  uint32_t host_id_;
  ConfigurationManager conf_;
  Logger logger_;
  HandlerHeartbeatManager hb_manager_;
};

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
  const std::string handler_log_output = conf.Get("HANDLER_LOG_PATH_PREFIX")
      + "_" + std::to_string(host_id) + ".log";

  FILE *desc = fopen(handler_log_output.c_str(), "a");
  if (desc == NULL) {
    fprintf(stderr, "Could not obtain descriptor for %s, logging to stderr.\n",
            handler_log_output.c_str());
    desc = stderr;
  }
  Logger logger(static_cast<Logger::Level>(conf.GetInt("HANDLER_LOG_LEVEL")),
                desc);

  succinct::HandlerHeartbeatManager hb_manager(host_id, conf, logger);

  int port = conf.GetInt("HANDLER_PORT");
  try {
    shared_ptr<succinct::HandlerImplProcessorFactory> handlerFactory(
        new succinct::HandlerImplProcessorFactory(host_id, conf, logger,
                                                  hb_manager));
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(handlerFactory, server_transport, transport_factory,
                           protocol_factory);

    logger.Info("Starting Handler on port %d...", port);
    server.serve();
  } catch (std::exception& e) {
    logger.Error("Could not create handler listening on port %d. Reason: %s",
                 conf.GetInt("HANDLER_PORT"), e.what());
  }

  return 0;
}

