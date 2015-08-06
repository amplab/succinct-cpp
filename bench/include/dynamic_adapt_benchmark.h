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

#include "adaptive_succinct_client.h"
#include "response_queue.h"
#include "layered_succinct_shard.h"
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

    this->query_client_ = new AdaptiveSuccinctClient();
    fprintf(stderr, "Created query client.\n");
    this->management_client_ = new AdaptiveManagementClient();

    this->reqfile_ = reqfile;
    this->resfile_ = resfile;
    this->addfile_ = addfile;
    this->delfile_ = delfile;
    this->skew_ = skew;
    this->num_partitions_ = num_partitions;

    generate_randoms();
    if (queryfile != "") {
      ReadQueries(queryfile);
    }
    ParseConfigFile(configfile);
  }

  static void SendRequests(AdaptiveSuccinctClient *query_client,
                           std::vector<int64_t> randoms,
                           std::vector<uint32_t> request_rates,
                           std::vector<uint32_t> durations,
                           std::string requests_file) {

    TimeStamp cur_time;
    const TimeStamp MEASURE_INTERVAL = 5000000;
    TimeStamp measure_start_time = GetTimestamp();
    std::ofstream req_stream(requests_file);
    uint64_t num_requests = 0;

    uint64_t i = 0;
    for (uint32_t stage = 0; stage < request_rates.size(); stage++) {
      TimeStamp duration = ((uint64_t) durations[stage]) * 1000L * 1000L;  // Seconds to microseconds
      TimeStamp sleep_time = 1000 * 1000 / request_rates[stage];
      fprintf(
          stderr,
          "Starting stage %u: request-rate = %u Ops/sec, duration = %llu us\n",
          stage, request_rates[stage], duration);
      TimeStamp start_time = GetTimestamp();
      while ((cur_time = GetTimestamp()) - start_time <= duration) {
        TimeStamp t0 = GetTimestamp();
        query_client->get_request(randoms[i % randoms.size()]);
        i++;
        num_requests++;
        while (GetTimestamp() - t0 < sleep_time)
          ;
        if ((cur_time = GetTimestamp()) - measure_start_time
            >= MEASURE_INTERVAL) {
          TimeStamp diff = cur_time - measure_start_time;
          double rr = ((double) num_requests * 1000 * 1000) / ((double) diff);
          req_stream << cur_time << "\t" << rr << "\n";
          req_stream.flush();
          num_requests = 0;
          measure_start_time = GetTimestamp();
        }
      }
      fprintf(stderr, "Finished stage %u, spent %llu us.\n", stage,
              (cur_time - start_time));
    }
    TimeStamp diff = cur_time - measure_start_time;
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

  static void MeasureResponses(AdaptiveSuccinctClient *query_client,
                               std::string responses_file) {
    std::string response;
    const TimeStamp MEASURE_INTERVAL = 5000000;
    uint32_t num_responses = 0;
    TimeStamp cur_time;
    std::ofstream res_stream(responses_file);
    uint64_t i = 0;
    TimeStamp measure_start_time = GetTimestamp();
    while (true) {
      try {
        query_client->get_response(response);
        num_responses++;
        i++;
        if ((cur_time = GetTimestamp()) - measure_start_time
            >= MEASURE_INTERVAL) {
          TimeStamp diff = cur_time - measure_start_time;
          double thput = ((double) num_responses * 1000 * 1000)
              / ((double) diff);
          res_stream << cur_time << "\t" << thput << "\n";
          res_stream.flush();
          num_responses = 0;
          measure_start_time = GetTimestamp();
        }
      } catch (std::exception& e) {
        break;
      }
    }
    TimeStamp diff = cur_time - measure_start_time;
    double thput = ((double) num_responses * 1000 * 1000) / ((double) diff);
    res_stream << cur_time << "\t" << thput << "\n";
    res_stream.close();
  }

  static void ManageLayers(AdaptiveManagementClient *management_client,
                           std::vector<std::vector<uint32_t>> layers_to_create,
                           std::vector<std::vector<uint32_t>> layers_to_delete,
                           std::vector<uint32_t> durations,
                           std::string additions_file,
                           std::string deletions_file) {

    std::ofstream add_stream(additions_file);
    std::ofstream del_stream(deletions_file);
    TimeStamp cur_time;

    for (uint32_t stage = 0; stage < layers_to_create.size(); stage++) {
      TimeStamp duration = ((uint64_t) durations[stage]) * 1000L * 1000L;  // Seconds to microseconds
      TimeStamp start_time = GetTimestamp();
      for (size_t i = 0; i < layers_to_create[stage].size(); i++) {
        try {
          size_t add_size = management_client->reconstruct_layer(
              0, layers_to_create[stage][i]);
          fprintf(stderr, "Created layer with size = %zu\n", add_size);
          add_stream << GetTimestamp() << "\t" << i << "\t" << add_size << "\n";
          add_stream.flush();
        } catch (std::exception& e) {
          break;
        }
      }

      for (size_t i = 0; i < layers_to_delete[stage].size(); i++) {
        try {
          size_t del_size = management_client->remove_layer(
              0, layers_to_delete[stage][i]);
          fprintf(stderr, "Deleted layer with size = %zu\n", del_size);
          del_stream << GetTimestamp() << "\t" << i << "\t" << del_size << "\n";
          del_stream.flush();
        } catch (std::exception& e) {
          fprintf(stderr, "Error: %s\n", e.what());
          break;
        }
      }

      // Sleep if there is still time left
      if ((cur_time = GetTimestamp()) - start_time < duration) {
        fprintf(
            stderr,
            "Done with layer management for stage %u, took %llu us, sleeping for %llu us.\n",
            stage, (cur_time - start_time),
            (duration - (cur_time - start_time)));
        usleep(duration - (cur_time - start_time));
      }
    }

    management_client->close();
  }

  void RunBenchmark() {
    ResponseQueue<int32_t> response_queue;

    std::thread req(&DynamicAdaptBenchmark::SendRequests, query_client_,
                    randoms_, request_rates_, durations_, reqfile_);
    fprintf(stderr, "Started request thread!\n");

    std::thread res(&DynamicAdaptBenchmark::MeasureResponses, query_client_,
                    resfile_);
    fprintf(stderr, "Started response thread!\n");

    std::thread lay(&DynamicAdaptBenchmark::ManageLayers, management_client_,
                    layers_to_create_, layers_to_delete_, durations_, addfile_,
                    delfile_);

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
  AdaptiveSuccinctClient *query_client_;
  AdaptiveManagementClient *management_client_;
  std::vector<std::vector<uint32_t>> layers_to_delete_;
  std::vector<std::vector<uint32_t>> layers_to_create_;
  std::vector<uint32_t> request_rates_;
  std::vector<uint32_t> durations_;
  std::string reqfile_;
  std::string resfile_;
  std::string addfile_;
  std::string delfile_;
  double skew_;
  uint32_t num_partitions_;

  void generate_randoms() {
    uint64_t query_count = 0;
    std::vector<uint64_t> cumulative_query_counts;
    fprintf(stderr, "Computing total number of keys in the system...\n");
    for (uint32_t i = 0; i < this->num_partitions_; i++) {
      query_count += query_client_->get_num_keys(i);
      cumulative_query_counts.push_back(query_count);
    }
    fprintf(stderr, "Found %llu keys.\n", query_count);
    fprintf(stderr, "Generating zipf distribution...\n");
    ZipfGenerator z(skew_, query_count);
    fprintf(stderr, "Generated zipf distribution, generating keys..\n");
    for (uint64_t i = 0; i < (query_count / num_partitions_); i++) {
      // Map from Zipf space to key space
      uint64_t r = z.Next();
      uint64_t key = 0;
      uint64_t prev_cnt = 0;
      for (size_t j = 0; j < cumulative_query_counts.size(); j++) {
        if (r < cumulative_query_counts[j]) {
          key = j * LayeredSuccinctShard::MAX_KEYS + (r - prev_cnt);
          break;
        }
        prev_cnt = cumulative_query_counts[j];
      }
      randoms_.push_back(key);
    }
    fprintf(stderr, "Generated keys.\n");
  }

  void ReadQueries(std::string filename) {
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
      queries_.push_back(query);
    }
    inputfile.close();
  }

  void ParseCsvEntry(std::vector<uint32_t> &out, std::string csv_entry) {
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

  void ParseConfigFile(std::string config_file) {
    std::ifstream conf(config_file);
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

      request_rates_.push_back(atoi(rr.c_str()));
      durations_.push_back(atoi(dur.c_str()));
      ParseCsvEntry(l_add, add);
      ParseCsvEntry(l_del, del);
      std::sort(l_add.begin(), l_add.end(), std::greater<uint32_t>());
      std::sort(l_del.begin(), l_del.end());
      layers_to_create_.push_back(l_add);
      layers_to_delete_.push_back(l_del);
    }
    assert(request_rates_.size() == durations_.size());
  }
};

#endif
