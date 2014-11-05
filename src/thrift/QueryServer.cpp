#include "thrift/QueryService.h"
#include "thrift/succinct_constants.h"
#include "succinct/SuccinctFile.hpp"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

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
    }

    int32_t init() {
        fd = new SuccinctFile(filename, construct);
        if(construct) {
            std::ofstream s_file(filename + ".succinct", std::ofstream::binary);
            fd->serialize(s_file);
            s_file.close();
            remove(filename.c_str());
        }
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
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [file]\n", exec);
}

int main(int argc, char **argv) {

    if(argc < 2 || argc > 4) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    uint32_t mode = 0, port = QUERY_SERVER_PORT;
    while((c = getopt(argc, argv, "mp:")) != -1) {
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
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    // TODO: Change to non-blocking server
    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    server.serve();
    return 0;
}
