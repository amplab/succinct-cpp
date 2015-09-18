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
#include <sstream>

#include "layered_succinct_shard.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

typedef std::map<uint32_t, uint32_t> ConfigMap;

class QueryServiceHandler : virtual public QueryServiceIf {
 public:
  QueryServiceHandler(std::string filename, ConfigMap& sr_map) {
    succinct_shard_ = NULL;
    filename_ = filename;
    is_init_ = false;
    num_keys_ = 0;
    sr_map_ = sr_map;
  }

  int32_t Initialize(int32_t id) {
    if(!is_init_) {
      fprintf(stderr, "Memory mapping data structures...\n");

      succinct_shard_ = new LayeredSuccinctShard(id, filename_,
          SuccinctMode::LOAD_MEMORY_MAPPED, 2, 2);

      fprintf(stderr, "Memory mapped data from file %s; size = %llu.\n",
              filename_.c_str(), succinct_shard_->GetOriginalSize());

      num_keys_ = succinct_shard_->GetNumKeys();

      // Drop layers as required
      uint32_t n_layers_to_remove = SuccinctUtils::IntegerLog2(sr_map_[id]) - 1;
      fprintf(stderr, "Dropping %u layers...\n", n_layers_to_remove);
      for (uint32_t i = 0; i < n_layers_to_remove; i++) {
        fprintf(stderr, "Dropping layer %u\n", i);
        succinct_shard_->RemoveLayer(i);
      }

      is_init_ = true;
      fprintf(stderr, "Ready!\n");
      return 0;
    }

    return -1;
  }

  void Get(std::string& _return, const int64_t key) {
    succinct_shard_->Get(_return, key);
  }

  void Search(std::set<int64_t>& _return, const std::string& query) {
    succinct_shard_->Search(_return, query);
  }

  int32_t GetNumKeys() {
    return succinct_shard_->GetNumKeys();
  }

  int64_t GetShardSize() {
    return succinct_shard_->GetOriginalSize();
  }

 private:
  LayeredSuccinctShard *succinct_shard_;
  std::string filename_;
  bool is_init_;
  uint32_t num_keys_;
  ConfigMap sr_map_;
};


void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-p port] [-s sampling-rate] [file]\n", exec);
}

int main(int argc, char **argv) {

  if (argc < 2 || argc > 4) {
    print_usage(argv[0]);
    return -1;
  }

  fprintf(stderr, "Command line: ");
  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "%s ", argv[i]);
  }
  fprintf(stderr, "\n");

  int c;
  uint32_t port = QUERY_SERVER_PORT;
  std::string conf_file = "conf/succinct.conf";

  while ((c = getopt(argc, argv, "p:c:")) != -1) {
    switch (c) {
      case 'p':
        port = atoi(optarg);
        break;
      case 'c':
        conf_file = optarg;
        break;
      default:
        fprintf(stderr, "Unrecognized argument %c\n", c);
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string filename = std::string(argv[optind]);

  // Parse config file
  std::ifstream conf(conf_file);
  std::string line;
  ConfigMap sr_map;
  while (std::getline(conf, line)) {
    std::istringstream iss(line);
    uint32_t id, sr;
    if (!(iss >> id >> sr)) {
      fprintf(stderr, "Malformed config file\n");
      return -1;
    }
    sr_map[id] = sr;
  }

  shared_ptr<QueryServiceHandler> handler(
      new QueryServiceHandler(filename, sr_map));
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
