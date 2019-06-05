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
#include "AggregatorService.h"
#include "succinct_constants.h"
#include "QueryService.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using stdcxx::shared_ptr;

class AggregatorServiceHandler : virtual public AggregatorServiceIf {
 public:
  AggregatorServiceHandler(uint32_t num_shards) {
    num_shards_ = num_shards;
  }

  int32_t Initialize() {
    // Connect to query servers and start initialization
    fprintf(stderr, "Num shards = %u\n", num_shards_);
    for (uint32_t i = 0; i < num_shards_; i++) {
      int port = SERVER_PORT + i;
      stdcxx::shared_ptr<TSocket> socket(new TSocket("localhost", port));
      stdcxx::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
      stdcxx::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
      QueryServiceClient client(protocol);
      transport->open();
      fprintf(stderr, "Connected to QueryServer %u!\n", i);
      client.send_Initialize(i);
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

    for (auto transport : qserver_transports_) {
      transport->close();
    }
    qserver_transports_.clear();
    qservers_.clear();

    return 0;
  }

  void Regex(std::set<int64_t> & _return, const std::string& query) {
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_Regex(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      std::set<int64_t> results;
      qservers_[j].recv_Regex(results);
      _return.insert(results.begin(), results.end());
    }
  }

  int64_t Count(const std::string& query) {
    int64_t ret = 0;
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_Count(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      ret += qservers_[j].recv_Count();
    }
    return ret;
  }

  void Extract(std::string& _return, const int64_t offset, const int64_t len) {
    auto it = greatest_less(shard_map_, offset);
    if(it != shard_map_.end()) {
      size_t shard_id = it->second;
      size_t shard_offset = it->first;
      qservers_.at(shard_id).Extract(_return, offset - shard_offset, len);
    } else {
      // TODO: Thrift does not support empty string/null string returns
      // Find a better way to indicate invalid offset
      _return = "**invalid offset**";
    }
  }

  void Search(std::vector<int64_t> & _return, const std::string& query) {
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_Search(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      std::vector<int64_t> results;
      qservers_[j].recv_Search(results);
      for (auto offset : results) {
        _return.push_back(offset_map_[j] + offset);
      }
    }
  }

  int32_t GetNumShards() {
    return num_shards_;
  }

  int64_t GetTotSize() {
    int64_t tot_size = 0;
    for (auto client : qservers_) {
      tot_size += client.GetShardSize();
    }
    return tot_size;
  }

  int32_t ConnectToServers() {
    for (int i = 0; i < num_shards_; i++) {
      fprintf(stderr, "Connecting to local server %d...", i);
      try {
        stdcxx::shared_ptr<TSocket> socket(
            new TSocket("localhost", SERVER_PORT + i));
        stdcxx::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        stdcxx::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
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

  int32_t DisconnectFromServers() {
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
    qservers_.clear();
    return 0;
  }

 private:

  template<typename Map> typename Map::const_iterator greatest_less(
      Map const& m, typename Map::key_type const& k) {
    typename Map::const_iterator it = m.lower_bound(k);
    if (it != m.begin()) {
      return --it;
    }
    return m.end();
  }

  template<typename Map> typename Map::iterator greatest_less(
      Map & m, typename Map::key_type const& k) {
    typename Map::iterator it = m.lower_bound(k);
    if (it != m.begin()) {
      return --it;
    }
    return m.end();
  }

  std::vector<QueryServiceClient> qservers_;
  std::vector<stdcxx::shared_ptr<TTransport>> qserver_transports_;
  std::map<int64_t, size_t> shard_map_;
  std::map<size_t, int64_t> offset_map_;
  uint32_t num_shards_;
};

class HandlerProcessorFactory : public TProcessorFactory {
 public:
  HandlerProcessorFactory(uint32_t num_shards) {
    num_shards_ = num_shards;
  }

  stdcxx::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    stdcxx::shared_ptr<AggregatorServiceHandler> handler(
        new AggregatorServiceHandler(num_shards_));
    stdcxx::shared_ptr<TProcessor> handlerProcessor(
        new AggregatorServiceProcessor(handler));
    return handlerProcessor;
  }

 private:
  uint32_t num_shards_;
};

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-s num_shards]\n", exec);
}

int main(int argc, char **argv) {
  if (argc < 1 || argc > 3) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t num_shards = 1;

  while ((c = getopt(argc, argv, "s:")) != -1) {
    switch (c) {
      case 's': {
        num_shards = atoi(optarg);
        break;
      }
      default: {
        fprintf(stderr, "Error parsing command line arguments.\n");
        return -1;
      }
    }
  }

  int port = AGGREGATOR_PORT;
  try {
    shared_ptr<HandlerProcessorFactory> handlerFactory(
        new HandlerProcessorFactory(num_shards));
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(handlerFactory, server_transport, transport_factory,
                           protocol_factory);
    server.serve();
  } catch (std::exception& e) {
    fprintf(stderr, "Exception at aggregator.cc:main(): %s\n", e.what());
  }

  return 0;
}

