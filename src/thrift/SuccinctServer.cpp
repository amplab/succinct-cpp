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
            uint32_t num_shards, std::string qserver_exec,
            std::vector<std::string> hostnames,
            uint32_t num_failures) {
        this->local_host_id = local_host_id;
        this->num_shards = num_shards;
        this->hostnames = hostnames;
        this->filename = filename;
        this->qserver_exec = qserver_exec;
        this->num_failures = num_failures;
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
            QueryServiceClient client(protocol);
            transport->open();
            fprintf(stderr, "Connected to QueryServer %u!\n ", i);
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

        return 0;
    }

    void get(std::string& _return, const int64_t key) {

        uint32_t shard_id = (uint32_t)(key / KVStoreShard::MAX_KEYS);
        uint32_t host_id = shard_id % hostnames.size();
        uint32_t qserver_id = shard_id / hostnames.size();

        // Currently only supports single failure emulation
        if(host_id < num_failures) {
            // Request parity data from the 4 parity machines
            for(size_t i = 10; i < 14; i++) {
                if(i == local_host_id) {
                    qservers.at(qserver_id).get(_return, key % KVStoreShard::MAX_KEYS);
                } else {
                    qhandlers.at(i).get_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS);
                }
            }

            // Request non-parity data from 6 random machines
            for(size_t i = key % 9; i < key % 9 + 6; i++) {
                size_t j = i % 9 + 1;
                if(j == local_host_id) {
                    qservers.at(qserver_id).get(_return, key % KVStoreShard::MAX_KEYS);
                } else {
                    qhandlers.at(j).get_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS);
                }
            }
        }

        if(host_id == local_host_id) {
            get_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS);
        } else {
            qhandlers.at(host_id).get_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS);
        }
    }

    void get_local(std::string& _return, const int32_t qserver_id, const int64_t key) {
        qservers.at(qserver_id).get(_return, key);
    }

    void access(std::string& _return, const int64_t key, const int32_t len) {

        uint32_t shard_id = (uint32_t)(key / KVStoreShard::MAX_KEYS);
        uint32_t host_id = shard_id % hostnames.size();
        uint32_t qserver_id = shard_id / hostnames.size();

        // Currently only supports single failure emulation
        if(host_id < num_failures) {
            // Request parity data from the 4 parity machines
            for(size_t i = 10; i < 14; i++) {
                if(i == local_host_id) {
                    qservers.at(qserver_id).access(_return, key % KVStoreShard::MAX_KEYS, len);
                } else {
                    qhandlers.at(i).access_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS, len);
                }
            }

            // Request non-parity data from 6 random machines
            for(size_t i = key % 9; i < key % 9 + 6; i++) {
                size_t j = i % 9 + 1;
                if(j == local_host_id) {
                    qservers.at(qserver_id).access(_return, key % KVStoreShard::MAX_KEYS, len);
                } else {
                    qhandlers.at(j).access_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS, len);
                }
            }
        }

        if(host_id == local_host_id) {
            access_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS, len);
        } else {
            qhandlers.at(host_id).access_local(_return, qserver_id, key % KVStoreShard::MAX_KEYS, len);
        }
    }

    void access_local(std::string& _return, const int32_t qserver_id, const int64_t key, const int32_t len) {
        qservers.at(qserver_id).access(_return, key, len);
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
            return qservers.at(shard_id / hostnames.size()).get_num_keys();
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
        for(int i = 0; i < num_shards; i++) {
            fprintf(stderr, "Connecting to local server %d...", i);
            try {
                boost::shared_ptr<TSocket> socket(new TSocket("localhost", QUERY_SERVER_PORT + i));
                boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
                boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
                QueryServiceClient qsclient(protocol);
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
    std::vector<std::string> hostnames;
    std::vector<QueryServiceClient> qservers;
    std::vector<boost::shared_ptr<TTransport>> qserver_transports;
    std::vector<boost::shared_ptr<TTransport> > qhandler_transports;
    std::map<int, SuccinctServiceClient> qhandlers;
    uint32_t num_shards;
    uint32_t local_host_id;
    uint32_t num_failures;
};

class HandlerProcessorFactory : public TProcessorFactory {
public:
    HandlerProcessorFactory(std::string filename, uint32_t local_host_id,
            uint32_t num_shards, std::string qserver_exec,
            std::vector<std::string> hostnames,
            uint32_t num_failures) {
        this->filename = filename;
        this->local_host_id = local_host_id;
        this->num_shards = num_shards;
        this->qserver_exec = qserver_exec;
        this->hostnames = hostnames;
        this->num_failures = num_failures;
    }

    boost::shared_ptr<TProcessor> getProcessor(const TConnectionInfo&) {
        boost::shared_ptr<SuccinctServiceHandler> handler(new SuccinctServiceHandler(filename,
                local_host_id, num_shards, qserver_exec, hostnames, num_failures));
        boost::shared_ptr<TProcessor> handlerProcessor(new SuccinctServiceProcessor(handler));
        return handlerProcessor;
    }

private:
    std::vector<std::string> hostnames;
    std::string filename;
    std::string qserver_exec;
    uint32_t local_host_id;
    uint32_t num_shards;
    uint32_t num_failures;
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-s num_shards] [-q query_server_executible] [-h hostsfile] [-i hostid] [-f num_failures] file\n", exec);
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 12) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    uint32_t mode = 0, num_shards = 1;
    std::string qserver_exec = "./bin/qserver";
    std::string hostsfile = "./conf/hosts";
    uint32_t local_host_id = 0;
    uint32_t num_failures = 0;

    while((c = getopt(argc, argv, "s:q:h:i:f:")) != -1) {
        switch(c) {
        case 's':
        {
            num_shards = atoi(optarg);
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
        case 'f':
        {
            num_failures = atoi(optarg);
            break;
        }
        default:
        {
            mode = 0;
            num_shards = 1;
            qserver_exec = "./bin/qserver";
            hostsfile = "./conf/hosts";
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

    int port = QUERY_HANDLER_PORT;
    try {
        shared_ptr<HandlerProcessorFactory> handlerFactory(new HandlerProcessorFactory(filename,
                local_host_id, num_shards, qserver_exec, hostnames, num_failures));
        shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
        shared_ptr<TBufferedTransportFactory> transport_factory(new TBufferedTransportFactory());
        shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
        TThreadedServer server(handlerFactory,
                         server_transport,
                         transport_factory,
                         protocol_factory);
        server.serve();
    } catch(std::exception& e) {
        fprintf(stderr, "Exception at SuccinctServer:main(): %s\n", e.what());
    }

    return 0;
}

