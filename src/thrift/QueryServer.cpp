#include "thrift/QueryService.h"
#include "thrift/succinct_constants.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include <cstdio>
#include <fstream>
#include <cstdint>

#include "../../include/succinct/KVStoreShard.hpp"
#include "thrift/ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class QueryServiceHandler : virtual public QueryServiceIf {
public:
    QueryServiceHandler(std::string filename) {
        this->fd = NULL;
        this->construct = construct;
        this->filename = filename;
        this->is_init = false;
        std::ifstream input(filename);
        this->num_keys = std::count(std::istreambuf_iterator<char>(input),
                        std::istreambuf_iterator<char>(), '\n');
        input.close();
    }

    int32_t init(int32_t id) {
        fprintf(stderr, "Received INIT signal, initializing data structures...\n");

        fd = new KVStoreShard(id, filename, num_keys);

        fprintf(stderr, "KV Store data size = %llu\n", fd->size());
        fprintf(stderr, "Done initializing...\n");
        fprintf(stderr, "Waiting for queries...\n");
        is_init = true;
        return 0;
    }

    void get(std::string& _return, const int64_t key) {
        fd->get(_return, key);
    }

    void access(std::string& _return, const int64_t key, const int32_t len) {
        fd->access(_return, key, len);
    }

    int32_t get_num_keys() {
        return fd->num_keys();
    }

private:
    KVStoreShard *fd;
    bool construct;
    std::string filename;
    bool is_init;
    uint32_t num_keys;
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-p port] [file]\n", exec);
}

int main(int argc, char **argv) {

    if(argc < 2 || argc > 4) {
        print_usage(argv[0]);
        return -1;
    }

    fprintf(stderr, "Command line: ");
    for(int i = 0; i < argc; i++) {
        fprintf(stderr, "%s ", argv[i]);
    }
    fprintf(stderr, "\n");

    int c;
    uint32_t mode = 0, port = QUERY_SERVER_PORT;
    while((c = getopt(argc, argv, "p:")) != -1) {
        switch(c) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            mode = 0;
            port = QUERY_SERVER_PORT;
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string filename = std::string(argv[optind]);

    shared_ptr<QueryServiceHandler> handler(new QueryServiceHandler(filename));
    shared_ptr<TProcessor> processor(new QueryServiceProcessor(handler));

    try {
        shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
        shared_ptr<TBufferedTransportFactory> transport_factory(new TBufferedTransportFactory());
        shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
        TThreadedServer server(processor,
                         server_transport,
                         transport_factory,
                         protocol_factory);
        server.serve();
    } catch(std::exception& e) {
        fprintf(stderr, "Exception at SuccinctServer:main(): %s\n", e.what());
    }
    return 0;
}
