#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct_server_benchmark.h"

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-q query-file] [-t num-threads] [-s num-shards] [-k num-keys] [-l fetch-length] bench-type\n",
      exec);
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
  if (benchmark_type == "throughput-get") {
    s_bench.BenchmarkGetThroughput(num_threads);
  } else if (benchmark_type == "throughput-search") {
    s_bench.BenchmarkSearchThroughput(num_threads);
  } else if (benchmark_type == "throughput-search-get") {
    s_bench.BenchmarkSearchThroughput(num_threads);
  } else {
    // Not supported
    assert(0);
  }

  return 0;
}
