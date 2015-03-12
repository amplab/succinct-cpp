#include <thrift/AdaptiveSuccinctService.h>
#include <thrift/adaptive_constants.h>
#include <thrift/AdaptiveQueryService.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "thrift/ports.h"
#include "thrift/LoadBalancer.hpp"

#include "succinct/LayeredSuccinctShard.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <array>
#include <atomic>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class AdaptiveSuccinctServiceHandler : virtual public AdaptiveSuccinctServiceIf {
private:
    std::ifstream::pos_type filesize(std::string filename) {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        return in.tellg();
    }

public:
    AdaptiveSuccinctServiceHandler(std::string filename, uint32_t local_host_id,
            uint32_t num_shards, std::string qserver_exec,
            std::vector<std::string> hostnames, bool construct,
            uint32_t replication,
            bool standalone) {
        this->local_host_id = local_host_id;
        this->num_shards = num_shards;
        this->hostnames = hostnames;
        this->filename = filename;
        this->qserver_exec = qserver_exec;
        this->construct = construct;
        this->replication = replication;
        for(size_t i = 0; i < 8; i++) {
            queue_lengths[i] = 0;
        }
        if(standalone) {
            start_servers();
        }
    }

    int32_t start_servers() {
        return initialize(0);
    }

    int32_t initialize(const int32_t mode) {
        // Connect to query servers and start initialization
        for(uint32_t i = 0; i < num_shards; i++) {
            int port = QUERY_SERVER_PORT + i;
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            AdaptiveQueryServiceClient client(protocol);
            transport->open();
            fprintf(stderr, "Connected to QueryServer %u!\n", i);
            int32_t shard_id = i * hostnames.size() + local_host_id;
            int32_t status = client.init(shard_id);
            if(status == 0) {
                fprintf(stderr, "Initialization complete at QueryServer %u!\n", i);
                qservers.push_back(client);
                qserver_transports.push_back(transport);
            } else {
                fprintf(stderr, "Initialization failed at QueryServer %u!\n", i);
                return 1;
            }
        }
        fprintf(stderr, "Finished initialization at handler, connected to %d servers\n", qservers.size());
        return 0;
    }

    void get_request(const int64_t key) {
        uint32_t shard_id = (uint32_t)(key / LayeredSuccinctShard::MAX_KEYS);
        queue_lengths[shard_id]++;
        qservers.at(shard_id).send_get(key % LayeredSuccinctShard::MAX_KEYS);
    }

    void get_response(std::string& _return, const int64_t key) {
        uint32_t shard_id = (uint32_t)(key / LayeredSuccinctShard::MAX_KEYS);
        qservers.at(shard_id).recv_get(_return);
        queue_lengths[shard_id]--;
    }

    int32_t get_num_shards() {
        return num_shards;
    }

    int32_t get_num_keys(const int32_t shard_id) {
        return qservers.at(shard_id).get_num_keys();
    }

    int64_t get_queue_length(const int32_t shard_id) {
        return queue_lengths[shard_id];
    }

    int32_t connect_to_local_servers() {
        for(int i = 0; i < num_shards; i++) {
            fprintf(stderr, "Connecting to local server %d...", i);
            try {
                boost::shared_ptr<TSocket> socket(new TSocket("localhost", QUERY_SERVER_PORT + i));
                boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
                boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
                AdaptiveQueryServiceClient qsclient(protocol);
                transport->open();
                fprintf(stderr, "Connected!\n");
                qservers.push_back(qsclient);
                qserver_transports.push_back(transport);
                fprintf(stderr, "Pushed!\n");
            } catch(std::exception& e) {
                fprintf(stderr, "Could not connect to server...: %s\n", e.what());
                return 1;
            }
        }
        fprintf(stderr, "Currently have %d local server connections.\n", qservers.size());
        return 0;
    }

    int32_t disconnect_from_local_servers() {
        for(int i = 0; i < qservers.size(); i++) {
            try {
                fprintf(stderr, "Disconnecting from local server %d\n", i);
                qserver_transports[i]->close();
                fprintf(stderr, "Disconnected!\n");
            } catch(std::exception& e) {
                fprintf(stderr, "Could not close local connection: %s\n", e.what());
                return 1;
            }
        }
        qserver_transports.clear();
        qhandlers.clear();
        return 0;
    }

private:
    std::string filename;
    std::string qserver_exec;
    bool construct;
    std::vector<std::string> hostnames;
    std::vector<AdaptiveQueryServiceClient> qservers;
    std::vector<boost::shared_ptr<TTransport>> qserver_transports;
    std::vector<boost::shared_ptr<TTransport>> qhandler_transports;
    std::map<int, AdaptiveSuccinctServiceClient> qhandlers;
    uint32_t num_shards;
    uint32_t local_host_id;
    uint32_t replication;
    std::array<std::atomic<uint64_t>, 8> queue_lengths;
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [-s num_shards] [-r replication] [-q query_server_executible] [-h hostsfile] [-i hostid] file\n", exec);
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 15) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    bool standalone = false;
    uint32_t mode = 0, num_shards = 1, replication = 1;
    std::string qserver_exec = "./bin/qserver";
    std::string hostsfile = "./conf/hosts";
    uint32_t local_host_id = 0;

    while((c = getopt(argc, argv, "m:s:r:q:h:i:d")) != -1) {
        switch(c) {
        case 'm':
        {
            mode = atoi(optarg);
            break;
        }
        case 's':
        {
            num_shards = atoi(optarg);
            break;
        }
        case 'r':
        {
            replication = atoi(optarg);
            break;
        }
        case 'q':
        {
            qserver_exec = optarg;
            break;
        }
        case 'h':
        {
            hostsfile = optarg;
            break;
        }
        case 'i':
        {
            local_host_id = atoi(optarg);
            break;
        }
        case 'd':
        {
            standalone = true;
            break;
        }
        default:
        {
            mode = 0;
            num_shards = 1;
            qserver_exec = "./bin/qserver";
            hostsfile = "./conf/hosts";
            replication = 1;
            local_host_id = 0;
            standalone = false;
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

    int port = QUERY_HANDLER_PORT;
    shared_ptr<AdaptiveSuccinctServiceHandler> handler(new AdaptiveSuccinctServiceHandler(filename,
            local_host_id, num_shards, qserver_exec, hostnames, construct, replication, standalone));
    shared_ptr<TProcessor> processor(new AdaptiveSuccinctServiceProcessor(handler));

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

