#ifndef ADAPT_BENCHMARK_HPP
#define ADAPT_BENCHMARK_HPP

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "AdaptiveQueryService.h"
#include "ports.h"

#include <thread>
#include <sstream>
#include <unistd.h>
#include <atomic>
#include <functional>

#include "benchmark.h"
#include "zipf_generator.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class AdaptBenchmark : public Benchmark {
 public:
  AdaptBenchmark(std::string config_file, std::string requests_file,
                 std::string responses_file, std::string additions_file,
                 std::string deletions_file, double skew, uint32_t fetch_length,
                 uint32_t batch_size, std::string query_file)
      : Benchmark() {

    this->query_client_ = this->GetClient(query_transport_);
    fprintf(stderr, "Created query client.\n");
    this->management_client_ = this->GetClient(management_transport_);
    fprintf(stderr, "Created management client.\n");
    this->stats_client_ = this->GetClient(stats_transport_);
    fprintf(stderr, "Created stats client.\n");

    this->requests_file_ = requests_file;
    this->responses_file_ = responses_file;
    this->additions_file_ = additions_file;
    this->deletions_file_ = deletions_file;

    this->queue_length_ = 0;
    this->skew_ = skew;
    this->batch_size_ = batch_size;
    this->fetch_length_ = fetch_length;

    // generate_randoms();
    if (query_file != "") {
      ReadQueries(query_file);
    }

    GenerateRandoms();

    if (randoms_.empty() && queries_.empty()) {
      fprintf(stderr, "Warning: No search or get queries loaded.\n");
    }

    ParseConfigFile(config_file);
  }

  static void SendRequests(AdaptiveQueryServiceClient *query_client,
                           boost::shared_ptr<TTransport> query_transport,
                           std::vector<std::string> queries,
                           std::vector<int64_t> query_ids,
                           std::vector<int64_t> randoms,
                           std::vector<uint32_t> request_rates,
                           std::vector<uint32_t> durations,
                           std::atomic<uint64_t> &queue_length,
                           std::string reqfile) {

    TimeStamp cur_time;
    const TimeStamp MEASURE_INTERVAL = 40000000;
    TimeStamp measure_start_time = GetTimestamp();
    std::ofstream req_stream(reqfile);
    uint64_t num_requests = 0;

    for (uint32_t stage = 0; stage < request_rates.size(); stage++) {
      TimeStamp duration = ((uint64_t) durations[stage]) * 1000L * 1000L;  // Seconds to microseconds
      TimeStamp sleep_time = (1000 * 1000) / request_rates[stage];
      uint64_t i = 0;
      fprintf(
          stderr,
          "[REQTH] Starting stage %u: request-rate = %u Ops/sec, duration = %llu us\n",
          stage, request_rates[stage], duration);
      TimeStamp start_time = GetTimestamp();
      while ((cur_time = GetTimestamp()) - start_time <= duration) {
        TimeStamp t0 = GetTimestamp();
        query_client->send_search(queries[query_ids[i % query_ids.size()]]);
        i++;
        num_requests++;
        queue_length++;
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
      fprintf(stderr, "[REQTH] Finished stage %u, spent %llu us.\n", stage,
              (cur_time - start_time));
    }
    TimeStamp diff = cur_time - measure_start_time;
    double rr = ((double) num_requests * 1000 * 1000) / ((double) diff);
    req_stream << cur_time << "\t" << rr << "\n";
    req_stream.close();

    // Sleep for some time and let other threads finish
    fprintf(stderr, "[REQTH] Request thread sleeping for 10 seconds...\n");
    sleep(10);
    fprintf(
        stderr,
        "[REQTH] Finished sending queries, attempting to close query socket...\n");
    query_transport->close();
    fprintf(stderr, "[REQTH] Closed query socket.\n");
  }

  static void MeasureResponses(AdaptiveQueryServiceClient *query_client,
                               AdaptiveQueryServiceClient *stats_client,
                               std::atomic<uint64_t> &queue_length,
                               std::string results_file) {

    const TimeStamp MEASURE_INTERVAL = 40000000;
    uint32_t num_responses = 0;
    TimeStamp cur_time;
    std::ofstream res_stream(results_file);
    TimeStamp measure_start_time = GetTimestamp();
    while (true) {
      try {
        std::set<int64_t> res;
        query_client->recv_search(res);
        num_responses++;
        queue_length--;
        if ((cur_time = GetTimestamp()) - measure_start_time
            >= MEASURE_INTERVAL) {
          TimeStamp diff = cur_time - measure_start_time;
          double thput = ((double) num_responses * 1000 * 1000)
              / ((double) diff);
          res_stream << cur_time << "\t" << thput << "\t"
              << stats_client->storage_size() << "\t" << queue_length << "\n";
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
    res_stream << cur_time << "\t" << thput << "\t"
               << stats_client->storage_size() << "\t"
               << stats_client->num_sampled_values() << "\n";
    res_stream.close();
  }

  static void ManageLayers(AdaptiveQueryServiceClient *management_client,
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
              layers_to_create[stage][i]);
          fprintf(stderr, "[MGMTH] Created layer with size = %zu\n", add_size);
          add_stream << GetTimestamp() << "\t" << i << "\t" << add_size << "\n";
          add_stream.flush();
        } catch (std::exception& e) {
          break;
        }
      }

      for (size_t i = 0; i < layers_to_delete[stage].size(); i++) {
        try {
          size_t del_size = management_client->remove_layer(
              layers_to_delete[stage][i]);
          fprintf(stderr, "[MGMTH] Deleted layer with size = %zu\n", del_size);
          del_stream << GetTimestamp() << "\t" << i << "\t" << del_size << "\n";
          del_stream.flush();
        } catch (std::exception& e) {
          fprintf(stderr, "[MGMTH] Error: %s\n", e.what());
          break;
        }
      }
      // Sleep if there is still time left
      if ((cur_time = GetTimestamp()) - start_time < duration) {
        fprintf(
            stderr,
            "[MGMTH] Done with layer management for stage %u, took %llu us, sleeping for %llu us.\n",
            stage, (cur_time - start_time),
            (duration - (cur_time - start_time)));
        usleep(duration - (cur_time - start_time));
      }
    }
  }

  void RunBenchmark() {
    std::thread request_thread(&AdaptBenchmark::SendRequests, query_client_,
                               query_transport_, queries_, query_ids_, randoms_,
                               request_rates_, durations_,
                               std::ref(queue_length_), requests_file_);
    std::thread response_thread(&AdaptBenchmark::MeasureResponses,
                                query_client_, stats_client_,
                                std::ref(queue_length_), responses_file_);
    std::thread layer_management_thread(&AdaptBenchmark::ManageLayers,
                                        management_client_, layers_to_create_,
                                        layers_to_delete_, durations_,
                                        additions_file_, deletions_file_);

    if (request_thread.joinable()) {
      request_thread.join();
      fprintf(stderr, "Request thread terminated.\n");
    }

    if (response_thread.joinable()) {
      response_thread.join();
      fprintf(stderr, "Response thread terminated.\n");
    }

    if (layer_management_thread.joinable()) {
      layer_management_thread.join();
      fprintf(stderr, "Layer management thread terminated.\n");
    }

    management_transport_->close();
    fprintf(stderr, "Closed management socket.\n");

    stats_transport_->close();
    fprintf(stderr, "Closed stats socket.\n");
  }

 private:
  void GenerateRandoms() {
    uint64_t query_count = query_client_->get_num_keys();
    fprintf(stderr, "Generating zipf distribution with theta=%f, N=%llu...\n",
            skew_, query_count);
    ZipfGenerator z(skew_, query_count);
    fprintf(stderr, "Generated zipf distribution, generating keys...\n");
    for (uint64_t i = 0; i < query_count; i++) {
      randoms_.push_back(z.Next());
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
    fprintf(stderr, "Generating zipf distribution with theta=%f, N=%zu...\n",
                skew_, queries_.size());
    ZipfGenerator z(skew_, queries_.size());
    fprintf(stderr, "Generated zipf distribution, generating query ids...\n");
    for (uint64_t i = 0; i < queries_.size(); i++) {
      query_ids_.push_back(z.Next());
    }
    fprintf(stderr, "Generated query ids.\n");
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

  void ParseConfigFile(std::string configfile) {
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

  AdaptiveQueryServiceClient *GetClient(
      boost::shared_ptr<TTransport> &client_transport) {
    int port = QUERY_SERVER_PORT;
    boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    AdaptiveQueryServiceClient *client = new AdaptiveQueryServiceClient(
        protocol);
    transport->open();
    client_transport = transport;
    return client;
  }

  AdaptiveQueryServiceClient *query_client_;
  AdaptiveQueryServiceClient *stats_client_;
  AdaptiveQueryServiceClient *management_client_;
  boost::shared_ptr<TTransport> query_transport_;
  boost::shared_ptr<TTransport> stats_transport_;
  boost::shared_ptr<TTransport> management_transport_;
  std::vector<uint32_t> request_rates_;
  std::vector<uint32_t> durations_;
  std::vector<std::vector<uint32_t>> layers_to_delete_;
  std::vector<std::vector<uint32_t>> layers_to_create_;
  std::string requests_file_;
  std::string responses_file_;
  std::string additions_file_;
  std::string deletions_file_;
  std::atomic<uint64_t> queue_length_;
  double skew_;
  uint32_t batch_size_;
  uint32_t fetch_length_;

  std::vector<int64_t> query_ids_;

};

#endif
