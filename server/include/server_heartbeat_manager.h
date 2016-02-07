#ifndef SERVER_HEARTBEAT_MANAGER_H_
#define SERVER_HEARTBEAT_MANAGER_H_

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TTransport.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "heartbeat_manager.h"
#include "Handler.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

namespace succinct {

class ServerHeartbeatManager : public HeartbeatManager {
 public:
  typedef struct {
    HandlerClient *client;
    boost::shared_ptr<TTransport> transport;
    uint32_t num_reconnection_attempts;
  } HandlerClientInfo;

  ServerHeartbeatManager();

  ServerHeartbeatManager(int32_t server_id_, ConfigurationManager& conf,
                         Logger& logger);

  void Start();

 private:
  void PeriodicallyPing();
  void ConnectToHandler();

  int32_t server_id_;
  HandlerClientInfo client_info_;
};

}

#endif /* SERVER_HEARTBEAT_MANAGER_H_ */
