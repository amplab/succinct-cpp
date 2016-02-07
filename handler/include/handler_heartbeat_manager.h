#ifndef HANDLER_HEARTBEAT_MANAGER_H_
#define HANDLER_HEARTBEAT_MANAGER_H_

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TTransport.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "heartbeat_manager.h"
#include "Master.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace succinct {
class HandlerHeartbeatManager : public HeartbeatManager {

 public:
  typedef struct {
    MasterClient *client;
    boost::shared_ptr<TTransport> transport;
    uint32_t num_reconnection_attempts;
  } MasterClientInfo;

  HandlerHeartbeatManager();

  HandlerHeartbeatManager(int32_t host_id, ConfigurationManager& conf,
                          Logger& logger);

  void Start();

 private:
  void PeriodicallyPing();
  void ConnectToMaster();

  int32_t host_id_;
  MasterClientInfo client_info_;
};
}

#endif /* HANDLER_HEARTBEAT_MANAGER_H_ */
