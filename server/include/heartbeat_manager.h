#ifndef HANDLER_HEARTBEAT_MANAGER_H_
#define HANDLER_HEARTBEAT_MANAGER_H_

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TTransport.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "heartbeat_manager.h"
#include "Coordinator.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace succinct {

class HeartbeatManager {

 public:
  typedef struct {
    CoordinatorClient *client;
    boost::shared_ptr<TTransport> transport;
    uint32_t num_reconnection_attempts;
  } CoordinatorClientInfo;

  HeartbeatManager();

  HeartbeatManager(int32_t host_id, ConfigurationManager& conf, Logger& logger);

  void Start();

 private:
  void PeriodicallyPing();
  void ConnectToMaster();

  ConfigurationManager conf_;
  Logger logger_;

  int32_t host_id_;
  CoordinatorClientInfo client_info_;
};

}

#endif /* HANDLER_HEARTBEAT_MANAGER_H_ */
