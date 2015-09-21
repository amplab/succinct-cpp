#ifndef ADAPTIVE_QUERY_SERVER_BENCHMARK_HPP
#define ADAPTIVE_QUERY_SERVER_BENCHMARK_HPP

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

class AdaptiveQueryServerBenchmark : public Benchmark {
 public:
  AdaptiveQueryServerBenchmark(std::string requests_file,
                               std::string responses_file, double skew,
                               int32_t fetch_length,
                               std::string query_file = "")
      : Benchmark() {

    this->requests_file_ = requests_file;
    this->responses_file_ = responses_file;
    this->key_skew_ = skew;
    this->fetch_length_skew_ = 1.0;  // Pure uniform for now

    this->fetch_length_ = fetch_length;

    GenerateRandoms();
    // GenerateLengths();
    if (query_file != "") {
      ReadQueries(query_file);
    }
  }

  static void SendSearchRequests(AdaptiveQueryServiceClient *query_client,
                                 int64_t storage_size,
                                 std::vector<std::string> queries,
                                 std::string requests_file) {

    std::ofstream req_stream(requests_file,
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_requests = 0;

    TimeStamp start_time = GetTimestamp();
    while (num_requests <= queries.size()) {
      query_client->send_search(queries[num_requests % queries.size()]);
      num_requests++;
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double rr = ((double) num_requests * 1000 * 1000) / ((double) diff);
    req_stream << storage_size << "\t" << rr << "\n";
    req_stream.close();
  }

  static void MeasureSearchResponses(AdaptiveQueryServiceClient *query_client,
                                     int64_t storage_size,
                                     std::vector<std::string> queries,
                                     std::string responses_file) {

    std::ofstream res_stream(responses_file,
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_responses = 0;
    TimeStamp start_time = GetTimestamp();
    while (num_responses <= queries.size()) {
      try {
        std::set<int64_t> res;
        query_client->recv_search(res);
        num_responses++;
      } catch (std::exception& e) {
        break;
      }
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double rr = ((double) num_responses * 1000 * 1000) / ((double) diff);
    res_stream << storage_size << "\t" << rr << "\n";
    res_stream.close();
  }

  static void SendSearchGetRequests(AdaptiveQueryServiceClient *query_client,
                                    int64_t storage_size,
                                    std::vector<std::string> queries,
                                    std::vector<int64_t> keys,
                                    std::string requests_file) {

    std::ofstream req_stream(requests_file,
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_requests = 0, i = 0, j = 0;

    TimeStamp start_time = GetTimestamp();
    while (num_requests <= (queries.size() + keys.size())) {
      if (num_requests % 2 == 0) {
        query_client->send_search(queries[i % queries.size()]);
        i++;
      } else {
        query_client->send_get(keys[j % queries.size()]);
        j++;
      }
      num_requests++;
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double rr = ((double) num_requests * 1000 * 1000) / ((double) diff);
    req_stream << storage_size << "\t" << rr << "\n";
    req_stream.close();
  }

  static void MeasureSearchGetResponses(
      AdaptiveQueryServiceClient *query_client, int64_t storage_size,
      std::vector<std::string> queries, std::vector<int64_t> keys,
      std::string responses_file) {

    std::ofstream res_stream(responses_file,
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_responses = 0;
    TimeStamp start_time = GetTimestamp();
    while (num_responses <= (queries.size() + keys.size())) {
      try {
        if (num_responses % 2 == 0) {
          std::set<int64_t> res;
          query_client->recv_search(res);
        } else {
          std::string res;
          query_client->recv_get(res);
        }
        num_responses++;
      } catch (std::exception& e) {
        break;
      }
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double rr = ((double) num_responses * 1000 * 1000) / ((double) diff);
    res_stream << storage_size << "\t" << rr << "\n";
    res_stream.close();
  }

  static void SendAccessRequests(AdaptiveQueryServiceClient *query_client,
                                 int64_t storage_size,
                                 std::vector<int64_t> randoms,
                                 uint32_t fetch_length,
                                 std::string requests_file) {

    std::ofstream req_stream(requests_file,
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_requests = 0;

    TimeStamp start_time = GetTimestamp();
    while (num_requests <= randoms.size()) {
      query_client->send_access(randoms[num_requests % randoms.size()], 0,
                                fetch_length);
      num_requests++;
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double rr = ((double) num_requests * 1000 * 1000) / ((double) diff);
    req_stream << storage_size << "\t" << rr << "\n";
    req_stream.close();
  }

  static void MeasureAccessResponses(AdaptiveQueryServiceClient *query_client,
                                     int64_t storage_size,
                                     std::vector<int64_t> randoms,
                                     std::string responses_file) {

    std::string res;
    std::ofstream res_stream(responses_file,
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_responses = 0;
    TimeStamp start_time = GetTimestamp();
    while (num_responses <= randoms.size()) {
      try {
        query_client->recv_access(res);
        num_responses++;
      } catch (std::exception& e) {
        break;
      }
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double rr = ((double) num_responses * 1000 * 1000) / ((double) diff);
    res_stream << storage_size << "\t" << rr << "\n";
    res_stream.close();
  }

  static void SendBatchedAccessRequests(
      AdaptiveQueryServiceClient *query_client, int64_t storage_size,
      std::vector<int64_t> randoms, uint32_t fetch_length, uint32_t batch_size,
      std::string requests_file) {

    std::ofstream req_stream(requests_file,
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_requests = 0;

    TimeStamp start_time = GetTimestamp();
    while (num_requests <= randoms.size()) {
      // Prepare batch
      std::vector<int64_t> keys;
      for (uint32_t i = 0; i < batch_size; i++) {
        keys.push_back(randoms[(num_requests + i) % randoms.size()]);
      }
      query_client->send_batch_access(keys, 0, fetch_length);
      num_requests += batch_size;
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double rr = ((double) num_requests * 1000 * 1000) / ((double) diff);
    req_stream << storage_size << "\t" << rr << "\n";
    req_stream.close();
  }

  static void MeasureBatchedAccessResponses(
      AdaptiveQueryServiceClient *query_client, int64_t storage_size,
      std::vector<int64_t> randoms, uint32_t batch_size,
      std::string responses_file) {

    std::ofstream res_stream(responses_file,
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_responses = 0;
    TimeStamp start_time = GetTimestamp();
    while (num_responses <= randoms.size()) {
      try {
        std::vector<std::string> res;
        res.resize(batch_size);
        query_client->recv_batch_access(res);
        num_responses += res.size();
        res.clear();
      } catch (std::exception& e) {
        break;
      }
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double rr = ((double) num_responses * 1000 * 1000) / ((double) diff);
    res_stream << storage_size << "\t" << rr << "\n";
    res_stream.close();
  }

  void DeleteLayer(int32_t layer_to_delete) {
    boost::shared_ptr<TTransport> query_transport;
    AdaptiveQueryServiceClient *query_client = GetClient(query_transport);

    if (layer_to_delete != -1) {
      fprintf(stderr, "Removing layer %d\n", layer_to_delete);
      query_client->remove_layer(layer_to_delete);
      fprintf(stderr, "Removed layer %d\n", layer_to_delete);
    }

    query_transport->close();
    delete query_client;
  }

  void MeasureSearchThroughput() {

    boost::shared_ptr<TTransport> query_transport;
    AdaptiveQueryServiceClient *query_client = GetClient(query_transport);

    int64_t storage_size = query_client->storage_size();

    fprintf(stderr, "Starting request thread...\n");
    std::thread req(&AdaptiveQueryServerBenchmark::SendSearchRequests,
                    query_client, storage_size, queries_, requests_file_);

    fprintf(stderr, "Starting response thread...\n");
    std::thread res(&AdaptiveQueryServerBenchmark::MeasureSearchResponses,
                    query_client, storage_size, queries_, responses_file_);

    if (req.joinable()) {
      req.join();
      fprintf(stderr, "Request thread terminated.\n");
    }

    if (res.joinable()) {
      res.join();
      fprintf(stderr, "Response thread terminated.\n");
    }

    query_transport->close();
    delete query_client;
  }

  void MeasureSearchGetThroughput() {

    boost::shared_ptr<TTransport> query_transport;
    AdaptiveQueryServiceClient *query_client = GetClient(query_transport);

    int64_t storage_size = query_client->storage_size();

    fprintf(stderr, "Starting request thread...\n");
    std::thread req(&AdaptiveQueryServerBenchmark::SendSearchGetRequests,
                    query_client, storage_size, queries_, randoms_,
                    requests_file_);

    fprintf(stderr, "Starting response thread...\n");
    std::thread res(&AdaptiveQueryServerBenchmark::MeasureSearchGetResponses,
                    query_client, storage_size, queries_, randoms_,
                    responses_file_);

    if (req.joinable()) {
      req.join();
      fprintf(stderr, "Request thread terminated.\n");
    }

    if (res.joinable()) {
      res.join();
      fprintf(stderr, "Response thread terminated.\n");
    }

    query_transport->close();
    delete query_client;
  }

  void MeasureAccessThroughput() {

    boost::shared_ptr<TTransport> query_transport;
    AdaptiveQueryServiceClient *query_client = GetClient(query_transport);

    int64_t storage_size = query_client->storage_size();

    fprintf(stderr, "Starting request thread...\n");
    std::thread req(&AdaptiveQueryServerBenchmark::SendAccessRequests,
                    query_client, storage_size, randoms_, fetch_length_,
                    requests_file_);

    fprintf(stderr, "Starting response thread...\n");
    std::thread res(&AdaptiveQueryServerBenchmark::MeasureAccessResponses,
                    query_client, storage_size, randoms_, responses_file_);

    if (req.joinable()) {
      req.join();
      fprintf(stderr, "Request thread terminated.\n");
    }

    if (res.joinable()) {
      res.join();
      fprintf(stderr, "Response thread terminated.\n");
    }

    query_transport->close();
    delete query_client;
  }

  void MeasureBatchedAccessThroughput(int32_t batch_size) {
    boost::shared_ptr<TTransport> query_transport;
    AdaptiveQueryServiceClient *query_client = GetClient(query_transport);

    int64_t storage_size = query_client->storage_size();

    fprintf(stderr, "Starting request thread...\n");
    std::thread req(&AdaptiveQueryServerBenchmark::SendBatchedAccessRequests,
                    query_client, storage_size, randoms_, fetch_length_,
                    batch_size,
                    requests_file_ + ".batch." + std::to_string(batch_size));

    fprintf(stderr, "Starting response thread...\n");
    std::thread res(
        &AdaptiveQueryServerBenchmark::MeasureBatchedAccessResponses,
        query_client, storage_size, randoms_, batch_size,
        responses_file_ + ".batch." + std::to_string(batch_size));

    if (req.joinable()) {
      req.join();
      fprintf(stderr, "Request thread terminated.\n");
    }

    if (res.joinable()) {
      res.join();
      fprintf(stderr, "Response thread terminated.\n");
    }

    query_transport->close();
    delete query_client;
  }

 private:
  void GenerateRandoms() {
    boost::shared_ptr<TTransport> transport;
    AdaptiveQueryServiceClient *client = GetClient(transport);
    uint64_t q_cnt = client->get_num_keys();
    fprintf(stderr, "Generating zipf distribution with theta=%f, N=%llu...\n",
            key_skew_, q_cnt);
    ZipfGenerator z(key_skew_, q_cnt);
    fprintf(stderr, "Generated zipf distribution, generating keys...\n");
    for (uint64_t i = 0; i < 100000; i++) {
      randoms_.push_back(z.Next());
    }
    fprintf(stderr, "Generated keys.\n");
    transport->close();
    delete client;
  }

  void GenerateLengths() {
    int32_t min_len = 10;
    int32_t max_len = 1000;
    fprintf(stderr, "Generating zipf distribution with theta=%f, N=%u...\n",
            fetch_length_skew_, (max_len - min_len));
    ZipfGenerator z(fetch_length_skew_, max_len - min_len);
    fprintf(stderr, "Generated zipf distribution, generating lengths...\n");

    for (uint64_t i = 0; i < 100000; i++) {
      // Map zipf value to a length
      int32_t len = z.Next() + min_len;
      assert(len >= min_len);
      assert(len <= max_len);
      lengths_.push_back(len);
    }
    fprintf(stderr, "Generated lengths.\n");
  }

  void ReadQueries(std::string filename) {
    std::ifstream inputfile(filename);
    if (!inputfile.is_open()) {
      fprintf(stderr, "Error: Query file [%s] may be missing.\n",
              filename.c_str());
      return;
    }

    std::string line, bin, query;
    std::vector<std::string> queries_temp;
    while (getline(inputfile, line)) {
      // Extract key and value
      int split_index = line.find_first_of('\t');
      bin = line.substr(0, split_index);
      query = line.substr(split_index + 1);
      queries_temp.push_back(query);
    }
    inputfile.close();
    fprintf(stderr, "Generating zipf distribution with theta=%f, N=%zu...\n",
            key_skew_, queries_.size());
    ZipfGenerator z(key_skew_, queries_temp.size());
    fprintf(stderr, "Generated zipf distribution, generating query ids...\n");
    for (uint64_t i = 0; i < 100000; i++) {
      queries_.push_back(queries_temp[z.Next()]);
    }
    queries_temp.clear();
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

  std::vector<int32_t> lengths_;
  std::string requests_file_;
  std::string responses_file_;
  double key_skew_;
  double fetch_length_skew_;
  int32_t fetch_length_;
};

#endif
