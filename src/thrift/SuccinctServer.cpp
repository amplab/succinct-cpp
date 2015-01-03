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
            std::vector<std::string> hostnames, bool construct) {
        this->local_host_id = local_host_id;
        this->num_shards = num_shards;
        this->hostnames = hostnames;
        this->filename = filename;
        this->qserver_exec = qserver_exec;
        this->construct = construct;
    }

    int32_t start_servers() {
        return initialize(0);
    }

    int32_t initialize(const int32_t mode) {
        std::string underscore = "_";

        // Start QueryServer(s)
        for(uint32_t i = 0; i < num_shards; i++) {
            std::string split_file = filename + underscore + std::to_string(i);
            std::string log_path = split_file + underscore + std::string("log");
            std::string start_cmd = std::string("nohup ") + qserver_exec +
                                    std::string(" -m ") +
                                    std::to_string((int)(!construct)) +
                                    std::string(" -p ") +
                                    std::to_string(QUERY_SERVER_PORT + i) +
                                    std::string(" ") +
                                    split_file + std::string(" 2>&1 > ") +
                                    log_path +
                                    std::string("&");
            fprintf(stderr, "Launching QueryServer: [%s]\n", start_cmd.c_str());
            system(start_cmd.c_str());
        }

        // FIXME: This is a hacky workaround, and could fail. Fix it.
        sleep(5);

        // Connect to query servers and start initialization
        for(uint32_t i = 0; i < num_shards; i++) {
            int port = QUERY_SERVER_PORT + i;
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            QueryServiceClient client(protocol);
            transport->open();
            fprintf(stderr, "Connected to QueryServer %u!\n ", i);
            int32_t shard_id = local_host_id * num_shards + i;
            for(int t = 0; t < 100000; t++) {
            fprintf(stderr, "Initializing shard %d\n", shard_id);
            }
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
        uint32_t qserver_id = key % num_shards; // TODO: +1, +2
        qservers.at(qserver_id).get(_return, key);
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
        int32_t host_id = shard_id / hostnames.size();
        int32_t num;
        if(host_id == local_host_id) {
            return qservers.at(shard_id % num_shards).get_num_keys();
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
    bool construct;
    std::vector<std::string> hostnames;
    std::vector<QueryServiceClient> qservers;
    std::vector<boost::shared_ptr<TTransport>> qserver_transports;
    std::vector<boost::shared_ptr<TTransport> > qhandler_transports;
    std::map<int, SuccinctServiceClient> qhandlers;
    uint32_t num_shards;
    uint32_t local_host_id;
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [-s num_shards] [-q query_server_executible] [-h hostsfile] [-i hostid] file\n", exec);
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
    while((c = getopt(argc, argv, "m:s:q:h:i:")) != -1) {
        switch(c) {
        case 'm':
            mode = atoi(optarg);
            break;
        case 's':
            num_shards = atoi(optarg);
            break;
        case 'q':
            qserver_exec = optarg;
            break;
        case 'h':
            hostsfile = optarg;
            break;
        case 'i':
            local_host_id = atoi(optarg);
            break;
        default:
            mode = 0;
            num_shards = 1;
            qserver_exec = "./bin/qserver";
            hostsfile = "./conf/hosts";
            local_host_id = 0;
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
    shared_ptr<SuccinctServiceHandler> handler(new SuccinctServiceHandler(filename, local_host_id, num_shards, qserver_exec, hostnames, construct));
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

