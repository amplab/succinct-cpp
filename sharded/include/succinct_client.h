#ifndef SHARDED_SUCCINCT_CLIENT_H_
#define SHARDED_SUCCINCT_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include "AggregatorService.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

// Simple wrapper around thrift client, for ease of use
class SuccinctClient {
 public:
  SuccinctClient(const std::string& host, const uint32_t port = AGGREGATOR_PORT) {
    socket_ = stdcxx::shared_ptr<TSocket>(new TSocket("localhost", port));
    transport_ = stdcxx::shared_ptr<TTransport>(new TBufferedTransport(socket_));
    protocol_ = stdcxx::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
    transport_->open();
    client_ = new AggregatorServiceClient(protocol_);
    client_->ConnectToServers();
  }

  ~SuccinctClient() {
    client_->DisconnectFromServers();
    transport_->close();
    delete client_;
  }

  void Regex(std::set<int64_t> & _return, const std::string& query) {
    client_->Regex(_return, query);
  }

  int64_t Count(const std::string& query) {
    return client_->Count(query);
  }

  void Extract(std::string& _return, const int64_t offset,
                     const int64_t len) {
    client_->Extract(_return, offset, len);
  }

  void Search(std::vector<int64_t> & _return, const std::string& query) {
    client_->Search(_return, query);
  }

  int32_t GetNumShards() {
    return client_->GetNumShards();
  }

  int64_t GetTotSize() {
    return client_->GetTotSize();
  }

  // Non-blocking send/receive calls
  // Send calls
  void SendRegex(const std::string& query) {
    client_->send_Regex(query);
  }

  void SendCount(const std::string& query) {
    client_->send_Count(query);
  }

  void SendExtract(const int64_t offset, const int64_t len) {
    client_->send_Extract(offset, len);
  }

  void SendSearch(const std::string& query) {
    client_->send_Search(query);
  }

  // Receive calls
  void RecvRegex(std::set<int64_t> & _return) {
    client_->recv_Regex(_return);
  }

  int64_t RecvCount() {
    return client_->recv_Count();
  }

  void RecvExtract(std::string& _return) {
    client_->recv_Extract(_return);
  }

  void RecvSearch(std::vector<int64_t>& _return) {
    client_->recv_Search(_return);
  }

 private:
  stdcxx::shared_ptr<TSocket> socket_;
  stdcxx::shared_ptr<TTransport> transport_;
  stdcxx::shared_ptr<TProtocol> protocol_;
  AggregatorServiceClient *client_;

};
#endif // SHARDED_SUCCINCT_CLIENT_H_
