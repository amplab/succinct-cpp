#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinctkv_benchmark.h"

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-q query-file] [-l fetch-length] bench-type\n",
          exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 6) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  int32_t len = 100;
  int32_t num_threads = 1;
  std::string queryfile = "";
  while ((c = getopt(argc, argv, "q:l:t:")) != -1) {
    switch (c) {
      case 't':
        num_threads = atoi(optarg);
        break;
      case 'l':
        len = atoi(optarg);
        break;
      case 'q':
        queryfile = std::string(optarg);
        break;
      default:
        fprintf(stderr, "Invalid option %c", c);
        return -1;
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string benchmark_type = std::string(argv[optind]);

  // Benchmark core functions
  SuccinctKVBenchmark s_bench(queryfile);
  if (benchmark_type == "latency-count") {
    s_bench.BenchmarkCountLatency("latency_results_count");
  } else if (benchmark_type == "latency-search") {
    s_bench.BenchmarkSearchLatency("latency_results_search");
  } else if (benchmark_type == "latency-get") {
    s_bench.BenchmarkGetLatency("latency_results_get");
  } else if (benchmark_type == "latency-access") {
    s_bench.BenchmarkAccessLatency("latency_results_access", len);
  } else if (benchmark_type == "latency-regex") {
    s_bench.BenchmarkRegexLatency("latency_results_regex_search");
  } else if (benchmark_type == "throughput-search") {
    s_bench.BenchmarkSearchThroughput(num_threads);
  } else if (benchmark_type == "throughput-get") {
    s_bench.BenchmarkGetThroughput(num_threads);
  } else if (benchmark_type == "throughput-get-search") {
    s_bench.BenchmarkGetAndSearchThroughput(num_threads);
  } else if (benchmark_type == "throughput-access") {
    s_bench.BenchmarkAccessThroughput(num_threads, len);
  } else {
    // Not supported
    assert(0);
  }

  return 0;
}
