#ifndef FILE_BENCHMARK_HPP
#define FILE_BENCHMARK_HPP

#include "succinct_file.h"
#include "benchmark.h"

class FileBenchmark : public Benchmark {
 public:
  FileBenchmark(SuccinctFile *succinct_file, std::string queryfile = "")
      : Benchmark() {
    succinct_file_ = succinct_file;
    GenerateRandoms();
    if (queryfile != "") {
      ReadQueries(queryfile);
    }
  }

  void BenchmarkLookupFunction(uint64_t (SuccinctFile::*function)(uint64_t),
                               std::string res_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t result;
    uint64_t sum;
    std::ofstream result_stream(res_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      result = (succinct_file_->*function)(randoms_[i]);
      sum = (sum + result) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      t0 = rdtsc();
      result = (succinct_file_->*function)(randoms_[i]);
      t1 = rdtsc();
      tdiff = t1 - t0;
      result_stream << randoms_[i] << "\t" << result << "\t" << tdiff << "\n";
      sum = (sum + result) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();
  }

  void BenchmarkCountTicks(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t result;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    fprintf(stderr, "Warming up for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
      result = succinct_file_->Count(queries_[i]);
      sum = (sum + result) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < queries_.size(); i++) {
      t0 = rdtsc();
      result = succinct_file_->Count(queries_[i]);
      t1 = rdtsc();
      tdiff = t1 - t0;
      result_stream << result << "\t" << tdiff << "\n";
      sum = (sum + result) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkSearchTicks(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    fprintf(stderr, "Warming up for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
      std::vector<int64_t> result;
      succinct_file_->Search(result, queries_[i]);
      sum = (sum + result.size()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < queries_.size(); i++) {
      std::vector<int64_t> result;
      t0 = rdtsc();
      succinct_file_->Search(result, queries_[i]);
      t1 = rdtsc();
      tdiff = t1 - t0;
      result_stream << result.size() << "\t" << tdiff << "\n";
      sum = (sum + result.size()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkExtractTicks(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    uint64_t extract_length = 1000;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      succinct_file_->Extract(result, randoms_[i], extract_length);
      sum = (sum + result.length()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = rdtsc();
      succinct_file_->Extract(result, randoms_[i], extract_length);
      t1 = rdtsc();
      tdiff = t1 - t0;
      result_stream << result.length() << "\t" << tdiff << "\n";
      sum = (sum + result.length()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkCount(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t result;
    uint64_t sum;
    std::ofstream result_stream(result_path);

//    // Warmup
//    fprintf(stderr, "Warming up for %llu queries...\n", kMeasureCount);
//    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
//      result = succinct_file_->Count(queries_[i]);
//      sum = (sum + result) % succinct_file_->GetOriginalSize();
//    }
//    fprintf(stderr, "Warmup chksum = %llu\n", sum);
//    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < queries_.size(); i++) {
      t0 = GetTimestamp();
      result = succinct_file_->Count(queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result << "\t" << tdiff << "\n";
      result = succinct_file_->Count2(queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result << "\t" << tdiff << "\n";
      sum = (sum + result) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkSearch(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    std::ofstream result_stream(result_path);

    // Warmup
    fprintf(stderr, "Warming up for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < std::min(queries_.size(), 100UL); i++) {
      std::vector<int64_t> result;
      succinct_file_->Search(result, queries_[i]);
      sum = (sum + result.size()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = 0; i < queries_.size(); i++) {
      std::vector<int64_t> result;
      t0 = GetTimestamp();
      succinct_file_->Search(result, queries_[i]);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result.size() << "\t" << tdiff << "\n";
      sum = (sum + result.size()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkExtract(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    uint64_t extract_length = 1000;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      succinct_file_->Extract(result, randoms_[i], extract_length);
      sum = (sum + result.length()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = GetTimestamp();
      succinct_file_->Extract(result, randoms_[i], extract_length);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result.length() << "\t" << tdiff << "\n";
      sum = (sum + result.length()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkExtract2(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    uint64_t extract_length = 1000;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      succinct_file_->Extract2(result, randoms_[i], extract_length);
      sum = (sum + result.length()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = GetTimestamp();
      succinct_file_->Extract2(result, randoms_[i], extract_length);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result.length() << "\t" << tdiff << "\n";
      sum = (sum + result.length()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkExtract3(std::string result_path) {

    TimeStamp t0, t1, tdiff;
    uint64_t sum;
    uint64_t extract_length = 1000;
    std::ofstream result_stream(result_path);

    // Warmup
    sum = 0;
    fprintf(stderr, "Warming up for %llu queries...\n", kWarmupCount);
    for (uint64_t i = 0; i < kWarmupCount; i++) {
      std::string result;
      succinct_file_->Extract2(result, randoms_[i], extract_length);
      sum = (sum + result.length()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Warmup chksum = %llu\n", sum);
    fprintf(stderr, "Warmup complete.\n");

    // Measure
    sum = 0;
    fprintf(stderr, "Measuring for %llu queries...\n", kMeasureCount);
    for (uint64_t i = kWarmupCount; i < kWarmupCount + kMeasureCount; i++) {
      std::string result;
      t0 = GetTimestamp();
      succinct_file_->Extract3(result, randoms_[i], extract_length);
      t1 = GetTimestamp();
      tdiff = t1 - t0;
      result_stream << result.length() << "\t" << tdiff << "\n";
      sum = (sum + result.length()) % succinct_file_->GetOriginalSize();
    }
    fprintf(stderr, "Measure chksum = %llu\n", sum);
    fprintf(stderr, "Measure complete.\n");

    result_stream.close();

  }

  void BenchmarkCore() {
    fprintf(stderr, "Benchmarking Core Functions...\n\n");
    fprintf(stderr, "Benchmarking lookupNPA...\n");
    BenchmarkLookupFunction(&SuccinctFile::LookupNPA,
                            succinct_file_->Name() + ".npa");
    fprintf(stderr, "Done!\n\n");
    fprintf(stderr, "Benchmarking lookupSA...\n");
    BenchmarkLookupFunction(&SuccinctFile::LookupSA,
                            succinct_file_->Name() + ".sa");
    fprintf(stderr, "Done!\n\n");
    fprintf(stderr, "Benchmarking lookupISA...\n");
    BenchmarkLookupFunction(&SuccinctFile::LookupISA,
                            succinct_file_->Name() + ".isa");
    fprintf(stderr, "Done!\n\n");
  }

  void BenchmarkFile() {
    fprintf(stderr, "Benchmarking File Functions...\n\n");
    fprintf(stderr, "Benchmarking extract ticks...\n");
    BenchmarkExtractTicks(succinct_file_->Name() + ".extract.ticks");
    fprintf(stderr, "Done!\n\n");
    fprintf(stderr, "Benchmarking extract...\n");
    BenchmarkExtract(succinct_file_->Name() + ".extract");
    fprintf(stderr, "Done!\n\n");
    if (queries_.size() == 0) {
      fprintf(stderr, "[WARNING]: No queries have been loaded.\n");
      fprintf(stderr, "[WARNING]: Skipping count and search benchmarks.\n");
    } else {
      fprintf(stderr, "Benchmarking count ticks...\n");
      BenchmarkCountTicks(succinct_file_->Name() + ".count.ticks");
      fprintf(stderr, "Done!\n\n");
      fprintf(stderr, "Benchmarking search ticks...\n");
      BenchmarkSearchTicks(succinct_file_->Name() + ".search.ticks");
      fprintf(stderr, "Done!\n\n");
      fprintf(stderr, "Benchmarking count...\n");
      BenchmarkCount(succinct_file_->Name() + ".count");
      fprintf(stderr, "Done!\n\n");
      fprintf(stderr, "Benchmarking search...\n");
      BenchmarkSearch(succinct_file_->Name() + ".search");
      fprintf(stderr, "Done!\n\n");
    }
  }

 private:
  void GenerateRandoms() {
    uint64_t query_count = kWarmupCount + kCooldownCount + kMeasureCount;
    for (uint64_t i = 0; i < query_count; i++) {
      randoms_.push_back(rand() % succinct_file_->GetOriginalSize());
    }
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

  std::vector<uint64_t> randoms_;
  std::vector<std::string> queries_;
  SuccinctFile *succinct_file_;
};

#endif
