#ifndef SHARDED_SUCCINCT_CLIENT_H_
#define SHARDED_SUCCINCT_CLIENT_H_

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include "KVAggregatorService.h"
#include "kv_ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

// Simple wrapper around thrift client, for ease of use
class SuccinctKVClient {
 public:
  SuccinctKVClient(const std::string& host, const uint32_t port =
  KV_AGGREGATOR_PORT) {
    socket_ = stdcxx::shared_ptr<TSocket>(new TSocket("localhost", port));
    transport_ = stdcxx::shared_ptr<TTransport>(new TBufferedTransport(socket_));
    protocol_ = stdcxx::shared_ptr<TProtocol>(new TBinaryProtocol(transport_));
    transport_->open();
    client_ = new KVAggregatorServiceClient(protocol_);
    client_->ConnectToServers();
  }

  ~SuccinctKVClient() {
    client_->DisconnectFromServers();
    transport_->close();
    delete client_;
  }

  void Regex(std::set<int64_t> & _return, const std::string& query) {
    client_->Regex(_return, query);
  }

  void Get(std::string& _return, const int64_t key) {
    client_->Get(_return, key);
  }

  void Access(std::string& _return, const int64_t key, const int32_t offset,
              const int32_t len) {
    client_->Access(_return, key, offset, len);
  }

  int64_t Count(const std::string& query) {
    return client_->Count(query);
  }

  void Search(std::set<int64_t> & _return, const std::string& query) {
    client_->Search(_return, query);
  }

  int32_t GetNumKeys() {
    return client_->GetNumKeys();
  }

  int32_t GetNumKeysShard(const int32_t shard_id) {
    return client_->GetNumKeysShard(shard_id);
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

  void SendGet(const int64_t key) {
    client_->send_Get(key);
  }

  void SendAccess(const int64_t key, const int32_t offset, const int32_t len) {
    client_->send_Access(key, offset, len);
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

  void RecvGet(std::string& _return) {
    client_->recv_Get(_return);
  }

  void RecvAccess(std::string& _return) {
    client_->recv_Access(_return);
  }

  void RecvSearch(std::set<int64_t>& _return) {
    client_->recv_Search(_return);
  }

 private:
  stdcxx::shared_ptr<TSocket> socket_;
  stdcxx::shared_ptr<TTransport> transport_;
  stdcxx::shared_ptr<TProtocol> protocol_;
  KVAggregatorServiceClient *client_;

};
#endif // SHARDED_SUCCINCT_CLIENT_H_
