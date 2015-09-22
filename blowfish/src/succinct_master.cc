#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <algorithm>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include "MasterService.h"
#include "SuccinctService.h"
#include "shard_config.h"
#include "succinct_constants.h"
#include "QueryService.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class SuccinctMaster : virtual public MasterServiceIf {

 public:
  SuccinctMaster(std::vector<std::string> hostnames, ConfigMap& conf,
                 StripeList& s_list) {
    hostnames_ = hostnames;
    conf_ = conf;
    s_list_ = s_list;

    std::vector<SuccinctServiceClient> clients;
    std::vector<boost::shared_ptr<TTransport> > transports;

    // Initiate client to client connections on all clients
    for (int i = 0; i < hostnames.size(); i++) {
      fprintf(stderr, "Connecting to client at %s...\n", hostnames[i].c_str());
      try {
        boost::shared_ptr<TSocket> socket(
            new TSocket(hostnames[i], QUERY_HANDLER_PORT));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        SuccinctServiceClient client(protocol);
        transport->open();
        fprintf(stderr, "Connected!\n");
        clients.push_back(client);
        transports.push_back(transport);
      } catch (std::exception& e) {
        fprintf(stderr, "Could not connect to handler on %s: %s\n",
                hostnames[i].c_str(), e.what());
        exit(1);
      }
    }

    // Start servers at each host
    for (int i = 0; i < hostnames.size(); i++) {
      try {
        fprintf(stderr, "Starting initialization at %s\n",
                hostnames[i].c_str());
        clients[i].send_StartLocalServers();
      } catch (std::exception& e) {
        fprintf(stderr, "Could not send start_servers signal to %s: %s\n",
                hostnames[i].c_str(), e.what());
        exit(1);
      }
    }

    // Cleanup connections
    for (int i = 0; i < hostnames.size(); i++) {
      try {
        clients[i].recv_StartLocalServers();
        fprintf(stderr, "Finished initialization at %s\n",
                hostnames[i].c_str());
      } catch (std::exception& e) {
        fprintf(stderr, "Could not recv start_servers signal to %s: %s\n",
                hostnames[i].c_str(), e.what());
        exit(1);
      }
      try {
        transports[i]->close();
        fprintf(stderr, "Closed connection!\n");
      } catch (std::exception& e) {
        fprintf(stderr, "Could not close connection to %s: %s\n",
                hostnames[i].c_str(), e.what());
        exit(1);
      }
    }

    fprintf(stderr, "Done!\n");
  }

  void GetClient(std::string& _return) {
    _return = hostnames_[rand() % hostnames_.size()];
  }

  int64_t RepairHost(int32_t host_id, int32_t mode) {
    assert(mode == 0 || mode == 1);
    std::vector<int32_t> failed_shards;

    // Determine the list of failed shards
    int32_t num_shards = conf_.size();
    for (int32_t i = 0; i < num_shards; i++) {
      if (i % hostnames_.size() == host_id) {
        fprintf(stderr, "Failed shard: %d\n", i);
        failed_shards.push_back(i);
      }
    }

    // Determine recovery shards
    std::map<int32_t, std::vector<int32_t>> recovery_shard_map;
    for (auto failed_shard : failed_shards) {
      std::vector<int32_t> recovery_shards;
      switch (mode) {
        case 0: {
          GetRecoveryReplica(recovery_shards, failed_shard);
          for (auto recovery_shard : recovery_shards) {
            fprintf(stderr, "Recovery Replica: %u\n", recovery_shard);
          }
          assert(recovery_shards.size() == 1);
          break;
        }
        case 1: {
          GetRecoveryBlocks(recovery_shards, failed_shard);
          for (auto recovery_shard : recovery_shards) {
            fprintf(stderr, "Recovery Block: %u\n", recovery_shard);
          }
          assert(recovery_shards.size() == 10);
          break;
        }
      }
      recovery_shard_map[failed_shard] = recovery_shards;
    }

    // Repair all failed shards
    uint64_t data_size = 0;
    for (auto recovery_entry : recovery_shard_map) {
      data_size += RepairShard(recovery_entry.first, recovery_entry.second);
    }

    return data_size;
  }

 private:

  void GetRecoveryBlocks(std::vector<int32_t>& recovery_blocks,
                         int32_t failed_shard) {
    // Determine the blocks to recover from
    for (auto stripe : s_list_) {
      if (stripe.ContainsBlock(failed_shard)) {
        fprintf(
            stderr,
            "Found stripe that contains block! Computing recovery blocks...\n");
        stripe.GetRecoveryBlocks(recovery_blocks, failed_shard);
        break;
      } else {
        fprintf(stderr, "Stripe does not contain block!");
      }
    }
  }

  void GetRecoveryReplica(std::vector<int32_t>& recovery_replicas,
                          int32_t failed_shard) {
    if (conf_.at(failed_shard).shard_type == ShardType::kPrimary) {
      // If failed shard is primary, just get
      // the replica with the minimum sampling rate
      fprintf(
          stderr,
          "Failed shard is primary: searching for replica with lowest storage...\n");
      recovery_replicas.push_back(
          GetSmallestAvailableReplica(failed_shard, conf_.at(failed_shard)));
    } else if (conf_.at(failed_shard).shard_type == ShardType::kReplica) {
      // If failed shard is not a primary, then search
      // for the primary whose replica this is
      ShardMetadata sdata;
      fprintf(stderr, "Failed shard is a replica, finding primary...\n");
      FindPrimary(sdata, failed_shard);
      fprintf(stderr, "Finding replica with lowest sampling rate...\n");
      recovery_replicas.push_back(
          GetSmallestAvailableReplica(failed_shard, sdata));
    } else {
      assert(0);
    }
  }

  void FindPrimary(ShardMetadata& sdata, int32_t replica_id) {
    for (auto conf_entry : conf_) {
      if (conf_entry.second.ContainsReplica(replica_id)) {
        sdata = conf_entry.second;
        fprintf(stderr, "Found primary: %d\n", conf_entry.first);
        break;
      }
    }
  }

  int32_t GetSmallestAvailableReplica(int32_t failed_shard,
                                      ShardMetadata& sdata) {
    // Note: only works for 3-replication or more
    int32_t smallest_replica, max_sampling_rate;
    if (failed_shard == sdata.replicas.at(0)) {
      smallest_replica = sdata.replicas.at(1);
    } else {
      smallest_replica = sdata.replicas.at(0);
    }
    max_sampling_rate = conf_.at(smallest_replica).sampling_rate;

    for (auto replica : sdata.replicas) {
      if (conf_.at(replica).sampling_rate > max_sampling_rate
          && replica != failed_shard) {
        smallest_replica = replica;
        max_sampling_rate = conf_.at(smallest_replica).sampling_rate;
      }
    }
    fprintf(stderr, "Smallest replica is %d\n", smallest_replica);
    return smallest_replica;
  }

  int64_t RepairShard(int32_t shard_id, std::vector<int32_t>& recovery_shards) {
    std::string filenames[5] = { "metadata", "keyval", "npa", "isa", "sa" };

    // Obtain clients for recovery shards
    std::map<int32_t, SuccinctServiceClient> clients;
    std::vector<boost::shared_ptr<TTransport> > transports;
    for (auto recovery_shard : recovery_shards) {
      uint32_t host_id = recovery_shard % hostnames_.size();
      try {
        fprintf(stderr, "Connecting to %s...\n", hostnames_[host_id].c_str());
        boost::shared_ptr<TSocket> socket(
            new TSocket(hostnames_[host_id], QUERY_HANDLER_PORT));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        SuccinctServiceClient client(protocol);
        transport->open();
        client.ConnectToHandlers();
        fprintf(stderr, "Connected!\n");
        clients.insert(
            std::pair<int32_t, SuccinctServiceClient>(recovery_shard, client));
        transports.push_back(transport);
      } catch (std::exception& e) {
        fprintf(stderr, "Could not connect to handler on %s: %s\n",
                hostnames_[host_id].c_str(), e.what());
        exit(1);
      }
    }

    for (auto filename : filenames) {
      RepairShardFile(filename, clients);
    }

    for (auto transport : transports) {
      transport->close();
    }

    fprintf(stderr, "Repaired shard %d\n", shard_id);

    return 0;
  }

  int64_t RepairShardFile(std::string filename,
                          std::map<int32_t, SuccinctServiceClient>& clients) {

    // Fetch all blocks
    int64_t sum = 0;
    int64_t offset = 0;
    int32_t len = 1024 * 1024 * 16;  // 16MB at a time
    std::string res;

    fprintf(stderr, "Repairing file %s, reading from %zu clients\n",
            filename.c_str(), clients.size());
    do {
      // Loop through the clients
      for (auto client_entry : clients) {
        client_entry.second.send_FetchShardData(client_entry.first, filename,
                                                offset, len);
      }

      for (auto client_entry : clients) {
        client_entry.second.recv_FetchShardData(res);
      }
      sum += res.length();
      offset += res.length();
    } while (res.length() == len);
    fprintf(stderr, "Done!\n");

    return 0;
  }

// Data members
  std::vector<std::string> hostnames_;
  ConfigMap conf_;
  StripeList s_list_;
};

void ParseHosts(std::vector<std::string>& hostnames, std::string& hosts_file) {
  std::ifstream hosts(hosts_file);
  std::string host;
  while (std::getline(hosts, host)) {
    hostnames.push_back(host);
  }
}

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-h hostsfile]\n", exec);
}

int main(int argc, char **argv) {
  if (argc < 1 || argc > 7) {
    print_usage(argv[0]);
    return -1;
  }

  fprintf(stderr, "Command line: ");
  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "%s ", argv[i]);
  }
  fprintf(stderr, "\n");

  int c;
  std::string hosts_file = "./conf/hosts";
  std::string erasure_conf_file = "./conf/erasure.conf";
  std::string conf_file = "./conf/blowfish.conf";
  while ((c = getopt(argc, argv, "h:e:c:")) != -1) {
    switch (c) {
      case 'h':
        hosts_file = optarg;
        break;
      case 'e':
        erasure_conf_file = optarg;
        break;
      case 'c':
        conf_file = optarg;
        break;
      default:
        hosts_file = "./conf/hosts";
    }
  }

  std::vector<std::string> hostnames;
  ConfigMap conf;
  StripeList s_list;

  ParseHosts(hostnames, hosts_file);
  ParseConfig(conf, conf_file);
  ParseErasureConfig(s_list, erasure_conf_file);

  int port = SUCCINCT_MASTER_PORT;
  shared_ptr<SuccinctMaster> handler(
      new SuccinctMaster(hostnames, conf, s_list));
  shared_ptr<TProcessor> processor(new MasterServiceProcessor(handler));

  try {
    shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
    TThreadedServer server(processor, server_transport, transport_factory,
                           protocol_factory);
    fprintf(stderr, "Starting Master Daemon...\n");
    server.serve();
  } catch (std::exception& e) {
    fprintf(stderr, "Exception at SuccinctMaster:main(): %s\n", e.what());
  }

  return 0;
}
