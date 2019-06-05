#include "succinct_client.h"
#include "ports.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

// Dummy program to initialize the Aggregator and die;
// Does not take any command line arguments
int main() {
  stdcxx::shared_ptr<TSocket> socket_(new TSocket("localhost", AGGREGATOR_PORT));
  stdcxx::shared_ptr<TTransport> transport_(new TBufferedTransport(socket_));
  stdcxx::shared_ptr<TProtocol> protocol_(new TBinaryProtocol(transport_));
  transport_->open();
  AggregatorServiceClient client_(protocol_);
  client_.Initialize();
  transport_->close();
  return 0;
}
