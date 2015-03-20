#ifndef ADAPTIVE_SUCCINCT_CLIENT_HPP
#define ADAPTIVE_SUCCINCT_CLIENT_HPP

#include "thrift/AdaptiveMasterService.h"
#include "thrift/AdaptiveSuccinctService.h"
#include "thrift/ports.h"
#include "thrift/ResponseQueue.hpp"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class AdaptiveSuccinctClient {
private:
    AdaptiveSuccinctServiceClient *request_client;
    AdaptiveSuccinctServiceClient *response_client;
    boost::shared_ptr<TTransport> request_transport;
    boost::shared_ptr<TTransport> response_transport;
    std::string handler;
    int32_t client_id;
    ResponseQueue<int32_t> replica_ids;

    AdaptiveSuccinctServiceClient *get_client(std::string host, int port, boost::shared_ptr<TTransport> &c_transport) {
        boost::shared_ptr<TSocket> socket(new TSocket(host, port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        AdaptiveSuccinctServiceClient *client = new AdaptiveSuccinctServiceClient(protocol);
        transport->open();
        c_transport = transport;
        return client;
    }

public:
    AdaptiveSuccinctClient(std::string master_ip = "localhost", int32_t port = QUERY_HANDLER_PORT) {

        // Connect to master, get client id, handler
        boost::shared_ptr<TSocket> socket(new TSocket(master_ip, SUCCINCT_MASTER_PORT));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        AdaptiveMasterServiceClient client(protocol);
        transport->open();
        client.get_client(handler);
        client_id = client.get_client_id();
        transport->close();

        fprintf(stderr, "AdaptiveSuccinctClient: Master gave client_id %d and handler %s\n", client_id, handler.c_str());

        // Connect to handler
        request_client = get_client(handler, port, request_transport);
        response_client = get_client(handler, port, response_transport);

        fprintf(stderr, "AdaptiveSuccinctClient: Asking request client to connect to handlers...\n");
        request_client->connect_to_handlers(client_id);
        fprintf(stderr, "AdaptiveSuccinctClient: Connected to handlers!\n");
    }

    ~AdaptiveSuccinctClient() {
        close();
    }

    void get_request(const int64_t key) {
        replica_ids.push(request_client->get_request(client_id, key));
    }

    void get_response(std::string& _return) {
        response_client->get_response(_return, client_id, replica_ids.pop());
    }

    int32_t get_num_keys(const int32_t shard_id) {
        return request_client->get_num_keys(shard_id);
    }

    void close() {
        fprintf(stderr, "Closing request client socket...\n");
        request_transport->close();
        fprintf(stderr, "Closing response client socket...\n");
        response_transport->close();
    }

};



#endif
