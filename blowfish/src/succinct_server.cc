#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <sstream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "succinct_shard.h"
#include "load_balancer.h"
#include "SuccinctService.h"
#include "succinct_constants.h"
#include "QueryService.h"
#include "ports.h"
#include "shard_config.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class SuccinctServiceHandler : virtual public SuccinctServiceIf {
 public:
  SuccinctServiceHandler(uint32_t local_host_id, uint32_t num_shards,
                         std::vector<std::string> hostnames, ConfigMap& conf) {
    this->local_host_id_ = local_host_id;
    this->num_shards_ = num_shards;
    this->hostnames_ = hostnames;
    this->conf_ = conf;
    this->load_balancer_ = new LoadBalancer(conf);
  }

  int32_t StartLocalServers() {
    return Initialize(0);
  }

  int32_t Initialize(const int32_t mode) {
    // Connect to query servers and start initialization
    for (uint32_t i = 0; i < num_shards_; i++) {
      int port = QUERY_SERVER_PORT + i;
      boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
      boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
      boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
      QueryServiceClient client(protocol);
      transport->open();
      fprintf(stderr, "Connected to QueryServer %u!\n", i);
      IdType shard_id = i * hostnames_.size() + local_host_id_;
      fprintf(stderr, "Initializing shard %d with sampling rate = %d\n",
              shard_id, conf_.at(shard_id).sampling_rate);
      client.send_Initialize(shard_id, conf_.at(shard_id).sampling_rate);
      qservers_.push_back(client);
      qserver_transports_.push_back(transport);
    }

    for (auto client : qservers_) {
      int32_t status = client.recv_Initialize();
      if (status) {
        fprintf(stderr, "Initialization failed, status = %d!\n", status);
        return status;
      }
    }

    fprintf(stderr, "All QueryServers successfully initialized!\n");

    return 0;
  }

  void Get(std::string& _return, const int32_t shard_id, const int64_t key) {
    IdType replica_id = load_balancer_->GetReplica(shard_id);
    IdType host_id = replica_id % hostnames_.size();
    IdType qserver_id = replica_id / hostnames_.size();

    if (host_id == 0) {
      // Fetch from next 10 shards
      for (IdType i = replica_id; i < replica_id + 10; i++) {
        Get(_return, replica_id, key);
      }
    } else {

      if (host_id == local_host_id_) {
        GetLocal(_return, qserver_id, key);
      } else {
        qhandlers_.at(host_id).GetLocal(_return, qserver_id, key);
      }
    }
  }

  void GetLocal(std::string& _return, const int32_t qserver_id,
                const int64_t key) {
    qservers_.at(qserver_id).Get(_return, key);
  }

  void Search(std::set<int64_t> & _return, const int32_t shard_id,
              const std::string& query) {
    IdType replica_id = load_balancer_->GetReplica(shard_id);
    IdType host_id = replica_id % hostnames_.size();
    IdType qserver_id = replica_id / hostnames_.size();

    if (host_id == 0) {
      // Fetch from next 10 shards
      for (IdType i = replica_id; i < replica_id + 10; i++) {
        _return.clear();
        Search(_return, replica_id, query);
      }
    } else {
      if (host_id == local_host_id_) {
        SearchLocal(_return, qserver_id, query);
      } else {
        qhandlers_.at(host_id).SearchLocal(_return, qserver_id, query);
      }
    }
  }

  void SearchLocal(std::set<int64_t> & _return, const int32_t qserver_id,
                   const std::string& query) {
    qservers_.at(qserver_id).Search(_return, query);
  }

  void FetchShardData(std::string& _return, const int32_t shard_id,
                      const std::string& filename, const int64_t offset,
                      const int64_t length) {
    IdType host_id = shard_id % hostnames_.size();
    IdType qserver_id = shard_id / hostnames_.size();

    assert(host_id == local_host_id_);
    qservers_.at(qserver_id).FetchData(_return, filename, offset, length);
  }

  int32_t GetNumHosts() {
    return hostnames_.size();
  }

  int32_t GetNumShards(const int32_t host_id) {
    int32_t num;
    if (host_id == local_host_id_) {
      return num_shards_;
    }
    return qhandlers_.at(host_id).GetNumShards(host_id);
  }

  int32_t GetNumKeys(const int32_t shard_id) {
    int32_t host_id = shard_id % hostnames_.size();
    int32_t num;
    if (host_id == local_host_id_) {
      return qservers_.at(shard_id / hostnames_.size()).GetNumKeys();
    }
    return qhandlers_.at(host_id).GetNumKeys(shard_id);
  }

  int64_t GetTotSize() {
    int64_t tot_size = 0;
    for (auto client : qservers_) {
      tot_size += client.GetShardSize();
    }
    return tot_size;
  }

  int32_t ConnectToHandlers() {
    // Create connections to all Succinct Clients
    for (int i = 0; i < hostnames_.size(); i++) {
      fprintf(stderr, "Connecting to %s:%d...\n", hostnames_[i].c_str(),
      QUERY_HANDLER_PORT);
      try {
        if (i == local_host_id_) {
          fprintf(stderr, "Self setup:\n");
          ConnectToLocalServers();
        } else {
          boost::shared_ptr<TSocket> socket(
              new TSocket(hostnames_[i], QUERY_HANDLER_PORT));
          boost::shared_ptr<TTransport> transport(
              new TBufferedTransport(socket));
          boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
          SuccinctServiceClient client(protocol);
          transport->open();
          fprintf(stderr, "Connected!\n");
          qhandler_transports_.push_back(transport);
          qhandlers_.insert(std::pair<int, SuccinctServiceClient>(i, client));
          fprintf(stderr, "Asking %s to connect to local servers...\n",
                  hostnames_[i].c_str());
          client.ConnectToLocalServers();
        }
      } catch (std::exception& e) {
        fprintf(stderr, "Client is not up...: %s\n ", e.what());
        return 1;
      }
    }
    fprintf(stderr, "Currently have %zu connections.\n", qhandlers_.size());
    return 0;
  }

  int32_t ConnectToLocalServers() {
    for (int i = 0; i < num_shards_; i++) {
      fprintf(stderr, "Connecting to local server %d...", i);
      try {
        boost::shared_ptr<TSocket> socket(
            new TSocket("localhost", QUERY_SERVER_PORT + i));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        QueryServiceClient qsclient(protocol);
        transport->open();
        fprintf(stderr, "Connected!\n");
        qservers_.push_back(qsclient);
        qserver_transports_.push_back(transport);
        fprintf(stderr, "Pushed!\n");
      } catch (std::exception& e) {
        fprintf(stderr, "Could not connect to server...: %s\n", e.what());
        return 1;
      }
    }
    fprintf(stderr, "Currently have %zu local server connections.\n",
            qservers_.size());
    return 0;
  }

  int32_t DisconnectFromHandlers() {
    // Destroy connections to all Succinct Handlers
    for (int i = 0; i < hostnames_.size(); i++) {
      try {
        if (i == local_host_id_) {
          fprintf(stderr, "Killing local server connections...\n");
          DisconnectFromLocalServers();
        } else {
          fprintf(stderr,
                  "Asking client %s:%d to kill local server connections...\n",
                  hostnames_[i].c_str(), QUERY_HANDLER_PORT);
          qhandlers_.at(i).DisconnectFromLocalServers();
          fprintf(stderr, "Closing connection to %s:%d...",
                  hostnames_[i].c_str(),
                  QUERY_HANDLER_PORT);
          qhandler_transports_[i]->close();
          fprintf(stderr, "Closed!\n");
        }
      } catch (std::exception& e) {
        fprintf(stderr, "Could not close connection: %s\n", e.what());
        return 1;
      }
    }
    qhandler_transports_.clear();
    qhandlers_.clear();

    return 0;
  }

  int32_t DisconnectFromLocalServers() {
    for (int i = 0; i < qservers_.size(); i++) {
      try {
        fprintf(stderr, "Disconnecting from local server %d\n", i);
        qserver_transports_[i]->close();
        fprintf(stderr, "Disconnected!\n");
      } catch (std::exception& e) {
        fprintf(stderr, "Could not close local connection: %s\n", e.what());
        return 1;
      }
    }
    qserver_transports_.clear();
    qhandlers_.clear();
    return 0;
  }

 private:
  std::vector<std::string> hostnames_;
  std::vector<QueryServiceClient> qservers_;
  std::vector<boost::shared_ptr<TTransport>> qserver_transports_;
  std::vector<boost::shared_ptr<TTransport>> qhandler_transports_;
  std::map<int, SuccinctServiceClient> qhandlers_;
  uint32_t num_shards_;
  uint32_t local_host_id_;
  ConfigMap conf_;
  LoadBalancer *load_balancer_;
};

class HandlerProcessorFactory : public TProcessorFactory {
 public:
  HandlerProcessorFactory(uint32_t local_host_id, uint32_t num_shards,
                          std::vector<std::string> hostnames, ConfigMap& conf) {
    this->local_host_id_ = local_host_id;
    this->num_shards_ = num_shards;
    this->hostnames_ = hostnames;
    this->conf_ = conf;
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    boost::shared_ptr<SuccinctServiceHandler> handler(
        new SuccinctServiceHandler(local_host_id_, num_shards_, hostnames_,
                                   conf_));
    boost::shared_ptr<TProcessor> handlerProcessor(
        new SuccinctServiceProcessor(handler));
    return handlerProcessor;
  }

 private:
  std::vector<std::string> hostnames_;
  uint32_t local_host_id_;
  uint32_t num_shards_;
  ConfigMap conf_;
};

void ParseHosts(std::vector<std::string>& hostnames, std::string& hosts_file) {
  std::ifstream hosts(hosts_file);
  std::string host;
  while (std::getline(hosts, host)) {
    hostnames.push_back(host);
  }
}

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-s num-shards] [-h hosts-file] [-c conf-file] [-i host-id]\n",
      exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 10) {
    print_usage(argv[0]);
    return -1;
  }

  fprintf(stderr, "Command line: ");
  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "%s ", argv[i]);
  }
  fprintf(stderr, "\n");

  int c;
  std::string hosts_file = "./conf/hosts";
  std::string conf_file = "./conf/blowfish.conf";
  uint32_t num_shards = 1;
  uint32_t local_host_id = 0;
  ConfigMap conf;
  std::vector<std::string> hostnames;

  while ((c = getopt(argc, argv, "s:h:c:i:")) != -1) {
    switch (c) {
      case 's': {
        num_shards = atoi(optarg);
        break;
      }
      case 'h': {
        hosts_file = optarg;
        break;
      }
      case 'c': {
        conf_file = optarg;
        break;
      }
      case 'i': {
        local_host_id = atoi(optarg);
        break;
      }
      default: {
        fprintf(stderr, "Error parsing command line arguments\n");
        return -1;
      }
    }
  }

  ParseConfig(conf, conf_file);
  ParseHosts(hostnames, hosts_file);

  int port = QUERY_HANDLER_PORT;
  try {
    shared_ptr<HandlerProcessorFactory> handlerFactory(
        new HandlerProcessorFactory(local_host_id, num_shards, hostnames,
                                    conf));
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(handlerFactory, server_transport, transport_factory,
                           protocol_factory);
    server.serve();
  } catch (std::exception& e) {
    fprintf(stderr, "Exception at succinct_server:main(): %s\n", e.what());
  }

  return 0;
}
