#include "thrift/QueryService.h"
#include "thrift/succinct_constants.h"
#include "succinct/SuccinctFile.hpp"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include <cstdio>
#include <fstream>
#include <cstdint>

#include "thrift/ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class QueryServiceHandler : virtual public QueryServiceIf {
public:
    QueryServiceHandler(std::string filename, bool construct) {
        this->fd = NULL;
        this->construct = construct;
        this->filename = filename;
        this->is_init = false;
    }

    int32_t init() {
        fprintf(stderr, "Received INIT signal, initializing data structures...\n");
        fprintf(stderr, "Construct is set to %d\n", construct);

        fd = new SuccinctFile(filename, construct);
        if(construct) {
            fprintf(stderr, "Constructing data structures for file %s\n", filename.c_str());
            std::ofstream s_file(filename + ".succinct", std::ofstream::binary);
            fd->serialize(s_file);
            s_file.close();
        } else {
            fprintf(stderr, "Read data structures from file %s\n", filename.c_str());
        }
        fprintf(stderr, "Succinct data structures with original size = %llu\n", fd->original_size());
        fprintf(stderr, "Done initializing...\n");
        fprintf(stderr, "Waiting for queries...\n");
        is_init = true;
        return 0;
    }

    void extract(std::string& _return, const int64_t offset, const int64_t len) {
        fd->extract(_return, offset, len);
    }

    int64_t count(const std::string& query) {
        return fd->count(query);
    }

    void search(std::vector<int64_t> & _return, const std::string& query) {
        fd->search(_return, query);
    }

    void wildcard_search(std::vector<int64_t> & _return, const std::string& pattern, const int64_t max_sep) {
        fd->wildcard_search(_return, pattern, max_sep);
    }

private:
    SuccinctFile *fd;
    bool construct;
    std::string filename;
    bool is_init;
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [-p port] [file]\n", exec);
}

int main(int argc, char **argv) {

    if(argc < 2 || argc > 6) {
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
    while((c = getopt(argc, argv, "m:p:")) != -1) {
        switch(c) {
        case 'm':
            mode = atoi(optarg);
            break;
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
    bool construct = (mode == 0) ? true : false;

    shared_ptr<QueryServiceHandler> handler(new QueryServiceHandler(filename, construct));
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
