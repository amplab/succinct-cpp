#include <thrift/AdaptiveMasterService.h>
#include <thrift/AdaptiveSuccinctService.h>
#include <thrift/AdaptiveQueryService.h>
#include <thrift/adaptive_constants.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "thrift/ports.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <atomic>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class AdaptiveSuccinctMaster : virtual public AdaptiveMasterServiceIf {

public:
    AdaptiveSuccinctMaster(std::vector<std::string> hostnames) {
        this->hostnames = hostnames;

        std::vector<AdaptiveSuccinctServiceClient> clients;
        std::vector<boost::shared_ptr<TTransport> > transports;

        // Initiate client to client connections on all clients
        for(int i = 0; i < hostnames.size(); i++) {
            fprintf(stderr, "Connecting to client at %s...\n", hostnames[i].c_str());
            try {
                boost::shared_ptr<TSocket> socket(new TSocket(hostnames[i], QUERY_HANDLER_PORT));
                boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
                boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
                AdaptiveSuccinctServiceClient client(protocol);
                transport->open();
                fprintf(stderr, "Connected!\n");
                clients.push_back(client);
                transports.push_back(transport);
            } catch(std::exception& e) {
                fprintf(stderr, "Could not connect to handler on %s: %s\n", hostnames[i].c_str(), e.what());
                exit(1);
            }
        }

        // Start servers at each host
        for(int i = 0; i < hostnames.size(); i++) {
            try {
                fprintf(stderr, "Starting initialization at %s\n", hostnames[i].c_str());
                clients[i].send_start_servers();
            } catch(std::exception& e) {
                fprintf(stderr, "Could not send start_servers signal to %s: %s\n", hostnames[i].c_str(), e.what());
                exit(1);
            }
        }

        // Cleanup connections
        for(int i = 0; i < hostnames.size(); i++) {
            try {
                clients[i].recv_start_servers();
                fprintf(stderr, "Finished initialization at %s\n", hostnames[i].c_str());
            } catch(std::exception& e) {
                fprintf(stderr, "Could not recv start_servers signal to %s: %s\n", hostnames[i].c_str(), e.what());
                exit(1);
            }
            try {
                transports[i]->close();
                fprintf(stderr, "Closed connection!\n");
            } catch(std::exception& e) {
                fprintf(stderr, "Could not close connection to %s: %s\n", hostnames[i].c_str(), e.what());
                exit(1);
            }
        }

        fprintf(stderr, "Done!\n");
    }

    void get_client(std::string& _return) {
        _return = hostnames[rand() % hostnames.size()];
    }
    
    int32_t get_client_id() {
        int32_t ret = client_id;
        client_id++;
        return ret;
    }

private:
    std::vector<std::string> hostnames;
    std::atomic<uint32_t> client_id;
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-h hostsfile]\n", exec);
}

int main(int argc, char **argv) {
    if(argc < 1 || argc > 3) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    std::string hostsfile = "./conf/hosts";
    while((c = getopt(argc, argv, "h:")) != -1) {
        switch(c) {
        case 'h':
            hostsfile = optarg;
            break;
        default:
            hostsfile = "./conf/hosts";
        }
    }

    std::ifstream hosts(hostsfile);
    std::string host;
    std::vector<std::string> hostnames;
    while(std::getline(hosts, host, '\n')) {
        hostnames.push_back(host);
        fprintf(stderr, "Read host: %s\n", host.c_str());
    }

    int port = SUCCINCT_MASTER_PORT;
    shared_ptr<AdaptiveSuccinctMaster> handler(new AdaptiveSuccinctMaster(hostnames));
    shared_ptr<TProcessor> processor(new AdaptiveMasterServiceProcessor(handler));

    try {
        shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
        shared_ptr<TBufferedTransportFactory> transport_factory(new TBufferedTransportFactory());
        shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
        TThreadedServer server(processor,
                         server_transport,
                         transport_factory,
                         protocol_factory);
        fprintf(stderr, "Starting Master Daemon...\n");
        server.serve();
    } catch(std::exception& e) {
        fprintf(stderr, "Exception at SuccinctMaster:main(): %s\n", e.what());
    }

    return 0;
}
