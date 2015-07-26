#ifndef ADAPT_BENCHMARK_HPP
#define ADAPT_BENCHMARK_HPP

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "AdaptiveSuccinctService.h"
#include "ports.h"
#include <thread>
#include <sstream>
#include <unistd.h>
#include <functional>

#include "../../adaptive/include/adaptive_succinct_client.h"
#include "../../adaptive/include/response_queue.h"
#include "../../core/include/layered_succinct_shard.h"
#include "adaptive_management_client.h"
#include "benchmark.h"
#include "zipf_generator.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class DynamicAdaptBenchmark : public Benchmark {
 public:
  DynamicAdaptBenchmark(std::string configfile, std::string reqfile,
                        std::string resfile, std::string addfile,
                        std::string delfile, double skew,
                        uint32_t num_partitions, std::string queryfile = "")
      : Benchmark() {

    this->query_client = new AdaptiveSuccinctClient();
    fprintf(stderr, "Created query client.\n");
    this->mgmnt_client = new AdaptiveManagementClient();

    this->reqfile = reqfile;
    this->resfile = resfile;
    this->addfile = addfile;
    this->delfile = delfile;
    this->skew = skew;
    this->num_partitions = num_partitions;

    generate_randoms();
    if (queryfile != "") {
      read_queries(queryfile);
    }
    parse_config_file(configfile);
  }

  static void send_requests(AdaptiveSuccinctClient *query_client,
                            std::vector<int64_t> randoms,
                            std::vector<uint32_t> request_rates,
                            std::vector<uint32_t> durations,
                            std::string reqfile) {

    timestamp_t cur_time;
    const timestamp_t MEASURE_INTERVAL = 5000000;
    timestamp_t measure_start_time = get_timestamp();
    std::ofstream req_stream(reqfile);
    uint64_t num_requests = 0;

    uint64_t i = 0;
    for (uint32_t stage = 0; stage < request_rates.size(); stage++) {
      timestamp_t duration = ((uint64_t) durations[stage]) * 1000L * 1000L;  // Seconds to microseconds
      timestamp_t sleep_time = 1000 * 1000 / request_rates[stage];
      fprintf(
          stderr,
          "Starting stage %u: request-rate = %u Ops/sec, duration = %llu us\n",
          stage, request_rates[stage], duration);
      timestamp_t start_time = get_timestamp();
      while ((cur_time = get_timestamp()) - start_time <= duration) {
        timestamp_t t0 = get_timestamp();
        query_client->get_request(randoms[i % randoms.size()]);
        i++;
        num_requests++;
        while (get_timestamp() - t0 < sleep_time)
          ;
        if ((cur_time = get_timestamp()) - measure_start_time
            >= MEASURE_INTERVAL) {
          timestamp_t diff = cur_time - measure_start_time;
          double rr = ((double) num_requests * 1000 * 1000) / ((double) diff);
          req_stream << cur_time << "\t" << rr << "\n";
          req_stream.flush();
          num_requests = 0;
          measure_start_time = get_timestamp();
        }
      }
      fprintf(stderr, "Finished stage %u, spent %llu us.\n", stage,
              (cur_time - start_time));
    }
    timestamp_t diff = cur_time - measure_start_time;
    double rr = ((double) num_requests * 1000 * 1000) / ((double) diff);
    req_stream << cur_time << "\t" << rr << "\n";
    req_stream.flush();

    // Sleep for some time and let other threads finish
    fprintf(stderr, "Request thread sleeping for 10 seconds...\n");
    sleep(10);
    fprintf(stderr,
            "Finished sending queries, attempting to close query socket...\n");
    query_client->close();
  }

  static void measure_responses(AdaptiveSuccinctClient *query_client,
                                std::string resfile) {
    std::string res;
    const timestamp_t MEASURE_INTERVAL = 5000000;
    uint32_t num_responses = 0;
    timestamp_t cur_time;
    std::ofstream res_stream(resfile);
    uint64_t i = 0;
    timestamp_t measure_start_time = get_timestamp();
    while (true) {
      try {
        query_client->get_response(res);
        num_responses++;
        i++;
        if ((cur_time = get_timestamp()) - measure_start_time
            >= MEASURE_INTERVAL) {
          timestamp_t diff = cur_time - measure_start_time;
          double thput = ((double) num_responses * 1000 * 1000)
              / ((double) diff);
          res_stream << cur_time << "\t" << thput << "\n";
          res_stream.flush();
          num_responses = 0;
          measure_start_time = get_timestamp();
        }
      } catch (std::exception& e) {
        break;
      }
    }
    timestamp_t diff = cur_time - measure_start_time;
    double thput = ((double) num_responses * 1000 * 1000) / ((double) diff);
    res_stream << cur_time << "\t" << thput << "\n";
    res_stream.close();
  }

  static void manage_layers(AdaptiveManagementClient *mgmt_client,
                            std::vector<std::vector<uint32_t>> layers_to_create,
                            std::vector<std::vector<uint32_t>> layers_to_delete,
                            std::vector<uint32_t> durations,
                            std::string addfile, std::string delfile) {

    std::ofstream add_stream(addfile);
    std::ofstream del_stream(delfile);
    timestamp_t cur_time;

    for (uint32_t stage = 0; stage < layers_to_create.size(); stage++) {
      timestamp_t duration = ((uint64_t) durations[stage]) * 1000L * 1000L;  // Seconds to microseconds
      timestamp_t start_time = get_timestamp();
      for (size_t i = 0; i < layers_to_create[stage].size(); i++) {
        try {
          size_t add_size = mgmt_client->reconstruct_layer(
              0, layers_to_create[stage][i]);
          fprintf(stderr, "Created layer with size = %zu\n", add_size);
          add_stream << get_timestamp() << "\t" << i << "\t" << add_size
                     << "\n";
          add_stream.flush();
        } catch (std::exception& e) {
          break;
        }
      }

      for (size_t i = 0; i < layers_to_delete[stage].size(); i++) {
        try {
          size_t del_size = mgmt_client->remove_layer(
              0, layers_to_delete[stage][i]);
          fprintf(stderr, "Deleted layer with size = %zu\n", del_size);
          del_stream << get_timestamp() << "\t" << i << "\t" << del_size
                     << "\n";
          del_stream.flush();
        } catch (std::exception& e) {
          fprintf(stderr, "Error: %s\n", e.what());
          break;
        }
      }

      // Sleep if there is still time left
      if ((cur_time = get_timestamp()) - start_time < duration) {
        fprintf(
            stderr,
            "Done with layer management for stage %u, took %llu us, sleeping for %llu us.\n",
            stage, (cur_time - start_time),
            (duration - (cur_time - start_time)));
        usleep(duration - (cur_time - start_time));
      }
    }

    mgmt_client->close();
  }

  void run_benchmark() {
    ResponseQueue<int32_t> response_queue;

    std::thread req(&DynamicAdaptBenchmark::send_requests, query_client,
                    randoms, request_rates, durations, reqfile);
    fprintf(stderr, "Started request thread!\n");

    std::thread res(&DynamicAdaptBenchmark::measure_responses, query_client,
                    resfile);
    fprintf(stderr, "Started response thread!\n");

    std::thread lay(&DynamicAdaptBenchmark::manage_layers, mgmnt_client,
                    layers_to_create, layers_to_delete, durations, addfile,
                    delfile);

    if (req.joinable()) {
      req.join();
      fprintf(stderr, "Request thread terminated.\n");
    }

//    if (res.joinable()) {
//      res.join();
//      fprintf(stderr, "Response thread terminated.\n");
//    }

    if (lay.joinable()) {
      lay.join();
      fprintf(stderr, "Layer creation thread terminated.\n");
    }

    std::terminate();
  }

 private:
  AdaptiveSuccinctClient *query_client;
  AdaptiveManagementClient *mgmnt_client;
  std::vector<std::vector<uint32_t>> layers_to_delete;
  std::vector<std::vector<uint32_t>> layers_to_create;
  std::vector<uint32_t> request_rates;
  std::vector<uint32_t> durations;
  std::string reqfile;
  std::string resfile;
  std::string addfile;
  std::string delfile;
  double skew;
  uint32_t num_partitions;

  void generate_randoms() {
    count_t q_cnt = 0;
    std::vector<count_t> cum_q_cnt;
    fprintf(stderr, "Computing total number of keys in the system...\n");
    for (uint32_t i = 0; i < this->num_partitions; i++) {
      q_cnt += query_client->get_num_keys(i);
      cum_q_cnt.push_back(q_cnt);
    }
    fprintf(stderr, "Found %lu keys.\n", q_cnt);
    fprintf(stderr, "Generating zipf distribution...\n");
    ZipfGenerator z(skew, q_cnt);
    fprintf(stderr, "Generated zipf distribution, generating keys..\n");
    for (count_t i = 0; i < (q_cnt / num_partitions); i++) {
      // Map from Zipf space to key space
      uint64_t r = z.next();
      uint64_t key = 0;
      uint64_t prev_cnt = 0;
      for (size_t j = 0; j < cum_q_cnt.size(); j++) {
        if (r < cum_q_cnt[j]) {
          key = j * LayeredSuccinctShard::MAX_KEYS + (r - prev_cnt);
          break;
        }
        prev_cnt = cum_q_cnt[j];
      }
      randoms.push_back(key);
    }
    fprintf(stderr, "Generated keys.\n");
  }

  void read_queries(std::string filename) {
    std::ifstream inputfile(filename);
    if (!inputfile.is_open()) {
      fprintf(stderr, "Error: Query file [%s] may be missing.\n",
              filename.c_str());
      return;
    }

    std::string line, bin, query;
    while (getline(inputfile, line)) {
      // Extract key and value
      int split_index = line.find_first_of('\t');
      bin = line.substr(0, split_index);
      query = line.substr(split_index + 1);
      queries.push_back(query);
    }
    inputfile.close();
  }

  void parse_csv_entry(std::vector<uint32_t> &out, std::string csv_entry) {
    std::string delimiter = ",";
    size_t pos = 0;
    std::string elem;
    while ((pos = csv_entry.find(delimiter)) != std::string::npos) {
      elem = csv_entry.substr(0, pos);
      out.push_back(atoi(elem.c_str()));
      csv_entry.erase(0, pos + delimiter.length());
    }
    if (csv_entry != "-") {
      out.push_back(atoi(csv_entry.c_str()));
    } else {
      assert(out.empty());
    }
  }

  void parse_config_file(std::string configfile) {
    std::ifstream conf(configfile);
    assert(conf.is_open());
    std::string conf_entry;
    while (std::getline(conf, conf_entry, '\n')) {
      std::istringstream iss(conf_entry);
      std::string rr, dur, add, del;
      std::vector<uint32_t> l_add, l_del;
      iss >> rr;
      iss >> dur;
      iss >> add;
      iss >> del;
      fprintf(stderr, "rr = %s, dur = %s, add = %s, del = %s\n", rr.c_str(),
              dur.c_str(), add.c_str(), del.c_str());

      request_rates.push_back(atoi(rr.c_str()));
      durations.push_back(atoi(dur.c_str()));
      parse_csv_entry(l_add, add);
      parse_csv_entry(l_del, del);
      std::sort(l_add.begin(), l_add.end(), std::greater<uint32_t>());
      std::sort(l_del.begin(), l_del.end());
      layers_to_create.push_back(l_add);
      layers_to_delete.push_back(l_del);
    }
    assert(request_rates.size() == durations.size());
  }
};

#endif
