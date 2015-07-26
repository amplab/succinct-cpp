#ifndef ADAPTIVE_MANAGEMENT_CLIENT_HPP
#define ADAPTIVE_MANAGEMENT_CLIENT_HPP

#include "AdaptiveMasterService.h"
#include "AdaptiveSuccinctService.h"
#include "ports.h"
#include "response_queue.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class AdaptiveManagementClient {
 public:
  AdaptiveManagementClient(std::string master_ip = "localhost") {

    // Connect to master, get client id, handler
    int32_t port = QUERY_HANDLER_PORT;
    boost::shared_ptr<TSocket> socket(
        new TSocket(master_ip, SUCCINCT_MASTER_PORT));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    AdaptiveMasterServiceClient client(protocol);
    transport->open();
    client.get_client(handler);
    client_id = client.get_client_id();
    transport->close();

    fprintf(
        stderr,
        "AdaptiveManagementClient: Master gave client_id %d and handler %s\n",
        client_id, handler.c_str());

    // Connect to handler
    mgmt_client = get_client(handler, port, mgmt_transport);

    fprintf(
        stderr,
        "AdaptiveManagementClient: Asking management client to connect to handlers...\n");
    mgmt_client->connect_to_handlers(client_id);
    fprintf(stderr, "AdaptiveManagementClient: Connected to handlers!\n");

  }

  ~AdaptiveManagementClient() {
    close();
  }

  int64_t remove_layer(const int32_t shard_id, const int32_t layer_id) {
    return mgmt_client->remove_layer(shard_id, layer_id);
  }

  int64_t reconstruct_layer(const int32_t shard_id, const int32_t layer_id) {
    return mgmt_client->reconstruct_layer(shard_id, layer_id);
  }

  void close() {
    fprintf(stderr, "Closing management client socket...\n");
    mgmt_transport->close();
  }

 private:
  AdaptiveSuccinctServiceClient *get_client(
      std::string host, int port, boost::shared_ptr<TTransport> &c_transport) {
    boost::shared_ptr<TSocket> socket(new TSocket(host, port));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    AdaptiveSuccinctServiceClient *client = new AdaptiveSuccinctServiceClient(
        protocol);
    transport->open();
    c_transport = transport;
    return client;
  }

  AdaptiveSuccinctServiceClient *mgmt_client;
  boost::shared_ptr<TTransport> mgmt_transport;
  std::string handler;
  int32_t client_id;

};

#endif
