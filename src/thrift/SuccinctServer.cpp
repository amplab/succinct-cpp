#include <thrift/SuccinctService.h>
#include <thrift/succinct_constants.h>
#include <thrift/QueryService.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "thrift/ports.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

#define SPLITSIZE(n, s)     (((n) % (s) == 0) ? ((n) / (s)) : (((n) / (s)) + 1))

class SuccinctServiceHandler : virtual public SuccinctServiceIf {
private:
    std::ifstream::pos_type filesize(std::string filename) {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        return in.tellg();
    }

public:
    SuccinctServiceHandler(std::string filename, uint32_t splits, std::string qserver_exec, bool construct) {
        this->splits = splits;

        // Compute split size
        size_t fsize = filesize(filename);
        this->split_size = SPLITSIZE(fsize, splits);

        assert(this->split_size * this->splits >= fsize);

        std::string dot = ".";
        if(construct) {
            // Create splits
            std::ifstream input(filename, std::ifstream::binary);
            char *buf = new char[fsize];
            input.read(buf, fsize);
            for(uint32_t i = 0; i < splits; i++) {
                std::string splitfile = dot + filename + dot + std::to_string(i);
                std::ofstream output(splitfile, std::ofstream::binary);
                size_t to_write = (i == (splits - 1)) ? fsize - i * split_size : split_size;
                output.write(buf + i * split_size, to_write);
                output.close();
            }
            delete [] buf;
            input.close();
        }

        // Start QueryServer(s)
        for(uint32_t i = 0; i < splits; i++) {
            std::string split_file = dot + filename + dot + std::to_string(i);
            std::string log_path = split_file + dot + std::string("log");
            std::string start_cmd = std::string("nohup ") + qserver_exec +
                                    std::string(" -m ") +
                                    std::to_string((int)(!construct)) +
                                    std::string(" -p ") +
                                    std::to_string(QUERY_SERVER_PORT + i) +
                                    split_file + std::string(" 2>&1 > ") +
                                    log_path +
                                    std::string("&");
            fprintf(stderr, "Launching QueryServer: [%s]\n", start_cmd.c_str());
            system(start_cmd.c_str());
        }

        // Connect to query servers and start initialization
        for(uint32_t i = 0; i < splits; i++) {
            int port = QUERY_SERVER_PORT + i;
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            QueryServiceClient client(protocol);
            transport->open();
            std::cout << "Connected!" << std::endl;
            qservers.push_back(client);
            qtransports.push_back(transport);
            std::cout << "Pushed!" << std::endl;
        }

    }

    void extract(std::string& _return, const int64_t offset, const int64_t len) {
        uint32_t qserver_id = offset / split_size;
        qservers.at(qserver_id).extract(_return, offset % split_size, len);
    }

    int64_t count(const std::string& query) override {
        for(size_t i = 0; i < qservers.size(); i++) {
            qservers.at(i).send_count(query);
        }

        uint64_t count = 0;
        for(size_t i = 0; i < qservers.size(); i++) {
            count += qservers.at(i).recv_count();
        }
        return count;
    }

    void search(std::vector<int64_t> & _return, const std::string& query) {
        for(size_t i = 0; i < qservers.size(); i++) {
            qservers.at(i).send_search(query);
        }

        for(size_t i = 0; i < qservers.size(); i++) {
            std::vector<int64_t> locs;
            qservers.at(i).recv_search(locs);
            for(uint64_t i = 0; i < locs.size(); i++) {
                _return.push_back(locs[i] + split_size * i);
            }
        }
    }

    void wildcard_search(std::vector<int64_t> & _return, const std::string& pattern, const int64_t max_sep) {
        for(size_t i = 0; i < qservers.size(); i++) {
            qservers.at(i).send_wildcard_search(pattern, max_sep);
        }

        for(size_t i = 0; i < qservers.size(); i++) {
            std::vector<int64_t> locs;
            qservers.at(i).recv_wildcard_search(locs);
            for(uint64_t i = 0; i < locs.size(); i++) {
                _return.push_back(locs[i] + split_size * i);
            }
        }
    }

private:
    std::vector<QueryServiceClient> qservers;
    std::vector<boost::shared_ptr<TTransport>> qtransports;
    uint32_t splits;
    uint64_t split_size;
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [-s num_splits] [-q query_server_executible] [file]\n", exec);
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 4) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    uint32_t mode = 0, splits = 1;
    std::string qserver_exec = "./bin/qserver";
    while((c = getopt(argc, argv, "msq:")) != -1) {
        switch(c) {
        case 'm':
            mode = atoi(optarg);
            break;
        case 's':
            splits = atoi(optarg);
            break;
        case 'q':
            qserver_exec = optarg;
            break;
        default:
            mode = 0;
            splits = 1;
            qserver_exec = "./bin/qserver";
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string filename = std::string(argv[optind]);
    bool construct = (mode == 0) ? true : false;

    int port = SUCCINCT_SERVER_PORT;
    shared_ptr<SuccinctServiceHandler> handler(new SuccinctServiceHandler(filename, splits, qserver_exec, construct));
    shared_ptr<TProcessor> processor(new SuccinctServiceProcessor(handler));

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

