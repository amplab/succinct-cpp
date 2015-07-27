#ifndef SHARD_BENCHMARK_H
#define SHARD_BENCHMARK_H

#include "succinct_shard.h"
#include "benchmark.h"

class ShardBenchmark : public Benchmark {
 public:
  ShardBenchmark(SuccinctShard *shard, std::string query_file = "")
      : Benchmark() {
    shard_ = shard;

    GenerateRandoms();
    if (query_file != "") {
      ReadQueries(query_file);
    }
  }

  void BenchmarkLookupFunction(uint64_t (SuccinctShard::*function)(uint64_t),
                               std::string result_path) {
    TimeStamp t0, t1, tdiff;
    uint64_t result;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %lu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      result = (shard_->*function)(randoms_[i]);
      sum = (sum + result) % shard_->original_size();
    }
    fprintf(stderr, "Warmup chksum = %lu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %lu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      t0 = rdtsc();
      result = (shard_->*function)(randoms_[i]);
      t1 = rdtsc();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << result << "\t" << tdiff << "\n";
      sum = (sum + result) % shard_->original_size();
    }
    fprintf(stderr, "Measure chksum = %lu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    // Cooldown
    sum = 0;
    fprintf(stderr, "Cooling down for %lu queries...\n", kCooldownCount);
    for (uint64_t i = kWarmupCount + kMeasureCount; i < randoms_.size(); i++) {
      result = (shard_->*function)(randoms_[i]);
      sum = (sum + result) % shard_->original_size();
    }
    fprintf(stderr, "Cooldown chksum = %lu\n", sum);
    fprintf(stderr, "Cooldown complete.\n");

    result_stream.close();

  }

  void BenchmarkGetLatency(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %lu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      shard_->get(result, randoms_[i]);
      sum = (sum + result.length()) % shard_->original_size();
    }
    fprintf(stderr, "Warmup chksum = %lu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %lu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = GetTimestamp();
      shard_->get(result, randoms_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << result.length() << "\t" << tdiff
                    << "\n";
      sum = (sum + result.length()) % shard_->original_size();
    }
    fprintf(stderr, "Measure chksum = %lu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    // Cooldown
    sum = 0;
    fprintf(stderr, "Cooling down for %lu queries...\n", kCooldownCount);
    for (uint64_t i = kWarmupCount + kMeasureCount; i < randoms_.size(); i++) {
      std::string result;
      shard_->get(result, randoms_[i]);
      sum = (sum + result.length()) % shard_->original_size();
    }
    fprintf(stderr, "Cooldown chksum = %lu\n", sum);
    fprintf(stderr, "Cooldown complete.\n");

    result_stream.close();

  }

  void BenchmarkAccessLatency(std::string result_path, int32_t len) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %lu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      shard_->access(result, randoms_[i], 0, len);
      sum = (sum + result.length()) % shard_->original_size();
    }
    fprintf(stderr, "Warmup chksum = %lu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %lu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = GetTimestamp();
      shard_->access(result, randoms_[i], 0, len);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << tdiff << "\n";
      sum = (sum + result.length()) % shard_->original_size();
    }
    fprintf(stderr, "Measure chksum = %lu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    // Cooldown
    sum = 0;
    fprintf(stderr, "Cooling down for %lu queries...\n", kCooldownCount);
    for (uint64_t i = kWarmupCount + kMeasureCount; i < randoms_.size(); i++) {
      std::string result;
      shard_->access(result, randoms_[i], 0, len);
      sum = (sum + result.length()) % shard_->original_size();
    }
    fprintf(stderr, "Cooldown chksum = %lu\n", sum);
    fprintf(stderr, "Cooldown complete.\n");

    result_stream.close();

  }

  void BenchmarkCountLatency(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %lu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      uint64_t result;
      result = shard_->count(queries_[i]);
      sum = (sum + result) % shard_->original_size();
    }
    fprintf(stderr, "Warmup chksum = %lu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %lu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      uint64_t result;
      t0 = GetTimestamp();
      result = shard_->count(queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << queries_[i] << "\t" << tdiff << "\n";
      sum = (sum + result) % shard_->original_size();
    }
    fprintf(stderr, "Measure chksum = %lu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    // Cooldown
    sum = 0;
    fprintf(stderr, "Cooling down for %lu queries...\n", kCooldownCount);
    for (uint64_t i = kWarmupCount + kMeasureCount; i < randoms_.size(); i++) {
      uint64_t result;
      result = shard_->count(queries_[i]);
      sum = (sum + result) % shard_->original_size();
    }
    fprintf(stderr, "Cooldown chksum = %lu\n", sum);
    fprintf(stderr, "Cooldown complete.\n");

    result_stream.close();

  }

  void BenchmarkSearchLatency(std::string result_path) {
    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %lu queries_...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount / 100; i++) {
      std::set<int64_t> result;
      shard_->search(result, queries_[i]);
      sum = (sum + result.size()) % shard_->original_size();
    }
    fprintf(stderr, "Warmup chksum = %lu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %lu queries_...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount / 100;
        i < (kWarmupCount + kMeasureCount) / 100; i++) {
      std::set<int64_t> result;
      t0 = GetTimestamp();
      shard_->search(result, queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result.size() << "\t" << tdiff << "\n";
      sum = (sum + result.size()) % shard_->original_size();
    }
    fprintf(stderr, "Measure chksum = %lu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    // Cooldown
    sum = 0;
    fprintf(stderr, "Cooling down for %lu queries_...\n", kCooldownCount);
    for (uint64_t i = kWarmupCount + kMeasureCount;
        i < (kWarmupCount + kMeasureCount + kCooldownCount) / 100; i++) {
      std::set<int64_t> result;
      shard_->search(result, queries_[i]);
      sum = (sum + result.size()) % shard_->original_size();
    }
    fprintf(stderr, "Cooldown chksum = %lu\n", sum);
    fprintf(stderr, "Cooldown complete.\n");

    result_stream.close();
  }

  void BenchmarkGetThroughput() {
    double thput = 0;
    std::string value;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        shard_->get(value, randoms_[i % randoms_.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        shard_->get(value, randoms_[i % randoms_.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      // Cooldown phase
      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        shard_->get(value, randoms_[i % randoms_.size()]);
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
  }

  void BenchmarkAccessThroughput(int32_t fetch_length) {
    double thput = 0;
    std::string value;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        shard_->access(value, randoms_[i % randoms_.size()], 0, fetch_length);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        shard_->access(value, randoms_[i % randoms_.size()], 0, fetch_length);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      // Cooldown phase
      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        shard_->access(value, randoms_[i % randoms_.size()], 0, fetch_length);
        i++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Access throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("throughput_results_access",
             std::ofstream::out | std::ofstream::app);
    ofs << thput << "\n";
    ofs.close();
  }

  void BenchmarkCountThrougput() {
    double thput = 0;
    int64_t value;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        value = shard_->count(queries_[i % randoms_.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        value = shard_->count(queries_[i % randoms_.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      // Cooldown phase
      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        value = shard_->count(queries_[i % randoms_.size()]);
        i++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Count throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("throughput_results_count",
             std::ofstream::out | std::ofstream::app);
    ofs << thput << "\n";
    ofs.close();
  }

  void BenchmarkSearchThroughput() {
    double thput = 0;
    std::string value;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        shard_->get(value, randoms_[i % randoms_.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        shard_->get(value, randoms_[i % randoms_.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      // Cooldown phase
      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        shard_->get(value, randoms_[i % randoms_.size()]);
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
  }

 private:
  void GenerateRandoms() {
    uint64_t q_cnt = kWarmupCount + kCooldownCount + kMeasureCount;
    for (uint64_t i = 0; i < q_cnt; i++) {
      randoms_.push_back(rand() % shard_->num_keys());
    }
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

  std::vector<uint64_t> randoms_;
  std::vector<std::string> queries_;
  SuccinctShard *shard_;
};

#endif
