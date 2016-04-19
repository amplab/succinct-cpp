#include <vector>
#include <fstream>
#include <ctime>
#include <thread>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>

#include "Coordinator.h"
#include "Handler.h"
#include "logger.h"
#include "configuration_manager.h"
#include "constants.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

namespace succinct {
class CoordinatorImpl: virtual public CoordinatorIf {

public:
	typedef struct HandlerMetadata {
	public:
		HandlerMetadata() {
			host_id = 0;
			hostname = "";

			last_hb_local_timestamp = 0;
			last_hb_timestamp = 0;
			goodness = 1.0;
		}

		HandlerMetadata(int32_t host_id, const std::string& hostname) {
			this->host_id = host_id;
			this->hostname = hostname;

			last_hb_local_timestamp = 0;
			last_hb_timestamp = 0;
			goodness = 1.0;
		}

		void NewHeartbeat(std::time_t hb_timestamp,
				std::time_t local_timestamp) {
			last_hb_timestamp = hb_timestamp;
			last_hb_local_timestamp = local_timestamp;
		}

		void UpdateGoodness(std::time_t current_local_timestamp) {
			goodness = ((double) Defaults::kHBPeriod)
					/ (double) (current_local_timestamp
							- last_hb_local_timestamp);
		}

		int32_t host_id;
		std::string hostname;
		std::time_t last_hb_local_timestamp;
		std::time_t last_hb_timestamp;
		double goodness;
	} HandlerMetadata;

	typedef std::map<int32_t, HandlerMetadata> HandlerMetadataMap;

	CoordinatorImpl(ConfigurationManager& conf, Logger& logger) :
			conf_(conf), logger_(logger) {

		ReadHostNames(conf_.Get("HOSTS_LIST"));

		std::vector<HandlerClient> clients;
		std::vector<boost::shared_ptr<TTransport> > transports;
		uint16_t handler_port = conf_.GetInt("HANDLER_PORT");

		// Initiate handler to handler connections on all servers
		for (int i = 0; i < hostnames_.size(); i++) {
			logger_.Info("Connecting to handler at %s...",
					hostnames_[i].c_str());
			try {
				boost::shared_ptr<TSocket> socket(
						new TSocket(hostnames_[i], handler_port));
				boost::shared_ptr<TTransport> transport(
						new TBufferedTransport(socket));
				boost::shared_ptr<TProtocol> protocol(
						new TBinaryProtocol(transport));
				HandlerClient client(protocol);
				transport->open();
				logger_.Info("Connection successful.");
				clients.push_back(client);
				transports.push_back(transport);
			} catch (std::exception& e) {
				logger_.Error(
						"Could not connect to handler on %s:%d. Reason: %s",
						hostnames_[i].c_str(), handler_port, e.what());
				exit(1);
			}
		}

		// Start servers at each host
		for (int i = 0; i < hostnames_.size(); i++) {
			try {
				logger_.Info("Starting initialization at %s...",
						hostnames_[i].c_str());
				clients[i].send_StartLocalServers();
			} catch (std::exception& e) {
				logger_.Error(
						"Could not send start_servers signal to %s. Reason: %s",
						hostnames_[i].c_str(), e.what());
				exit(1);
			}
		}

		// Cleanup connections
		for (int i = 0; i < hostnames_.size(); i++) {
			try {
				clients[i].recv_StartLocalServers();
				logger_.Info("Finished initialization at %s.",
						hostnames_[i].c_str());
			} catch (std::exception& e) {
				logger_.Error(
						"Could not receive start_servers signal from %s. Reason: %s",
						hostnames_[i].c_str(), e.what());
				exit(1);
			}
			try {
				transports[i]->close();
				logger_.Info("Closed connection.");
			} catch (std::exception& e) {
				logger_.Error("Could not close connection to %s. Reason: %s",
						hostnames_[i].c_str(), e.what());
				exit(1);
			}
		}

		// Initialize handler metadata
		for (int i = 0; i < hostnames_.size(); i++) {
			typedef std::pair<int32_t, HandlerMetadata> HMEntry;
			hm_map_.insert(HMEntry(i, HandlerMetadata(i, hostnames_[i])));

			logger_.Info("Initialized metadata for handler at %s...",
					hostnames_[i].c_str());
		}

		logger_.Info("Initialization complete.");

//    std::thread hb_monitor_thread = std::thread([=] {
//      MonitorHeartbeats();
//    });
//    hb_monitor_thread.detach();
	}

	void GetHostname(std::string& _return) {
		_return = hostnames_[rand() % hostnames_.size()];
	}

	void Ping(const HeartBeat& hb) {
		logger_.Debug("Received heartbeat from handler %d.");
		hm_map_.at(hb.sender_id).NewHeartbeat(hb.timestamp, std::time(NULL));
		logger_.Debug("Updated handler metadata.");
	}

	void MonitorHeartbeats() {
		while (true) {
			for (size_t i = 0; i < hm_map_.size(); i++) {
				hm_map_[i].UpdateGoodness(std::time(NULL));
				if (hm_map_[i].goodness < 0.3) {
					logger_.Warn("Goodness for handler %d at %s is low (%lf)!",
							hm_map_[i].host_id, hm_map_[i].hostname.c_str(),
							hm_map_[i].goodness);
				} else {
					logger_.Debug("Goodness for handler %d at %s is ok (%lf).",
							hm_map_[i].host_id, hm_map_[i].hostname.c_str(),
							hm_map_[i].goodness);
				}
			}
			sleep(Defaults::kHBPeriod);
		}
	}

private:
	void ReadHostNames(const std::string& hostsfile) {
		std::ifstream hosts(hostsfile);
		if (!hosts.is_open()) {
			logger_.Error("Could not open hosts file at %s.",
					hostsfile.c_str());
			exit(1);
		}
		std::string host;
		while (std::getline(hosts, host, '\n')) {
			hostnames_.push_back(host);
			logger_.Info("Read host: %s.", host.c_str());
		}
	}

	HandlerMetadataMap hm_map_;
	std::vector<std::string> hostnames_;
	ConfigurationManager conf_;
	Logger logger_;
};

}

int main(int argc, char **argv) {
	ConfigurationManager conf;
	std::string coordinator_log_output = conf.Get("MASTER_LOG_FILE");
	FILE *desc = fopen(coordinator_log_output.c_str(), "a");
	if (desc == NULL) {
		fprintf(stderr,
				"Could not obtain descriptor for %s, logging to stderr.\n",
				coordinator_log_output.c_str());
		desc = stderr;
	}

	Logger logger(static_cast<Logger::Level>(conf.GetInt("MASTER_LOG_LEVEL")),
			desc);
	shared_ptr<succinct::CoordinatorImpl> coordinator(
			new succinct::CoordinatorImpl(conf, logger));
	shared_ptr<TProcessor> processor(
			new succinct::CoordinatorProcessor(coordinator));

	try {
		shared_ptr<TServerSocket> server_transport(
				new TServerSocket(conf.GetInt("MASTER_PORT")));
		shared_ptr<TBufferedTransportFactory> transport_factory(
				new TBufferedTransportFactory());
		shared_ptr<TProtocolFactory> protocol_factory(
				new TBinaryProtocolFactory());
		TThreadedServer server(processor, server_transport, transport_factory,
				protocol_factory);
		logger.Info("Starting Master on port %d...",
				conf.GetInt("MASTER_PORT"));
		server.serve();
	} catch (std::exception& e) {
		logger.Error("Could not create master listening on port %d. Reason: %s",
				conf.GetInt("MASTER_PORT"), e.what());
	}

	return 0;
}
