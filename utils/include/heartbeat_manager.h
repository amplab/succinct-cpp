#ifndef HEARTBEAT_MANAGER_H_
#define HEARTBEAT_MANAGER_H_

#include <thread>

#include "heartbeat_types.h"
#include "configuration_manager.h"
#include "logger.h"

namespace succinct {

class HeartbeatManager {
 public:
  HeartbeatManager() {
  }

  HeartbeatManager(ConfigurationManager& conf, Logger& logger)
      : conf_(conf),
        logger_(logger) {
  }

  virtual ~HeartbeatManager() {
  }

 protected:
  ConfigurationManager conf_;
  Logger logger_;
};

}

#endif /* HEARTBEAT_MANAGER_H_ */
