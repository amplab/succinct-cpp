#include <ctime>
#include <thread>

#include "heartbeat_manager.h"
#include "constants.h"

namespace succinct {

HeartbeatManager::HeartbeatManager() {
  host_id_ = 0;
  client_info_.client = NULL;
}

HeartbeatManager::HeartbeatManager(int32_t host_id, ConfigurationManager& conf,
                                   Logger& logger)
    : host_id_(host_id),
      conf_(conf),
      logger_(logger) {
  client_info_.client = NULL;
}

void HeartbeatManager::ConnectToMaster() {
  const char *master_hostname = conf_.Get("MASTER_HOSTNAME").c_str();
  int32_t master_port = conf_.GetInt("MASTER_PORT");

  logger_.Info("Heartbeat manager connecting to master at %s:%d.",
               master_hostname, master_port);

  boost::shared_ptr<TSocket> socket(new TSocket(master_hostname, master_port));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  logger_.Debug("Created transport.");

  if (client_info_.client != NULL) {
    logger_.Debug("Deleting old reference to client.");
    delete client_info_.client;
  }

  client_info_.client = new CoordinatorClient(protocol);
  client_info_.transport = transport;
  client_info_.num_reconnection_attempts = 0;
  logger_.Debug("Created new client.");

  client_info_.transport->open();
  logger_.Info("Connection successful.");
}

void HeartbeatManager::PeriodicallyPing() {
  int32_t hb_period = Defaults::kHBPeriod;
  logger_.Info("Started thread with hb_period=%d", hb_period);

  const char* master_hostname = conf_.Get("MASTER_HOSTNAME").c_str();
  int32_t master_port = conf_.GetInt("MASTER_PORT");

  try {
    ConnectToMaster();
  } catch (std::exception& e) {
    logger_.Error("Could not connect to master at %s:%d. Reason: %s",
                  master_hostname, master_port, e.what());
    exit(-1);
  }

  while (true) {
    HeartBeat hb;
    std::time_t ts = std::time(NULL);
    hb.timestamp = ts;
    hb.sender_id = host_id_;

    try {
      client_info_.client->Ping(hb);
      logger_.Debug("Pinged master at %s:%d with timestamp %d.",
                    master_hostname, master_port, hb.timestamp);
    } catch (std::exception& e) {
      logger_.Warn("Could not ping master at %s:%d. Reason: %s",
                   master_hostname, master_port, e.what());
      try {
        ConnectToMaster();
      } catch (std::exception& e) {
        logger_.Warn("Reconnection attempt failed. Attempt#=%d.",
                     client_info_.num_reconnection_attempts);
        client_info_.num_reconnection_attempts++;
      }
    }
    sleep(hb_period);
  }
}

void HeartbeatManager::Start() {
  logger_.Info("Starting handler heartbeat manager thread...");
  auto hb_thread = std::thread([=] {
    PeriodicallyPing();
  });
  logger_.Info("Detaching thread...");
  hb_thread.detach();
}

}
