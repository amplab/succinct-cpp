#include <iostream>
#include <fstream>
#include <unistd.h>

#include <sys/time.h>

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "thrift/MasterService.h"
#include "thrift/ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s\n", exec);
}

int main(int argc, char **argv) {
    if(argc != 1) {
        print_usage(argv[0]);
        return -1;
    }

    try {
        fprintf(stderr, "Connecting to master...\n");
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", SUCCINCT_MASTER_PORT));
        boost::shared_ptr<TTransport> transport(
                new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(
                new TBinaryProtocol(transport));
        MasterServiceClient *master = new MasterServiceClient(protocol);
        transport->open();
        fprintf(stderr, "Connected!\n");

        master->reconstruct();
    } catch(std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }

    return 0;
}




