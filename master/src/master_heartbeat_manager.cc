#include "master_heartbeat_manager.h"

namespace succinct {

MasterHeartBeatManager::MasterHeartBeatManager(uint32_t hb_interval) {
  hb_interval_ = hb_interval;
}

void MasterHeartBeatManager::PeriodicHeartBeats(std::vector<HandlerClientInfo> client_list) {

}

void MasterHeartBeatManager::Start() {

}

}
