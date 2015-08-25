#ifndef SUCCINCT_SERVER_BENCHMARK_H
#define SUCCINCT_SERVER_BENCHMARK_H

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include "succinct_shard.h"
#include "benchmark.h"
#include "AggregatorService.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class SuccinctBenchmark : public Benchmark {
 public:

  SuccinctBenchmark(std::string benchmark_type, std::string query_file = "")
      : Benchmark() {
    benchmark_type_ = benchmark_type;
    int port = AGGREGATOR_PORT;

    fprintf(stderr, "Connecting to server...\n");
    boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    client_ = new AggregatorServiceClient(protocol);
    transport->open();
    fprintf(stderr, "Connected!\n");
    client_->ConnectToServers();

    uint64_t tot_size = client_->GetTotSize();
    GenerateRandoms(tot_size);
    if (query_file != "") {
      ReadQueries(query_file);
    }
  }

  void BenchmarkRegexLatency(std::string result_path) {
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
        client_->Regex(result, queries_[i]);
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

  void BenchmarkExtractLatency(std::string result_path, int32_t fetch_length) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);
    int64_t tot_size = client_->GetTotSize();
    fprintf(stderr, "Tot size = %lld\n", tot_size);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      int64_t pos = llrand() % (tot_size - fetch_length);
      client_->Extract(result, pos, fetch_length);
      sum = (sum + result.length()) % kMaxSum;
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      int64_t pos = llrand() % (tot_size - fetch_length);
      t0 = GetTimestamp();
      client_->Extract(result, pos, fetch_length);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << tdiff << "\n";
      sum = (sum + result.length()) % kMaxSum;
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkCountLatency(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %lu queries...\n", std::min(queries_.size(), 100UL));
    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
      uint64_t result;
      result = client_->Count(queries_[i]);
      sum = (sum + result) % kMaxSum;
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %lu queries...\n", queries_.size());
    for (uint64_t i = 0; i < queries_.size(); i++) {
      uint64_t result;
      t0 = GetTimestamp();
      result = client_->Count(queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result << "\t" << tdiff << "\n";
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
    fprintf(stderr, "Warming up for %lu queries...\n", std::min(queries_.size(), 100UL));
    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
      std::vector<int64_t> result;
      client_->Search(result, queries_[i]);
      sum = (sum + result.size()) % kMaxSum;
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %lu queries...\n", queries_.size());
    for (uint64_t i = 0; i < queries_.size(); i++) {
      std::vector<int64_t> result;
      t0 = GetTimestamp();
      client_->Search(result, queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result.size() << "\t" << tdiff << "\n";
      sum = (sum + result.size()) % kMaxSum;
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();
  }

 private:
  static const uint64_t kMaxSum = 10000;

  unsigned long long llrand() {
    unsigned long long r = 0;

    for (int i = 0; i < 5; ++i) {
      r = (r << 15) | (rand() & 0x7FFF);
    }

    return r & 0xFFFFFFFFFFFFFFFFULL;
  }

  void GenerateRandoms(uint64_t tot_size) {
    uint64_t query_count = kWarmupCount + kMeasureCount;

    fprintf(stderr, "Generating random keys...\n");

    for (uint64_t i = 0; i < query_count; i++) {
      int64_t rand_num = llrand() % tot_size;
      randoms_.push_back(rand_num);
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
  AggregatorServiceClient *client_;
};

#endif
