#ifndef SUCCINCT_SERVER_BENCHMARK_H
#define SUCCINCT_SERVER_BENCHMARK_H

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include "succinct_shard.h"
#include "benchmark.h"
#include "SuccinctService.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class SuccinctServerBenchmark : public Benchmark {
 public:

  SuccinctServerBenchmark(std::string benchmark_type, uint32_t num_shards,
                          uint32_t num_keys, std::string query_file = "")
      : Benchmark() {
    benchmark_type_ = benchmark_type;
    int port = QUERY_HANDLER_PORT;

    if (!benchmark_type.compare(0, 7, "latency")) {
      fprintf(stderr, "Connecting to server...\n");
      boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
      boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
      boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
      client_ = new SuccinctServiceClient(protocol);
      transport->open();
      fprintf(stderr, "Connected!\n");
      client_->connect_to_handlers();
    } else {
      client_ = NULL;
    }

    GenerateRandoms(num_shards, num_keys);
    if (query_file != "") {
      ReadQueries(query_file);
    }
  }

  void BenchmarkGetLatency(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      client_->get(result, randoms_[i]);
      sum = (sum + result.length()) % kMaxSum;
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = GetTimestamp();
      client_->get(result, randoms_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << tdiff << "\n";
      sum = (sum + result.length()) % kMaxSum;
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    // Cooldown
    sum = 0;
    fprintf(stderr, "Cooling down for %llu queries...\n", kCooldownCount);
    for (uint64_t i = kWarmupCount + kMeasureCount; i < randoms_.size(); i++) {
      std::string result;
      client_->get(result, randoms_[i]);
      sum = (sum + result.length()) % kMaxSum;
    }
    fprintf(stderr, "Cooldown chksum = %llu\n", sum);
    fprintf(stderr, "Cooldown complete.\n");

    result_stream.close();

  }

  void BenchmarkAccessLatency(std::string result_path, int32_t fetch_length) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      client_->access(result, randoms_[i], 0, fetch_length);
      sum = (sum + result.length()) % kMaxSum;
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = GetTimestamp();
      client_->access(result, randoms_[i], 0, fetch_length);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << tdiff << "\n";
      sum = (sum + result.length()) % kMaxSum;
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    // Cooldown
    sum = 0;
    fprintf(stderr, "Cooling down for %llu queries...\n", kCooldownCount);
    for (uint64_t i = kWarmupCount + kMeasureCount; i < randoms_.size(); i++) {
      std::string result;
      client_->access(result, randoms_[i], 0, fetch_length);
      sum = (sum + result.length()) % kMaxSum;
    }
    fprintf(stderr, "Cooldown chksum = %llu\n", sum);
    fprintf(stderr, "Cooldown complete.\n");

    result_stream.close();

  }

  void BenchmarkCountLatency(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
      uint64_t result;
      result = client_->count(queries_[i]);
      sum = (sum + result) % kMaxSum;
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < queries_.size(); i++) {
      uint64_t result;
      t0 = GetTimestamp();
      result = client_->count(queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << queries_[i] << "\t" << tdiff << "\n";
      sum = (sum + result) % kMaxSum;
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkSearchLatency(std::string result_path) {
    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
      std::set<int64_t> result;
      client_->search(result, queries_[i]);
      sum = (sum + result.size()) % kMaxSum;
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < queries_.size(); i++) {
      std::set<int64_t> result;
      t0 = GetTimestamp();
      client_->search(result, queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result.size() << "\t" << tdiff << "\n";
      sum = (sum + result.size()) % kMaxSum;
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();
  }

  void BenchmarkRegexCountLatency(std::string result_path) {
    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint32_t i = 0; i < queries_.size(); i++) {
      for (uint32_t j = 0; j < 10; j++) {
        fprintf(stderr, "Running iteration %u of query %u\n", j, i);
        std::vector<int64_t> result;
        t0 = GetTimestamp();
        client_->regex_count(result, queries_[i]);
        t1 = GetTimestamp();
        tdiff = t1 - t0;
        result_stream << i << "\t" << j << "\t";
        for (auto r : result) {
          result_stream << r << "\t";
        }
        result_stream << tdiff << "\n";
        result_stream.flush();
        sum = (sum + result.size()) % kMaxSum;
      }
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();
  }

  void BenchmarkRegexSearchLatency(std::string result_path) {
    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint32_t i = 0; i < queries_.size(); i++) {
      for (uint32_t j = 0; j < 10; j++) {
        fprintf(stderr, "Running iteration %u of query %u\n", j, i);
        std::set<int64_t> result;
        t0 = GetTimestamp();
        client_->regex_search(result, queries_[i]);
        t1 = GetTimestamp();
        tdiff = t1 - t0;
        result_stream << i << "\t" << j << "\t" << result.size() << "\t"
                      << tdiff << "\n";
        result_stream.flush();
        sum = (sum + result.size()) % kMaxSum;
      }
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();
  }

  static void *GetThroughput(void *ptr) {
    ThreadData data = *((ThreadData*) ptr);
    std::cout << "GET\n";

    SuccinctServiceClient client = *(data.client);
    std::string value;

    double thput = 0;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        client.get(value, data.randoms[i % data.randoms.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        client.get(value, data.randoms[i % data.randoms.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        client.get(value, data.randoms[i % data.randoms.size()]);
        i++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Get throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("throughput_results_get", std::ofstream::out | std::ofstream::app);
    ofs << thput << "\n";
    ofs.close();

    return 0;
  }

  int BenchmarkGetThroughput(uint32_t num_threads) {
    pthread_t thread[num_threads];
    std::vector<ThreadData> data;
    fprintf(stderr, "Starting all threads...\n");
    for (uint32_t i = 0; i < num_threads; i++) {
      try {
        boost::shared_ptr<TSocket> socket(
            new TSocket("localhost", QUERY_HANDLER_PORT));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        SuccinctServiceClient *client = new SuccinctServiceClient(protocol);
        transport->open();
        client->connect_to_handlers();
        ThreadData th_data;
        th_data.client = client;
        th_data.transport = transport;
        th_data.randoms = randoms_;
        data.push_back(th_data);
      } catch (std::exception& e) {
        fprintf(stderr, "Could not connect to handler on localhost : %s\n",
                e.what());
        return -1;
      }
    }
    fprintf(stderr, "Started %zu clients.\n", data.size());

    for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
      int result = 0;
      result = pthread_create(&thread[current_t], NULL,
                              SuccinctServerBenchmark::GetThroughput,
                              static_cast<void*>(&(data[current_t])));
      if (result != 0) {
        fprintf(stderr, "Error creating thread %d; return code = %d\n",
                current_t, result);
      }
    }

    for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
      pthread_join(thread[current_t], NULL);
    }
    fprintf(stderr, "All threads completed.\n");

    for (uint32_t i = 0; i < num_threads; i++) {
      data[i].transport->close();
    }

    data.clear();
    return 0;
  }

  static void *AccessThroughput(void *ptr) {
    ThreadData data = *((ThreadData*) ptr);
    std::cout << "ACCESS\n";

    SuccinctServiceClient client = *(data.client);
    std::string value;

    double thput = 0;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        client.access(value, data.randoms[i % data.randoms.size()], 0,
                      data.fetch_length);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        client.access(value, data.randoms[i % data.randoms.size()], 0,
                      data.fetch_length);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        client.access(value, data.randoms[i % data.randoms.size()], 0,
                      data.fetch_length);
        i++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Get throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("throughput_results_access",
             std::ofstream::out | std::ofstream::app);
    ofs << thput << "\n";
    ofs.close();

    return 0;
  }

  int BenchmarkAccessThroughput(uint32_t num_threads, int32_t len) {
    pthread_t thread[num_threads];
    std::vector<ThreadData> data;
    fprintf(stderr, "Starting all threads...\n");
    for (uint32_t i = 0; i < num_threads; i++) {
      try {
        boost::shared_ptr<TSocket> socket(
            new TSocket("localhost", QUERY_HANDLER_PORT));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        SuccinctServiceClient *client = new SuccinctServiceClient(protocol);
        transport->open();
        client->connect_to_handlers();
        ThreadData th_data;
        th_data.client = client;
        th_data.transport = transport;
        th_data.randoms = randoms_;
        th_data.fetch_length = len;
        data.push_back(th_data);
      } catch (std::exception& e) {
        fprintf(stderr, "Could not connect to handler on localhost : %s\n",
                e.what());
        return -1;
      }
    }
    fprintf(stderr, "Started %zu clients.\n", data.size());

    for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
      int result = 0;
      result = pthread_create(&thread[current_t], NULL,
                              SuccinctServerBenchmark::AccessThroughput,
                              static_cast<void*>(&(data[current_t])));
      if (result != 0) {
        fprintf(stderr, "Error creating thread %d; return code = %d\n",
                current_t, result);
      }
    }

    for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
      pthread_join(thread[current_t], NULL);
    }
    fprintf(stderr, "All threads completed.\n");

    for (uint32_t i = 0; i < num_threads; i++) {
      data[i].transport->close();
    }

    data.clear();
    return 0;
  }

 private:
  typedef struct {
    SuccinctServiceClient *client;
    boost::shared_ptr<TTransport> transport;
    std::vector<int64_t> randoms;
    int32_t fetch_length;
  } ThreadData;

  static const uint64_t kMaxSum = 10000;

  void GenerateRandoms(uint32_t num_shards, uint32_t num_keys) {
    uint64_t query_count = kWarmupCount + kCooldownCount + kMeasureCount;

    fprintf(stderr, "Generating random keys...\n");

    for (uint64_t i = 0; i < query_count; i++) {
      // Pick a host
      int64_t shard_id = rand() % num_shards;
      int64_t key = rand() % num_keys;
      randoms_.push_back(shard_id * SuccinctShard::MAX_KEYS + key);
    }
    fprintf(stderr, "Generated %llu random keys\n", query_count);
  }

  void ReadQueries(std::string filename) {
    std::ifstream inputfile(filename);
    if (!inputfile.is_open()) {
      fprintf(stderr, "Error: Query file [%s] may be missing.\n",
              filename.c_str());
      return;
    }

    std::string line, query, query_count;
    while (getline(inputfile, line)) {
      // Extract key and value
      int split_index = line.find_first_of('\t');
      query = line.substr(0, split_index);
      query_count = line.substr(split_index + 1);
      queries_.push_back(query);
    }
    inputfile.close();
  }

  std::vector<int64_t> randoms_;
  std::vector<std::string> queries_;
  std::string benchmark_type_;
  SuccinctServiceClient *client_;
};

#endif
