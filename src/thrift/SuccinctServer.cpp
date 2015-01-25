#include <thrift/SuccinctService.h>
#include <thrift/succinct_constants.h>
#include <thrift/QueryService.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "thrift/ports.h"
#include "thrift/LoadBalancer.h"

#include "succinct/KVStoreShard.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class SuccinctServiceHandler : virtual public SuccinctServiceIf {
private:
    std::ifstream::pos_type filesize(std::string filename) {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        return in.tellg();
    }

public:
    SuccinctServiceHandler(std::string filename, uint32_t local_host_id,
            uint32_t num_shards, std::vector<std::string> hostnames, std::vector<double> distribution,
            uint32_t num_failures) {
        this->local_host_id = local_host_id;
        this->num_shards = num_shards;
        this->hostnames = hostnames;
        this->filename = filename;
        this->balancer = new LoadBalancer(distribution);
        this->num_failures = num_failures;
    }

    int32_t start_servers() {
        return initialize(0);
    }

    int32_t initialize(const int32_t mode) {
        // Connect to query servers and start initialization
        std::ifstream input(filename);
        uint32_t num_keys = std::count(std::istreambuf_iterator<char>(input),
                        std::istreambuf_iterator<char>(), '\n');
        for(uint32_t i = 0; i < num_shards; i++) {
            uint32_t id = i * hostnames.size() + local_host_id;
            shards.push_back(new KVStoreShard(id, filename, num_keys));
        }

        return 0;
    }

    void get(std::string& _return, const int64_t key) {
        uint32_t shard_id = (uint32_t)((key / KVStoreShard::MAX_KEYS) * balancer->num_replicas()) + balancer->get_replica();
        uint32_t host_id = shard_id % hostnames.size();
        // Currently only supports single failure emulation
        if(host_id < num_failures) {
            // Get new replica#
            uint32_t replica_num = (shard_id % balancer->num_replicas() == 0) ? 1 : 0;
            shard_id = (uint32_t)((key / KVStoreShard::MAX_KEYS) * balancer->num_replicas()) + replica_num;
            assert(shard_id % hostnames.size() != host_id);
            host_id = shard_id % hostnames.size();
        }
        uint32_t qserver_id = shard_id / hostnames.size();
        if(host_id == local_host_id) {
            get_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS);
        } else {
            qhandlers.at(host_id).get_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS);
        }
    }

    void get_local(std::string& _return, const int32_t qserver_id, const int64_t key) {
        shards.at(qserver_id)->get(_return, key);
    }

    int32_t get_num_hosts() {
        return hostnames.size();
    }

    int32_t get_num_shards(const int32_t host_id) {
        int32_t num;
        if(host_id == local_host_id) {
            return num_shards;
        }
        return qhandlers.at(host_id).get_num_shards(host_id);
    }

    int32_t get_num_keys(const int32_t shard_id) {
        int32_t host_id = shard_id % hostnames.size();
        int32_t num;
        if(host_id == local_host_id) {
            return shards.at(shard_id / hostnames.size())->num_keys();
        }
        return qhandlers.at(host_id).get_num_keys(shard_id);
    }

    int32_t connect_to_handlers() {
        // Create connections to all Succinct Clients
        for(int i = 0; i < hostnames.size(); i++) {
            fprintf(stderr, "Connecting to %s:%d...\n", hostnames[i].c_str(), QUERY_HANDLER_PORT);
            try {
                if(i == local_host_id) {
                    fprintf(stderr, "Self setup:\n");
                    connect_to_local_servers();
                } else {
                    boost::shared_ptr<TSocket> socket(new TSocket(hostnames[i], QUERY_HANDLER_PORT));
                    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
                    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
                    SuccinctServiceClient client(protocol);
                    transport->open();
                    fprintf(stderr, "Connected!\n");
                    qhandler_transports.push_back(transport);
                    qhandlers.insert(std::pair<int, SuccinctServiceClient>(i, client));
                    fprintf(stderr, "Asking %s to connect to local servers...\n", hostnames[i].c_str());
                    client.connect_to_local_servers();
                }
            } catch(std::exception& e) {
                fprintf(stderr, "Client is not up...: %s\n ", e.what());
                return 1;
            }
        }
        fprintf(stderr, "Currently have %d connections.\n", qhandlers.size());
        return 0;
    }

    int32_t connect_to_local_servers() {
        return 0;
    }

    int32_t disconnect_from_handlers() {
        // Destroy connections to all Succinct Handlers
        for(int i = 0; i < hostnames.size(); i++) {
            try {
                if(i == local_host_id) {
                    fprintf(stderr, "Killing local server connections...\n");
                    disconnect_from_local_servers();
                } else {
                    fprintf(stderr, "Asking client %s:%d to kill local server connections...\n", hostnames[i].c_str(), QUERY_HANDLER_PORT);
                    qhandlers.at(i).disconnect_from_local_servers();
                    fprintf(stderr, "Closing connection to %s:%d...", hostnames[i].c_str(), QUERY_HANDLER_PORT);
                    qhandler_transports[i]->close();
                    fprintf(stderr, "Closed!\n");
                }
            } catch(std::exception& e) {
                fprintf(stderr, "Could not close connection: %s\n", e.what());
                return 1;
            }
        }
        qhandler_transports.clear();
        qhandlers.clear();

        return 0;
    }

    int32_t disconnect_from_local_servers() {
        return 0;
    }

private:
    std::string filename;
    std::vector<std::string> hostnames;
    std::vector<KVStoreShard *> shards;
    std::vector<boost::shared_ptr<TTransport> > qhandler_transports;
    std::map<int, SuccinctServiceClient> qhandlers;
    uint32_t num_shards;
    uint32_t local_host_id;
    uint32_t num_failures;
    LoadBalancer *balancer;
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-s num_shards] [-r replfile] [-h hostsfile] [-i hostid] [-f num_failures] file\n", exec);
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 14) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    uint32_t mode = 0, num_shards = 1;
    std::string hostsfile = "./conf/hosts";
    std::string replfile = "./conf/repl";
    uint32_t local_host_id = 0;
    uint32_t num_failures = 0;

    while((c = getopt(argc, argv, "s:h:r:i:f:")) != -1) {
        switch(c) {
        case 's':
        {
            num_shards = atoi(optarg);
            break;
        }
        case 'h':
        {
            hostsfile = optarg;
            break;
        }
        case 'r':
        {
            replfile = optarg;
            break;
        }
        case 'i':
        {
            local_host_id = atoi(optarg);
            break;
        }
        case 'f':
        {
            num_failures = atoi(optarg);
            break;
        }
        default:
        {
            mode = 0;
            num_shards = 1;
            hostsfile = "./conf/hosts";
            replfile = "./conf/repl";
            local_host_id = 0;
            num_failures = 0;
        }
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string filename = std::string(argv[optind]);
    bool construct = (mode == 0) ? true : false;

    std::ifstream hosts(hostsfile);
    std::string host;
    std::vector<std::string> hostnames;
    while(std::getline(hosts, host, '\n')) {
        hostnames.push_back(host);
    }

    std::ifstream repl(replfile);
    std::string dist;
    std::vector<double> distribution;
    while(std::getline(repl, dist, '\n')) {
        distribution.push_back(atof(dist.c_str()));
    }

    int port = QUERY_HANDLER_PORT;
    try {
        shared_ptr<SuccinctServiceHandler> handler(new SuccinctServiceHandler(filename,
                local_host_id, num_shards, hostnames, distribution, num_failures));
        shared_ptr<TProcessor> processor(new SuccinctServiceProcessor(handler));
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

