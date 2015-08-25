#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct_benchmark.h"

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
  std::string queryfile = "";
  while ((c = getopt(argc, argv, "q:l:")) != -1) {
    switch (c) {
      case 'l':
        len = atoi(optarg);
        break;
      case 'q':
        queryfile = std::string(optarg);
        break;
      default:
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
  SuccinctBenchmark s_bench(benchmark_type, queryfile);
  if (benchmark_type == "latency-count") {
    s_bench.BenchmarkCountLatency("latency_results_count");
  } else if (benchmark_type == "latency-search") {
    s_bench.BenchmarkSearchLatency("latency_results_search");
  } else if (benchmark_type == "latency-extract") {
    s_bench.BenchmarkExtractLatency("latency_results_extract", len);
  } else if (benchmark_type == "latency-regex") {
    s_bench.BenchmarkRegexLatency("latency_results_regex_search");
  } else {
    // Not supported
    assert(0);
  }

  return 0;
}
