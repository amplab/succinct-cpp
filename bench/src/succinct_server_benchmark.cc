#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct_server_benchmark.h"
#include "shard_config.h"

void ParseConfig(std::vector<int32_t>& primary_ids, std::string& conf_file) {
  std::ifstream conf_stream(conf_file);

  std::string line;
  while (std::getline(conf_stream, line)) {
    int32_t id, sampling_rate;
    ShardType shard_type;
    std::istringstream iss(line);

    // Get ID and sampling rate
    iss >> id >> sampling_rate;

    // Get get whether it is primary or not
    int type;
    iss >> type;
    shard_type = (ShardType) type;

    if (shard_type == ShardType::kPrimary
        || shard_type == ShardType::kReplica) {
      fprintf(stderr, "Primary: %d\n", id);
      primary_ids.push_back(id);
    }
  }

  fprintf(stderr, "Read %zu primaries\n", primary_ids.size());
}

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-c config-file] [-q query-file] [-t num-threads] [-k num-keys] [-z skew] bench-type\n",
      exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 12) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t num_threads = 1;
  uint32_t num_keys = 100000;
  double skew = 0.01;
  std::string query_file = "";
  std::string config_file = "conf/blowfish.conf";
  while ((c = getopt(argc, argv, "q:t:k:z:c:")) != -1) {
    switch (c) {
      case 't':
        num_threads = atoi(optarg);
        break;
      case 'k':
        num_keys = atoi(optarg);
        break;
      case 'z':
        skew = atof(optarg);
        break;
      case 'q':
        query_file = std::string(optarg);
        break;
      case 'c':
        config_file = std::string(optarg);
        break;
      default:
        fprintf(stderr, "Invalid parameter %c\n", c);
        return -1;
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string benchmark_type = std::string(argv[optind]);
  std::vector<int32_t> primary_ids;
  ParseConfig(primary_ids, config_file);

  // Benchmark core functions
  SuccinctServerBenchmark s_bench(benchmark_type, skew, num_keys, query_file,
                                  primary_ids);
  if (benchmark_type == "throughput-get") {
    s_bench.BenchmarkGetThroughput(num_threads);
  } else if (benchmark_type == "throughput-search") {
    s_bench.BenchmarkSearchThroughput(num_threads);
  } else if (benchmark_type == "throughput-search-get") {
    s_bench.BenchmarkSearchGetThroughput(num_threads);
  } else {
    // Not supported
    assert(0);
  }

  return 0;
}
