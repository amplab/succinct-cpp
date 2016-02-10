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
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      result = (shard_->*function)(randoms_[i]);
      sum = (sum + result) % shard_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      t0 = rdtsc();
      result = (shard_->*function)(randoms_[i]);
      t1 = rdtsc();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << result << "\t" << tdiff << "\n";
      sum = (sum + result) % shard_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

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
      shard_->Get(result, randoms_[i]);
      sum = (sum + result.length()) % shard_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = GetTimestamp();
      shard_->Get(result, randoms_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << result.length() << "\t" << tdiff
                    << "\n";
      sum = (sum + result.length()) % shard_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkAccessLatency(std::string result_path, int32_t len) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      shard_->Access(result, randoms_[i], 0, len);
      sum = (sum + result.length()) % shard_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = GetTimestamp();
      shard_->Access(result, randoms_[i], 0, len);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << tdiff << "\n";
      sum = (sum + result.length()) % shard_->GetOriginalSize();
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
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
      uint64_t result;
      result = shard_->Count(queries_[i]);
      sum = (sum + result) % shard_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < queries_.size(); i++) {
      uint64_t result;
      t0 = GetTimestamp();
      result = shard_->Count(queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result << "\t" << tdiff << "\n";
      sum = (sum + result) % shard_->GetOriginalSize();
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
    fprintf(stderr, "Warming up for %llu queries_...\n", kWarmupCount);
    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
      std::set<int64_t> result;
      shard_->Search(result, queries_[i]);
      sum = (sum + result.size()) % shard_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries_...\n", kMeasureCount);
    for (uint64_t i = 0; i < queries_.size(); i++) {
      std::set<int64_t> result;
      t0 = GetTimestamp();
      shard_->Search(result, queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result.size() << "\t" << tdiff << "\n";
      sum = (sum + result.size()) % shard_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();
  }

  static void *GetThroughput(void *ptr) {
    ThreadData data = *((ThreadData*) ptr);
    std::cout << "GET\n";

    SuccinctShard shard = *(data.shard);
    std::string value;

    double thput = 0;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        shard.Get(value, data.randoms[i % data.randoms.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        shard.Get(value, data.randoms[i % data.randoms.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        shard.Get(value, data.randoms[i % data.randoms.size()]);
        i++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Get throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("thput", std::ofstream::out | std::ofstream::app);
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
        ThreadData th_data;
        th_data.shard = shard_;
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
                              ShardBenchmark::GetThroughput,
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

    data.clear();
    return 0;
  }

  static void *SearchThroughput(void *ptr) {
    ThreadData data = *((ThreadData*) ptr);
    std::cout << "SEARCH\n";

    SuccinctShard shard = *(data.shard);

    double thput = 0;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        std::set<int64_t> res;
        shard.Search(res, data.queries[i % data.queries.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        std::set<int64_t> res;
        shard.Search(res, data.queries[i % data.queries.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        std::set<int64_t> res;
        shard.Search(res, data.queries[i % data.queries.size()]);
        i++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Search throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("thput", std::ofstream::out | std::ofstream::app);
    ofs << thput << "\n";
    ofs.close();

    return 0;
  }

  int BenchmarkSearchThroughput(uint32_t num_threads) {
    pthread_t thread[num_threads];
    std::vector<ThreadData> data;
    fprintf(stderr, "Starting all threads...\n");
    for (uint32_t i = 0; i < num_threads; i++) {
      try {
        ThreadData th_data;
        th_data.shard = shard_;
        th_data.queries = queries_;
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
                              ShardBenchmark::SearchThroughput,
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

    data.clear();
    return 0;
  }

 private:
  typedef struct {
    SuccinctShard *shard;
    std::vector<int64_t> randoms;
    std::vector<std::string> queries;
    int32_t fetch_length;
  } ThreadData;

  void GenerateRandoms() {
    uint64_t q_cnt = kWarmupCount + kCooldownCount + kMeasureCount;
    for (uint64_t i = 0; i < q_cnt; i++) {
      randoms_.push_back(rand() % shard_->GetNumKeys());
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
      query = line.substr(0, split_index);
      bin = line.substr(split_index + 1);
      queries_.push_back(query);
    }
    inputfile.close();
  }

  std::vector<int64_t> randoms_;
  std::vector<std::string> queries_;
  SuccinctShard *shard_;
};

#endif
