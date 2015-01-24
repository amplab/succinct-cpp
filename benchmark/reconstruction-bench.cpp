#include <iostream>
#include <fstream>
#include <unistd.h>

#include "../include/succinct/KVStoreShard.hpp"

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "thrift/MasterService.h"
#include "thrift/ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s bench-type\n", exec);
}

static time_t get_timestamp() {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_usec + (time_t) now.tv_sec * 1000000;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        print_usage(argv[0]);
        return -1;
    }

    std::string benchmark_type = std::string(argv[1]);

    try {
        // Connect to master
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", SUCCINCT_MASTER_PORT));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        MasterServiceClient *master = new MasterServiceClient(protocol);

        transport->open();

        if(benchmark_type == "once") {
            time_t start, end, diff;
            start = get_timestamp();
            master->reconstruct(10);
            end = get_timestamp();
            diff = end - start;
            fprintf(stderr, "%ld", diff);
        } else if(benchmark_type == "repeat") {
            while(true) {
                master->reconstruct(10);
            }
        } else {
            // Not supported yet
            assert(0);
        }
    } catch(std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }

    return 0;
}
