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
#include "KVAggregatorService.h"
#include "KVQueryService.h"
#include "succinctkv_constants.h"
#include "kv_ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class KVAggregatorServiceHandler : virtual public KVAggregatorServiceIf {
 public:
  KVAggregatorServiceHandler(uint32_t num_shards) {
    num_shards_ = num_shards;
  }

  int32_t Initialize() {
    // Connect to query servers and start initialization
    fprintf(stderr, "Num shards = %u\n", num_shards_);
    for (uint32_t i = 0; i < num_shards_; i++) {
      int port = KV_SERVER_PORT + i;
      boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
      boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
      boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
      KVQueryServiceClient client(protocol);
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

    for (auto transport : qserver_transports_) {
      transport->close();
    }

    qserver_transports_.clear();
    qservers_.clear();

    fprintf(stderr, "All QueryServers successfully initialized!\n");

    return 0;
  }

  void Get(std::string& _return, const int64_t key) {
    uint32_t qserver_id = key / SuccinctShard::MAX_KEYS;
    qservers_.at(qserver_id).Get(_return, key % SuccinctShard::MAX_KEYS);
  }

  void Access(std::string& _return, const int64_t key, const int32_t offset,
              const int32_t len) {
    uint32_t qserver_id = key / SuccinctShard::MAX_KEYS;
    qservers_.at(qserver_id).Access(_return, key % SuccinctShard::MAX_KEYS,
                                    offset, len);
  }

  void Search(std::set<int64_t> & _return, const std::string& query) {
    for (int j = 0; j < qservers_.size(); j++) {
      qservers_[j].send_Search(query);
    }

    for (int j = 0; j < qservers_.size(); j++) {
      std::set<int64_t> keys;
      qservers_[j].recv_Search(keys);
      _return.insert(keys.begin(), keys.end());
    }
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

  int32_t GetNumShards() {
    return num_shards_;
  }

  int32_t GetNumKeys() {
    int32_t num_keys = 0;
    for (auto client : qservers_) {
      num_keys += client.GetNumKeys();
    }
    return num_keys;
  }

  int32_t GetNumKeysShard(const int32_t shard_id) {
    if (shard_id < 0 || shard_id >= num_shards_) {
      return 0;
    }
    return qservers_.at(shard_id).GetNumKeys();
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
        boost::shared_ptr<TSocket> socket(
            new TSocket("localhost", KV_SERVER_PORT + i));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        KVQueryServiceClient qsclient(protocol);
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
  std::vector<KVQueryServiceClient> qservers_;
  std::vector<boost::shared_ptr<TTransport>> qserver_transports_;
  uint32_t num_shards_;
};

class KVHandlerProcessorFactory : public TProcessorFactory {
 public:
  KVHandlerProcessorFactory(uint32_t num_shards) {
    num_shards_ = num_shards;
  }

  boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
    boost::shared_ptr<KVAggregatorServiceHandler> handler(
        new KVAggregatorServiceHandler(num_shards_));
    boost::shared_ptr<TProcessor> handlerProcessor(
        new KVAggregatorServiceProcessor(handler));
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

  int port = KV_AGGREGATOR_PORT;
  try {
    shared_ptr<KVHandlerProcessorFactory> handlerFactory(
        new KVHandlerProcessorFactory(num_shards));
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

