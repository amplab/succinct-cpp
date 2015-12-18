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

#include "succinct_shard.h"
#include "load_balancer.h"
#include "SuccinctService.h"
#include "succinct_constants.h"
#include "QueryService.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class SuccinctServiceHandler : virtual public SuccinctServiceIf {
 public:
  SuccinctServiceHandler(std::string filename, uint32_t local_host_id,
                         uint32_t num_shards,
                         std::vector<std::string> hostnames, bool construct,
                         std::vector<double> distribution,
                         std::vector<uint32_t> sampling_rates,
                         uint32_t num_failures) {
    this->local_host_id_ = local_host_id;
    this->num_shards_ = num_shards;
    this->hostnames_ = hostnames;
    this->filename_ = filename;
    this->construct_ = construct;
    this->balancer_ = new LoadBalancer(distribution);
    this->sampling_rates_ = sampling_rates;
    this->num_failures_ = num_failures;
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
      int32_t shard_id = i * hostnames_.size() + local_host_id_;
      client.send_Initialize(shard_id);
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

    size_t offset = 0, client_id = 0;
    for (auto client : qservers_) {
      shard_map_[offset] = client_id;
      offset_map_[client_id] = offset;
      fprintf(stderr, "(%zu, %zu)\n", offset, client_id);
      client_id++;
      offset += client.GetShardSize();
    }

    fprintf(stderr, "All QueryServers successfully initialized!\n");

    return 0;
  }

  void Get(std::string& _return, const int64_t key) {
    uint32_t shard_id = (uint32_t) (key / SuccinctShard::MAX_KEYS)
        * balancer_->num_replicas() + balancer_->get_replica();
    uint32_t host_id = shard_id % hostnames_.size();
    // Currently only supports single failure emulation
    if (host_id < num_failures_) {
      // Get new replica#
      uint32_t replica_num =
          (shard_id % balancer_->num_replicas() == 0) ? 1 : 0;
      shard_id = (uint32_t) ((key / SuccinctShard::MAX_KEYS)
          * balancer_->num_replicas()) + replica_num;
      assert(host_id != shard_id % hostnames_.size());
      host_id = shard_id % hostnames_.size();
    }
    uint32_t qserver_id = shard_id / hostnames_.size();
    if (host_id == local_host_id_) {
      GetLocal(_return, qserver_id, key % SuccinctShard::MAX_KEYS);
    } else {
      qhandlers_.at(host_id).GetLocal(_return, qserver_id,
                                       key % SuccinctShard::MAX_KEYS);
    }
  }

  void GetLocal(std::string& _return, const int32_t qserver_id,
                const int64_t key) {
    qservers_.at(qserver_id).Get(_return, key);
  }

  void Access(std::string& _return, const int64_t key, const int32_t offset,
              const int32_t len) {
    uint32_t shard_id = (uint32_t) (key / SuccinctShard::MAX_KEYS)
        * balancer_->num_replicas() + balancer_->get_replica();
    uint32_t host_id = shard_id % hostnames_.size();
    // Currently only supports single failure emulation
    if (host_id < num_failures_) {
      // Get new replica#
      uint32_t replica_num =
          (shard_id % balancer_->num_replicas() == 0) ? 1 : 0;
      shard_id = (uint32_t) ((key / SuccinctShard::MAX_KEYS)
          * balancer_->num_replicas()) + replica_num;
      assert(host_id != shard_id % hostnames_.size());
      host_id = shard_id % hostnames_.size();
    }
    uint32_t qserver_id = shard_id / hostnames_.size();
    if (host_id == local_host_id_) {
      AccessLocal(_return, qserver_id, key % SuccinctShard::MAX_KEYS, offset,
                  len);
    } else {
      qhandlers_.at(host_id).AccessLocal(_return, qserver_id,
                                          key % SuccinctShard::MAX_KEYS, offset,
                                          len);
    }
  }

  void AccessLocal(std::string& _return, const int32_t qserver_id,
                   const int64_t key, const int32_t offset, const int32_t len) {
    qservers_.at(qserver_id).Access(_return, key, offset, len);
  }

  void SearchLocal(std::set<int64_t> & _return, const std::string& query) {
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_Search(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      std::set<int64_t> keys;
      qservers_[j].recv_Search(keys);
      _return.insert(keys.begin(), keys.end());
    }
  }

  void Search(std::set<int64_t> & _return, const std::string& query) {

    for (int i = 0; i < hostnames_.size(); i++) {
      if (i == local_host_id_) {
        for (int j = 0; j < qservers_.size(); j++) {
          qservers_[j].send_Search(query);
        }
      } else {
        qhandlers_.at(i).send_SearchLocal(query);
      }
    }

    for (int i = 0; i < hostnames_.size(); i++) {
      if (i == local_host_id_) {
        for (int j = 0; j < qservers_.size(); j++) {
          std::set<int64_t> keys;
          qservers_[j].recv_Search(keys);
          _return.insert(keys.begin(), keys.end());
        }
      } else {
        std::set<int64_t> keys;
        qhandlers_.at(i).recv_Search(keys);
        _return.insert(keys.begin(), keys.end());
      }
    }
  }

  void RegexSearchLocal(std::set<int64_t> & _return, const std::string& query) {
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_RegexSearch(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      std::set<int64_t> results;
      qservers_[j].recv_RegexSearch(results);
      _return.insert(results.begin(), results.end());
    }
  }

  void RegexSearch(std::set<int64_t> & _return, const std::string& query) {

    for (int i = 0; i < hostnames_.size(); i++) {
      if (i == local_host_id_) {
        for (int j = 0; j < qservers_.size(); j++) {
          qservers_[j].send_RegexSearch(query);
        }
      } else {
        qhandlers_.at(i).send_RegexSearchLocal(query);
      }
    }

    for (int i = 0; i < hostnames_.size(); i++) {
      if (i == local_host_id_) {
        for (int j = 0; j < qservers_.size(); j++) {
          std::set<int64_t> results;
          qservers_[j].recv_RegexSearch(results);
          _return.insert(results.begin(), results.end());
        }
      } else {
        std::set<int64_t> results;
        qhandlers_.at(i).recv_RegexSearch(results);
        _return.insert(results.begin(), results.end());
      }
    }
  }

  void RegexCountLocal(std::vector<int64_t> & _return,
                       const std::string& query) {
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_RegexCount(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      std::vector<int64_t> results;
      qservers_[j].recv_RegexCount(results);
      _return.insert(_return.begin(), results.begin(), results.end());
    }
  }

  void RegexCount(std::vector<int64_t> & _return, const std::string& query) {

    for (int i = 0; i < hostnames_.size(); i++) {
      if (i == local_host_id_) {
        for (int j = 0; j < qservers_.size(); j++) {
          qservers_[j].send_RegexCount(query);
        }
      } else {
        qhandlers_.at(i).send_RegexCountLocal(query);
      }
    }

    for (int i = 0; i < hostnames_.size(); i++) {
      if (i == local_host_id_) {
        for (int j = 0; j < qservers_.size(); j++) {
          std::vector<int64_t> results;
          qservers_[j].recv_RegexCount(results);
          _return.insert(_return.begin(), results.begin(), results.end());
        }
      } else {
        std::vector<int64_t> results;
        qhandlers_.at(i).recv_RegexCount(results);
        _return.insert(_return.begin(), results.begin(), results.end());
      }
    }
  }

  void FlatExtract(std::string& _return, const int64_t offset,
                   const int64_t len) {
    auto it = std::prev(shard_map_.upper_bound(offset));
    size_t shard_id = it->second;
    size_t shard_offset = it->first;
    qservers_.at(shard_id).FlatExtract(_return, offset - shard_offset, len);
  }

  void FlatExtractLocal(std::string& _return, const int64_t offset,
                        const int64_t len) {
    auto it = std::prev(shard_map_.upper_bound(offset));
    size_t shard_id = it->second;
    size_t shard_offset = it->first;
    qservers_.at(shard_id).FlatExtract(_return, offset - shard_offset, len);
  }

  void FlatSearchLocal(std::vector<int64_t> & _return,
                       const std::string& query) {
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_FlatSearch(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      std::vector<int64_t> results;
      qservers_[j].recv_FlatSearch(results);
      for (auto offset : results) {
        _return.push_back(offset_map_[j] + offset);
      }
    }
  }

  void FlatSearch(std::vector<int64_t> & _return, const std::string& query) {
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_FlatSearch(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      std::vector<int64_t> results;
      qservers_[j].recv_FlatSearch(results);
      for (auto offset : results) {
        _return.push_back(offset_map_[j] + offset);
      }
    }
  }

  int64_t FlatCountLocal(const std::string& query) {
    int64_t ret = 0;
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_FlatCount(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      ret += qservers_[j].recv_FlatCount();
    }
    return ret;
  }

  int64_t FlatCount(const std::string& query) {
    int64_t ret = 0;
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_FlatCount(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      ret += qservers_[j].recv_FlatCount();
    }
    return ret;
  }

  int64_t CountLocal(const std::string& query) {
    int64_t ret = 0;
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_Count(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      ret += qservers_[j].recv_Count();
    }
    return ret;
  }

  int64_t Count(const std::string& query) {
    int64_t ret = 0;

    for (int i = 0; i < hostnames_.size(); i++) {
      if (i == local_host_id_) {
        for (int j = 0; j < qservers_.size(); j++) {
          qservers_[j].send_Count(query);
        }
      } else {
        qhandlers_.at(i).send_CountLocal(query);
      }
    }

    for (int i = 0; i < hostnames_.size(); i++) {
      if (i == local_host_id_) {
        for (int j = 0; j < qservers_.size(); j++) {
          ret += qservers_[j].recv_Count();
        }
      } else {
        ret += qhandlers_.at(i).recv_CountLocal();
      }
    }

    return ret;
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
  std::string filename_;
  bool construct_;
  std::vector<std::string> hostnames_;
  std::vector<QueryServiceClient> qservers_;
  std::vector<boost::shared_ptr<TTransport>> qserver_transports_;
  std::vector<boost::shared_ptr<TTransport>> qhandler_transports_;
  std::map<int, SuccinctServiceClient> qhandlers_;
  std::map<int64_t, size_t> shard_map_;
  std::map<size_t, int64_t> offset_map_;
  uint32_t num_shards_;
  uint32_t local_host_id_;
  uint32_t num_failures_;
  LoadBalancer *balancer_;
  std::vector<uint32_t> sampling_rates_;
};

class HandlerProcessorFactory : public TProcessorFactory {
 public:
  HandlerProcessorFactory(std::string filename, uint32_t local_host_id,
                          uint32_t num_shards,
                          std::vector<std::string> hostnames, bool construct,
                          std::vector<double> distribution,
                          std::vector<uint32_t> sampling_rates,
                          uint32_t num_failures) {
    this->filename_ = filename;
    this->local_host_id_ = local_host_id;
    this->num_shards_ = num_shards;
    this->hostnames_ = hostnames;
    this->construct_ = construct;
    this->distribution_ = distribution;
    this->sampling_rates_ = sampling_rates;
    this->num_failures_ = num_failures;
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    boost::shared_ptr<SuccinctServiceHandler> handler(
        new SuccinctServiceHandler(filename_, local_host_id_, num_shards_,
                                   hostnames_, construct_, distribution_,
                                   sampling_rates_, num_failures_));
    boost::shared_ptr<TProcessor> handlerProcessor(
        new SuccinctServiceProcessor(handler));
    return handlerProcessor;
  }

 private:
  std::vector<std::string> hostnames_;
  std::string filename_;
  uint32_t local_host_id_;
  uint32_t num_shards_;
  std::vector<double> distribution_;
  std::vector<uint32_t> sampling_rates_;
  bool construct_;
  uint32_t num_failures_;

};

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-m mode] [-s num_shards] [-r replfile] [-h hostsfile] [-i hostid] [-f num_failures] file\n",
      exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 14) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t mode = 0, num_shards = 1;
  std::string hostsfile = "./conf/hosts";
  std::string replfile = "./conf/repl";
  uint32_t local_host_id = 0;
  uint32_t num_failures = 0;

  while ((c = getopt(argc, argv, "m:s:r:h:i:f:")) != -1) {
    switch (c) {
      case 'm': {
        mode = atoi(optarg);
        break;
      }
      case 's': {
        num_shards = atoi(optarg);
        break;
      }
      case 'r': {
        replfile = optarg;
        break;
      }
      case 'h': {
        hostsfile = optarg;
        break;
      }
      case 'i': {
        local_host_id = atoi(optarg);
        break;
      }
      case 'f': {
        num_failures = atoi(optarg);
        break;
      }
      default: {
        fprintf(stderr, "Error parsing command line arguments\n");
        return -1;
      }
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string filename = std::string(argv[optind]);
  bool construct = (mode == 0) ? true : false;

  std::ifstream hosts(hostsfile);
  std::string host;
  std::vector<std::string> hostnames;
  while (std::getline(hosts, host, '\n')) {
    hostnames.push_back(host);
  }

  std::ifstream repl(replfile);
  std::string repl_entry;
  std::vector<uint32_t> sampling_rates;
  std::vector<double> distribution;
  while (std::getline(repl, repl_entry, '\n')) {
    std::istringstream iss(repl_entry);
    std::string sr, dist;
    iss >> sr;
    iss >> dist;
    sampling_rates.push_back(atoi(sr.c_str()));
    distribution.push_back(atof(dist.c_str()));
  }

  int port = QUERY_HANDLER_PORT;
  try {
    shared_ptr<HandlerProcessorFactory> handlerFactory(
        new HandlerProcessorFactory(filename, local_host_id, num_shards,
                                    hostnames, construct, distribution,
                                    sampling_rates, num_failures));
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(handlerFactory, server_transport, transport_factory,
                           protocol_factory);
    server.serve();
  } catch (std::exception& e) {
    fprintf(stderr, "Exception at SuccinctServer:main(): %s\n", e.what());
  }

  return 0;
}

