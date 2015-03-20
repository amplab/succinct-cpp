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
#include "thrift/DynamicLoadBalancer.hpp"

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
#include <thread>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class AdaptiveSuccinctServiceHandler : virtual public AdaptiveSuccinctServiceIf {
public:
    uint32_t MAX_CLIENTS = 1024;

    AdaptiveSuccinctServiceHandler(uint32_t local_host_id,
            uint32_t num_shards, std::vector<std::string> hostnames, bool construct,
            uint32_t replication, std::atomic<uint64_t> *local_queue_lengths,
            std::atomic<uint64_t> *global_queue_lengths) {
        this->local_host_id = local_host_id;
        this->num_shards = num_shards;
        this->num_hosts = hostnames.size();
        this->hostnames = hostnames;
        this->construct = construct;
        this->replication = replication;
        this->local_queue_lengths = local_queue_lengths;
        this->global_queue_lengths = global_queue_lengths;

        this->qservers = new std::vector<AdaptiveQueryServiceClient *>[MAX_CLIENTS];
        this->qserver_transports = new std::vector<boost::shared_ptr<TTransport>>[MAX_CLIENTS];
        this->qhandlers = new std::map<uint32_t, AdaptiveSuccinctServiceClient*>[MAX_CLIENTS];
        this->qhandler_transports = new std::map<uint32_t, boost::shared_ptr<TTransport>>[MAX_CLIENTS];
    }

    int32_t initialize(const int32_t mode) {
        int res = start_servers();
        return res;
    }

    int32_t start_servers() {
        // Connect to query servers and start initialization
        for(uint32_t local_shard_id = 0; local_shard_id < num_shards; local_shard_id++) {
            int port = QUERY_SERVER_PORT + local_shard_id;
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            AdaptiveQueryServiceClient *client = new AdaptiveQueryServiceClient(protocol);
            transport->open();
            fprintf(stderr, "Connected to QueryServer %u!\n", local_shard_id);
            uint32_t global_shard_id = (local_shard_id * num_hosts + local_host_id) / replication;
            int32_t status = client->init(global_shard_id);
            transport->close();
            if(status == 0) {
                fprintf(stderr, "Started QueryServer %u!\n", global_shard_id);
            } else {
                fprintf(stderr, "Could not start QueryServer %u!\n", global_shard_id);
                return 1;
            }
        }
        fprintf(stderr, "Started all QueryServers!\n");
        return 0;
    }

    int32_t connect_to_handlers(const int32_t client_id) {
        // Create connections to all Succinct Clients
        for(int i = 0; i < num_hosts; i++) {
            fprintf(stderr, "Connecting to %s:%d...\n", hostnames[i].c_str(), QUERY_HANDLER_PORT);
            try {
                if(i == local_host_id) {
                    fprintf(stderr, "Self setup:\n");
                    connect_to_local_servers(client_id);
                } else {
                    boost::shared_ptr<TSocket> socket(new TSocket(hostnames[i], QUERY_HANDLER_PORT));
                    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
                    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
                    AdaptiveSuccinctServiceClient *client = new AdaptiveSuccinctServiceClient(protocol);
                    transport->open();
                    fprintf(stderr, "Connected!\n");
                    qhandler_transports[client_id].insert(std::pair<uint32_t, boost::shared_ptr<TTransport>>(i, transport));
                    qhandlers[client_id].insert(std::pair<uint32_t, AdaptiveSuccinctServiceClient*>(i, client));
                    fprintf(stderr, "Asking %s to connect to local servers...\n", hostnames[i].c_str());
                    client->connect_to_local_servers(client_id);
                }
            } catch(std::exception& e) {
                fprintf(stderr, "Client is not up...: %s\n ", e.what());
                return 1;
            }
        }
        fprintf(stderr, "Currently have %d connections.\n", qhandlers[client_id].size());
        return 0;
    }

    int32_t connect_to_local_servers(const int32_t client_id) {
        for(int i = 0; i < num_shards; i++) {
            fprintf(stderr, "Connecting to local server %d...", i);
            try {
                boost::shared_ptr<TSocket> socket(new TSocket("localhost", QUERY_SERVER_PORT + i));
                boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
                boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
                AdaptiveQueryServiceClient *qsclient = new AdaptiveQueryServiceClient(protocol);
                transport->open();
                fprintf(stderr, "Connected!\n");
                qservers[client_id].push_back(qsclient);
                qserver_transports[client_id].push_back(transport);
                fprintf(stderr, "Pushed!\n");
            } catch(std::exception& e) {
                fprintf(stderr, "Could not connect to server...: %s\n", e.what());
                return 1;
            }
        }
        fprintf(stderr, "Currently have %d local server connections.\n", qservers[client_id].size());
        return 0;
    }

    int32_t disconnect_from_handlers(const int32_t client_id) {
        // Destroy connections to all Succinct Handlers
        for(int i = 0; i < num_hosts; i++) {
            try {
                if(i == local_host_id) {
                    fprintf(stderr, "Killing local server connections...\n");
                    disconnect_from_local_servers(client_id);
                } else {
                    fprintf(stderr, "Asking client %s:%d to kill local server connections...\n", hostnames[i].c_str(), QUERY_HANDLER_PORT);
                    qhandlers[client_id].at(i)->disconnect_from_local_servers(client_id);
                    fprintf(stderr, "Closing connection to %s:%d...", hostnames[i].c_str(), QUERY_HANDLER_PORT);
                    qhandler_transports[client_id].at(i)->close();
                    fprintf(stderr, "Closed!\n");
                }
            } catch(std::exception& e) {
                fprintf(stderr, "Could not close connection: %s\n", e.what());
                return 1;
            }
        }
        qhandler_transports[client_id].clear();
        qhandlers[client_id].clear();

        return 0;
    }

    int32_t disconnect_from_local_servers(const int32_t client_id) {
        for(int i = 0; i < qservers[client_id].size(); i++) {
            try {
                fprintf(stderr, "Disconnecting from local server %d\n", i);
                qserver_transports[client_id][i]->close();
                fprintf(stderr, "Disconnected!\n");
            } catch(std::exception& e) {
                fprintf(stderr, "Could not close local connection: %s\n", e.what());
                return 1;
            }
        }
        qserver_transports[client_id].clear();
        qhandlers[client_id].clear();
        return 0;
    }

    int32_t get_request(const int32_t client_id, const int64_t key) {
        uint32_t primary_shard_id = (uint32_t)(key / LayeredSuccinctShard::MAX_KEYS);
        uint32_t replica_id = (primary_shard_id * replication);   // TODO: Add load balancer code; right now all queries go to primary only
        uint32_t host_id = replica_id % num_hosts;
        uint32_t local_shard_id =  replica_id / num_hosts;

        int64_t queue_length;
        if(host_id == local_host_id)
            queue_length = get_request_local(client_id, local_shard_id, key % LayeredSuccinctShard::MAX_KEYS);
        else
            queue_length = qhandlers[client_id].at(host_id)->get_request_local(client_id, local_shard_id, key % LayeredSuccinctShard::MAX_KEYS);

        global_queue_lengths[replica_id] = queue_length;

        return replica_id;
    }

    int64_t get_request_local(const int32_t client_id, const int32_t local_shard_id, const int64_t key) {
        local_queue_lengths[local_shard_id]++;
        qservers[client_id].at(local_shard_id)->send_get(key);
        return local_queue_lengths[local_shard_id];
    }

    void get_response(std::string& _return, const int32_t client_id, const int32_t replica_id) {
        uint32_t host_id = replica_id % num_hosts;
        uint32_t local_shard_id =  replica_id / num_hosts;

        if(host_id == local_host_id)
            get_response_local(_return, client_id, local_shard_id);
        else
            qhandlers[client_id].at(host_id)->get_response_local(_return, client_id, local_shard_id);
    }

    void get_response_local(std::string& _return, const int32_t client_id, const int32_t local_shard_id) {
        qservers[client_id].at(local_shard_id)->recv_get(_return);
        local_queue_lengths[local_shard_id]--;
    }

    int64_t remove_layer(const int32_t shard_id, const int32_t layer_id) {
        int port = QUERY_SERVER_PORT + shard_id;
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        AdaptiveQueryServiceClient client(protocol);
        transport->open();
        int64_t size = client.remove_layer(layer_id);
        transport->close();
        return size;
    }

    int64_t reconstruct_layer(const int32_t shard_id, const int32_t layer_id) {
        int port = QUERY_SERVER_PORT + shard_id;
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        AdaptiveQueryServiceClient client(protocol);
        transport->open();
        int64_t size = client.reconstruct_layer(layer_id);
        transport->close();
        return size;
    }

    int64_t storage_size(const int32_t shard_id) {
        int port = QUERY_SERVER_PORT + shard_id;
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        AdaptiveQueryServiceClient client(protocol);
        transport->open();
        int64_t size = client.storage_size();
        transport->close();
        return size;
    }

    int32_t get_num_shards() {
        return num_shards;
    }

    int32_t get_num_keys(const int32_t shard_id) {
        uint32_t replica_id = (shard_id * replication);
        uint32_t host_id = replica_id % num_hosts;
        uint32_t local_shard_id =  replica_id / num_hosts;
        int32_t size = 0;
        if(host_id == local_host_id) {
            int port = QUERY_SERVER_PORT + local_shard_id;
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            AdaptiveQueryServiceClient client(protocol);
            transport->open();
            size = client.get_num_keys();
            transport->close();
        } else {
            int port = QUERY_HANDLER_PORT;
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            AdaptiveSuccinctServiceClient client(protocol);
            transport->open();
            size = client.get_num_keys(shard_id);
            transport->close();
        }
        return size;
    }

    int64_t get_queue_length(const int32_t shard_id) {
        return local_queue_lengths[shard_id];
    }

private:
    bool construct;
    std::vector<std::string> hostnames;
    std::vector<AdaptiveQueryServiceClient*> *qservers;
    std::vector<boost::shared_ptr<TTransport>> *qserver_transports;
    std::map<uint32_t, AdaptiveSuccinctServiceClient*> *qhandlers;
    std::map<uint32_t, boost::shared_ptr<TTransport>> *qhandler_transports;
    uint32_t num_shards;
    uint32_t num_hosts;
    uint32_t local_host_id;
    uint32_t replication;
    std::atomic<uint64_t> *local_queue_lengths;
    std::atomic<uint64_t> *global_queue_lengths;
};

/*
class AdaptiveHandlerProcessorFactory : public TProcessorFactory {
public:
    AdaptiveHandlerProcessorFactory(uint32_t local_host_id,
            uint32_t num_shards, std::vector<std::string> hostnames, bool construct,
            uint32_t replication, std::atomic<uint64_t> *local_queue_lengths,
            std::atomic<uint64_t> *global_queue_lengths) {
        this->local_host_id = local_host_id;
        this->num_shards = num_shards;
        this->hostnames = hostnames;
        this->construct = construct;
        this->distribution = distribution;
        this->replication = replication;
        this->local_queue_lengths = local_queue_lengths;
        this->global_queue_lengths = global_queue_lengths;
    }

    boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
        boost::shared_ptr<AdaptiveSuccinctServiceHandler> handler(new AdaptiveSuccinctServiceHandler(local_host_id,
                num_shards, hostnames, construct, replication, local_queue_lengths, global_queue_lengths));
        boost::shared_ptr<TProcessor> handlerProcessor(new AdaptiveSuccinctServiceProcessor(handler));
        return handlerProcessor;
    }

private:
    std::vector<std::string> hostnames;
    uint32_t local_host_id;
    uint32_t num_shards;
    std::vector<double> distribution;
    std::vector<uint32_t> sampling_rates;
    bool construct;
    uint32_t replication;
    std::atomic<uint64_t> *local_queue_lengths;
    std::atomic<uint64_t> *global_queue_lengths;
};
*/

typedef unsigned long long int timestamp_t;

static timestamp_t get_timestamp() {
    struct timeval now;
    gettimeofday (&now, NULL);

    return  now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}

static void monitor_queues_local(std::atomic<uint64_t> *local_queue_lengths, uint64_t num_local_shards) {

    /*
     * Connect to Query Handler; this would give the local queue monitor daemon it's own
     * thread at the Query Handler, which would not interfere with the query threads
     * at the QueryHandler.
     *
     */

    double alpha = 0.9;
    uint32_t delta = 5;

    sleep(delta);
    std::ofstream output("local_queue_lengths.out");

    int port = QUERY_HANDLER_PORT;
    boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    AdaptiveSuccinctServiceClient *client = new AdaptiveSuccinctServiceClient(protocol);
    transport->open();

    double *avg_qlens = new double[num_local_shards];
    for(uint32_t i = 0; i < num_local_shards; i++) {
        avg_qlens[i] = 0.0;
    }

    while(true) {
        output << get_timestamp();
        for(uint32_t i = 0; i < num_local_shards; i++) {
            avg_qlens[i] = local_queue_lengths[i] * alpha + (1.0 - alpha) * avg_qlens[i];
            output << "\t" << avg_qlens[i];
        }
        output << "\n";
        sleep(delta);
    }
}

static void monitor_queues_global(std::atomic<uint64_t> *global_queue_lengths, uint64_t num_global_shards) {

    /*
     * Connect to Query Handler; this would give the global queue monitor daemon it's own
     * thread at the Query Handler, which would not interfere with the query threads
     * at the QueryHandler.
     *
     */

    uint32_t delta = 5;

    sleep(delta);
    std::ofstream output("global_queue_lengths.out");

    int port = QUERY_HANDLER_PORT;
    boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    AdaptiveSuccinctServiceClient *client = new AdaptiveSuccinctServiceClient(protocol);
    transport->open();


    while(true) {
        output << get_timestamp();
        for(uint32_t i = 0; i < num_global_shards; i++) {
            uint64_t queue_len = global_queue_lengths[i];
            output << "\t" << queue_len;
        }
        output << "\n";
        sleep(delta);
    }
}

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [-s num_local_shards] [-p num_primary_shards] [-r replication] [-h hostsfile] [-i hostid]\n", exec);
}

int main(int argc, char **argv) {
    if(argc > 13) {
        print_usage(argv[0]);
        return -1;
    }

    fprintf(stderr, "Command line: ");
    for(int i = 0; i < argc; i++) {
        fprintf(stderr, "%s ", argv[i]);
    }
    fprintf(stderr, "\n");

    int c;
    uint32_t mode = 0, num_local_shards = 1, replication = 1, num_primary_shards = 1;
    std::string hostsfile = "./conf/hosts";
    uint32_t local_host_id = 0;

    while((c = getopt(argc, argv, "m:s:r:h:i:p:")) != -1) {
        switch(c) {
        case 'm':
        {
            mode = atoi(optarg);
            break;
        }
        case 's':
        {
            num_local_shards = atoi(optarg);
            break;
        }
        case 'p':
        {
            num_primary_shards = atoi(optarg);
            break;
        }
        case 'r':
        {
            replication = atoi(optarg);
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
        default:
        {
            mode = 0;
            num_local_shards = 1;
            num_primary_shards = 1;
            hostsfile = "./conf/hosts";
            replication = 1;
            local_host_id = 0;
        }
        }
    }

    bool construct = (mode == 0) ? true : false;

    std::ifstream hosts(hostsfile);
    std::string host;
    std::vector<std::string> hostnames;
    while(std::getline(hosts, host, '\n')) {
        hostnames.push_back(host);
    }

    std::atomic<uint64_t> *local_queue_lengths = new std::atomic<uint64_t>[num_local_shards];
    for(uint32_t i = 0; i < num_local_shards; i++) {
        local_queue_lengths[i] = 0;
    }

    std::atomic<uint64_t> *global_queue_lengths = new std::atomic<uint64_t>[num_primary_shards * replication];
    for(uint32_t i = 0; i < num_primary_shards; i++) {
        global_queue_lengths[i] = 0;
    }

    DynamicLoadBalancer *lb = new DynamicLoadBalancer(num_primary_shards, replication);

    int port = QUERY_HANDLER_PORT;
//    shared_ptr<AdaptiveHandlerProcessorFactory> handlerFactory(new AdaptiveHandlerProcessorFactory(local_host_id,
//            num_local_shards, hostnames, construct, replication, local_queue_lengths, global_queue_lengths));
    shared_ptr<AdaptiveSuccinctServiceHandler> handler(new AdaptiveSuccinctServiceHandler(local_host_id,
            num_local_shards, hostnames, construct, replication, local_queue_lengths, global_queue_lengths));
    shared_ptr<TProcessor> processor(new AdaptiveSuccinctServiceProcessor(handler));

    std::thread local_queue_monitor_daemon(monitor_queues_local, local_queue_lengths, num_local_shards);
    std::thread global_queue_monitor_daemon(monitor_queues_global, global_queue_lengths, num_primary_shards * replication);
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

    if(local_queue_monitor_daemon.joinable())
        local_queue_monitor_daemon.join();

    return 0;
}
