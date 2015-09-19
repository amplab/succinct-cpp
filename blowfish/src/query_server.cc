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

class QueryServiceHandler : virtual public QueryServiceIf {
 public:
  QueryServiceHandler(std::string data_path) {
    succinct_shard_ = NULL;
    data_path_ = data_path;
    is_init_ = false;
    num_keys_ = 0;
  }

  int32_t Initialize(int32_t id, int32_t sampling_rate) {
    if (!is_init_) {
      assert(ISPOWOF2(sampling_rate));
      std::string filename = data_path_ + "/data_" + std::to_string(id);
      fprintf(stderr,
              "Memory mapping shard with id = %d, sampling-rate = %d, path = %s...\n", id,
              sampling_rate, filename.c_str());

      succinct_shard_ = new SuccinctShard(id, filename,
                                          SuccinctMode::LOAD_MEMORY_MAPPED,
                                          sampling_rate, sampling_rate);

      fprintf(stderr, "Memory mapped data from file %s; size = %llu.\n",
              filename.c_str(), succinct_shard_->GetOriginalSize());

      num_keys_ = succinct_shard_->GetNumKeys();

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
  SuccinctShard *succinct_shard_;
  std::string data_path_;
  bool is_init_;
  uint32_t num_keys_;
};

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-p port] [-c conf-file] [file]\n", exec);
}

int main(int argc, char **argv) {

  if (argc < 2 || argc > 6) {
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

  while ((c = getopt(argc, argv, "p:")) != -1) {
    switch (c) {
      case 'p':
        port = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Unrecognized argument %c\n", c);
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string data_path = std::string(argv[optind]);

  shared_ptr<QueryServiceHandler> handler(new QueryServiceHandler(data_path));
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
