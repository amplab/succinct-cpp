#include <iostream>
#include <fstream>
#include <unistd.h>

#include "../include/succinct_server_benchmark.h"

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-t num-threads] bench-type\n", exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 12) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t num_threads = 1;
  uint32_t num_shards = 1;
  uint32_t num_keys = 10000;
  int32_t len = 100;
  std::string queryfile = "";
  while ((c = getopt(argc, argv, "q:t:s:k:l:")) != -1) {
    switch (c) {
      case 't':
        num_threads = atoi(optarg);
        break;
      case 's':
        num_shards = atoi(optarg);
        break;
      case 'k':
        num_keys = atoi(optarg);
        break;
      case 'l':
        len = atoi(optarg);
        break;
      case 'q':
        queryfile = std::string(optarg);
        break;
      default:
        num_threads = 1;
        num_shards = 1;
        num_keys = 10000;
        len = 100;
        queryfile = "";
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string benchmark_type = std::string(argv[optind]);

  // Benchmark core functions
  SuccinctServerBenchmark s_bench(benchmark_type, num_shards, num_keys,
                                  queryfile);
  if (benchmark_type == "latency-get") {
    s_bench.BenchmarkGetLatency("latency_results_get");
  } else if (benchmark_type == "latency-access") {
    s_bench.BenchmarkAccessLatency("latency_results_access", len);
  } else if (benchmark_type == "latency-count") {
    s_bench.BenchmarkCountLatency("latency_results_count");
  } else if (benchmark_type == "latency-search") {
    s_bench.BenchmarkSearchLatency("latency_results_search");
  } else if (benchmark_type == "latency-regex-search") {
    s_bench.BenchmarkRegexSearchLatency("latency_results_regex_search");
  } else if (benchmark_type == "latency-regex-count") {
    s_bench.BenchmarkRegexCountLatency("latency_results_regex_count");
  } else if (benchmark_type == "throughput-get") {
    s_bench.BenchmarkGetThroughput(num_threads);
  } else if (benchmark_type == "throughput-access") {
    s_bench.BenchmarkAccessThroughput(num_threads, len);
  } else {
    // Not supported
    assert(0);
  }

  return 0;
}
