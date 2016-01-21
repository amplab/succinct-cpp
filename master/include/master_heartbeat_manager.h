#ifndef GLOBAL_HEARTBEAT_MANAGER_H
#define GLOBAL_HEARTBEAT_MANAGER_H

#include <vector>

#include <thrift/transport/TTransport.h>

#include "Handler.h"

using namespace ::apache::thrift::transport;

namespace succinct {

class MasterHeartBeatManager {
public:
  static const uint32_t kDefaultHBInterval = 5;

  typedef struct {
    HandlerClient *client;
    boost::shared_ptr<TTransport> transport;
    std::string hostname;
    uint16_t port;
    bool good;
    uint8_t num_retries;
  } HandlerClientInfo;

  MasterHeartBeatManager(uint32_t hb_interval = kDefaultHBInterval);

  ~MasterHeartBeatManager();

  void Start();

private:
  static void PeriodicHeartBeats(std::vector<HandlerClientInfo> client_list);

  uint32_t hb_interval_;
};

}

#endif // GLOBAL_HEARTBEAT_MANAGER_H
