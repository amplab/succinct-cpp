#ifndef LAYERED_SUCCINCT_SHARD_BENCHMARK_HPP
#define LAYERED_SUCCINCT_SHARD_BENCHMARK_HPP

#include <thread>
#include <sstream>
#include <unistd.h>

#include "layered_succinct_shard.h"
#include "benchmark.h"
#include "zipf_generator.h"

class LayeredSuccinctShardBenchmark : public Benchmark {
 public:
  LayeredSuccinctShardBenchmark(std::string filename, bool construct,
                                uint32_t isa_sampling_rate,
                                uint32_t sa_sampling_rate,
                                std::string results_file, double skew,
                                std::string query_file = "")
      : Benchmark() {

    results_file_ = results_file;
    key_skew_ = skew;
    length_skew_ = 1.0;  // Pure uniform for now
    layered_succinct_shard_ = new LayeredSuccinctShard(
        0,
        filename,
        construct ?
            SuccinctMode::CONSTRUCT_IN_MEMORY : SuccinctMode::LOAD_IN_MEMORY,
        sa_sampling_rate, isa_sampling_rate);
    if (construct) {
      // Serialize and save to file
      layered_succinct_shard_->serialize();
    }

    GenerateRandoms();
    GenerateLengths();

    if (query_file != "") {
      ReadQueries(query_file);
    }
  }

  void MeasureAccessThroughput(uint32_t fetch_length) {
    fprintf(stderr, "Starting access throughput measurement...");
    size_t storage_size = layered_succinct_shard_->storage_size();
    std::string res;
    std::ofstream res_stream(results_file_ + ".access",
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_ops = 0;

    TimeStamp start_time = GetTimestamp();
    while (num_ops <= randoms_.size()) {
      layered_succinct_shard_->access(res, randoms_[num_ops % randoms_.size()],
                                      0, fetch_length);
      num_ops++;
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double throughput = ((double) num_ops * 1000 * 1000) / ((double) diff);
    res_stream << storage_size << "\t" << throughput << "\n";
    res_stream.close();
    fprintf(stderr, "Done.\n");
  }

  void MeasureGetThroughput() {
    fprintf(stderr, "Starting get throughput measurement...");
    size_t storage_size = layered_succinct_shard_->storage_size();
    std::string res;
    std::ofstream res_stream(results_file_ + ".get",
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_ops = 0;

    TimeStamp start_time = GetTimestamp();
    while (num_ops <= randoms_.size()) {
      layered_succinct_shard_->get(res, randoms_[num_ops % randoms_.size()]);
      num_ops++;
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double thput = ((double) num_ops * 1000 * 1000) / ((double) diff);
    res_stream << storage_size << "\t" << thput << "\n";
    res_stream.close();
    fprintf(stderr, "Done.\n");
  }

  void MeasureSearchThroughput() {
    fprintf(stderr, "Starting search throughput measurement...");
    size_t storage_size = layered_succinct_shard_->storage_size();
    std::set<int64_t> res;
    std::ofstream res_stream(results_file_ + ".search",
                             std::ofstream::out | std::ofstream::app);
    uint64_t num_ops = 0;

    TimeStamp start_time = GetTimestamp();
    while (num_ops <= randoms_.size()) {
      layered_succinct_shard_->search(res, queries_[num_ops % queries_.size()]);
      num_ops++;
    }
    TimeStamp diff = GetTimestamp() - start_time;
    double thput = ((double) num_ops * 1000 * 1000) / ((double) diff);
    res_stream << storage_size << "\t" << thput << "\n";
    res_stream.close();
    fprintf(stderr, "Done.\n");
  }

  void DeleteLayer(int32_t layer_id) {
    if (layer_id >= 0) {
      fprintf(stderr, "Deleting layer %d...\n", layer_id);
      layered_succinct_shard_->remove_layer(layer_id);
      fprintf(stderr, "Done.\n");
    }
  }

 private:
  void GenerateRandoms() {
    uint64_t q_cnt = layered_succinct_shard_->num_keys();
    fprintf(stderr, "Generating zipf distribution with theta=%f, N=%lu...\n",
            key_skew_, q_cnt);
    ZipfGenerator z(key_skew_, q_cnt);
    fprintf(stderr, "Generated zipf distribution, generating keys...\n");
    for (uint64_t i = 0; i < 100000; i++) {
      randoms_.push_back(z.Next());
    }
    fprintf(stderr, "Generated keys.\n");
  }

  void GenerateLengths() {
    uint64_t q_cnt = layered_succinct_shard_->num_keys();
    int32_t min_len = 100;
    int32_t max_len = 500;
    fprintf(stderr, "Generating zipf distribution with theta=%f, N=%u...\n",
            length_skew_, (max_len - min_len));
    ZipfGenerator z(length_skew_, max_len - min_len);
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
    while (getline(inputfile, line)) {
      // Extract key and value
      int split_index = line.find_first_of('\t');
      bin = line.substr(0, split_index);
      query = line.substr(split_index + 1);
      queries_.push_back(query);
    }
    inputfile.close();
  }

  LayeredSuccinctShard *layered_succinct_shard_;
  std::vector<int32_t> lengths_;
  std::string results_file_;
  double key_skew_;
  double length_skew_;
};

#endif
