#include "server_heartbeat_manager.h"

#include <ctime>

#include "constants.h"

namespace succinct {

ServerHeartbeatManager::ServerHeartbeatManager()
    : HeartbeatManager() {
  server_id_ = 0;
  client_info_.client = NULL;
}

ServerHeartbeatManager::ServerHeartbeatManager(int32_t server_id,
                                               ConfigurationManager& conf,
                                               Logger& logger)
    : HeartbeatManager(conf, logger) {
  server_id_ = server_id;
  client_info_.client = NULL;
}

void ServerHeartbeatManager::ConnectToHandler() {
  int32_t handler_port = conf_.GetInt("HANDLER_PORT");

  logger_.Info("Heartbeat manager connecting to handler at localhost:%d.",
               handler_port);

  boost::shared_ptr<TSocket> socket(new TSocket("localhost", handler_port));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  logger_.Debug("Created transport.");

  if (client_info_.client != NULL) {
    logger_.Debug("Deleting old reference to client.");
    delete client_info_.client;
  }

  client_info_.client = new HandlerClient(protocol);
  client_info_.transport = transport;
  client_info_.num_reconnection_attempts = 0;
  logger_.Debug("Created new client.");

  client_info_.transport->open();
  logger_.Info("Connection successful.");
}

void ServerHeartbeatManager::PeriodicallyPing() {
  int32_t hb_period = Defaults::kHBPeriod;
  logger_.Info("Started thread with hb_period=%d", hb_period);

  int32_t handler_port = conf_.GetInt("HANDLER_PORT");

  try {
    ConnectToHandler();
  } catch (std::exception& e) {
    logger_.Error("Could not connect to handler at localhost:%d. Reason: %s",
                  handler_port, e.what());
    exit(-1);
  }

  while (true) {
    HeartBeat hb;
    std::time_t ts = std::time(NULL);
    hb.timestamp = ts;
    hb.sender_id = server_id_;

    try {
      client_info_.client->Ping(hb);
      logger_.Debug("Pinged handler at localhost:%d with timestamp %d.",
                    handler_port, hb.timestamp);
    } catch (std::exception& e) {
      logger_.Warn("Could not ping local handler. Reason: %s", e.what());
      try {
        ConnectToHandler();
      } catch (std::exception& e) {
        logger_.Warn("Reconnection attempt failed. Attempt#=%d.",
                     client_info_.num_reconnection_attempts);
        client_info_.num_reconnection_attempts++;
      }
    }
    sleep(hb_period);
  }
}

void ServerHeartbeatManager::Start() {
  logger_.Info("Starting server heartbeat manager thread...");
  auto hb_thread = std::thread([=] {
    PeriodicallyPing();
  });
  logger_.Info("Detaching thread...");
  hb_thread.detach();
}

}
