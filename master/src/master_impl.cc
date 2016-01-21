#include <vector>
#include <fstream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>

#include "Master.h"
#include "Handler.h"
#include "logger.h"
#include "configuration_manager.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace succinct {
class MasterImpl : virtual public MasterIf {

 public:
  MasterImpl(ConfigurationManager& conf, Logger& logger) {
    conf_ = conf;
    logger_ = logger;

    ReadHostNames(conf_.Get("HOSTS_LIST"));

    std::vector<HandlerClient> clients;
    std::vector<boost::shared_ptr<TTransport> > transports;
    uint16_t handler_port = conf_.GetInt("HANDLER_PORT");

    // Initiate handler to handler connections on all servers
    for (int i = 0; i < hostnames_.size(); i++) {
      logger_.Info("Connecting to handler at %s...", hostnames_[i].c_str());
      try {
        boost::shared_ptr<TSocket> socket(
            new TSocket(hostnames_[i], handler_port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        HandlerClient client(protocol);
        transport->open();
        logger_.Info("Connection successful.");
        clients.push_back(client);
        transports.push_back(transport);
      } catch (std::exception& e) {
        logger_.Error("Could not connect to handler on %s. Reason: %s",
                      hostnames_[i].c_str(), e.what());
        exit(1);
      }
    }

    // Start servers at each host
    for (int i = 0; i < hostnames_.size(); i++) {
      try {
        logger_.Info("Starting initialization at %s...", hostnames_[i].c_str());
        clients[i].send_StartLocalServers();
      } catch (std::exception& e) {
        logger_.Error("Could not send start_servers signal to %s. Reason: %s",
                      hostnames_[i].c_str(), e.what());
        exit(1);
      }
    }

    // Cleanup connections
    for (int i = 0; i < hostnames_.size(); i++) {
      try {
        clients[i].recv_StartLocalServers();
        logger_.Info("Finished initialization at %s.", hostnames_[i].c_str());
      } catch (std::exception& e) {
        logger_.Error(
            "Could not receive start_servers signal from %s. Reason: %s",
            hostnames_[i].c_str(), e.what());
        exit(1);
      }
      try {
        transports[i]->close();
        logger_.Info("Closed connection.");
      } catch (std::exception& e) {
        logger_.Error("Could not close connection to %s. Reason: %s",
                      hostnames_[i].c_str(), e.what());
        exit(1);
      }
    }

    logger_.Info("Initialization complete.");
  }

  void GetHostname(std::string& _return) {
    _return = hostnames_[rand() % hostnames_.size()];
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

  std::vector<std::string> hostnames_;
  ConfigurationManager conf_;
  Logger logger_;
};

}

int main(int argc, char **argv) {

  ConfigurationManager conf;
  std::string master_log_output = conf.Get("MASTER_LOG_FILE");
  FILE *desc = fopen(master_log_output.c_str(), "a");
  if (desc == NULL) {
    fprintf(stderr, "Could not obtain descriptor for %s, logging to stderr.\n",
            master_log_output.c_str());
    desc = stderr;
  }

  Logger logger(static_cast<Logger::Level>(conf.GetInt("MASTER_LOG_LEVEL")),
                desc);
  shared_ptr<succinct::MasterImpl> handler(
      new succinct::MasterImpl(conf, logger));
  shared_ptr<TProcessor> processor(new succinct::MasterProcessor(handler));

  try {
    shared_ptr<TServerSocket> server_transport(
        new TServerSocket(conf.GetInt("MASTER_PORT")));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(processor, server_transport, transport_factory,
                           protocol_factory);
    logger.Info("Starting Master on port %d...", conf.GetInt("MASTER_PORT"));
    server.serve();
  } catch (std::exception& e) {
    logger.Error("Could not create master listening on port %d. Reason: %s",
                 conf.GetInt("MASTER_PORT"), e.what());
  }

  return 0;
}
