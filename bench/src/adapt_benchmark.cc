#include <iostream>
#include <fstream>
#include <unistd.h>

#include "adapt_benchmark.h"

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-c test-config-file] [-o output-path]\n", exec);
}

int main(int argc, char **argv) {
  if (argc > 12) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  std::string configfile = "bench/conf/adashard-bench.conf";
  std::string outpath = "bench/res";
  double skew = 1.0;  // Pure uniform
  int32_t len = 100, batch_size = 10;
  std::string querypath = "";
  while ((c = getopt(argc, argv, "c:o:z:l:b:q:")) != -1) {
    switch (c) {
      case 'c':
        configfile = std::string(optarg);
        break;
      case 'o':
        outpath = std::string(optarg);
        break;
      case 'z':
        skew = atof(optarg);
        break;
      case 'l':
        len = atoi(optarg);
        break;
      case 'b':
        batch_size = atoi(optarg);
        break;
      case 'q':
        querypath = std::string(optarg);
        break;
      default:
        configfile = "benchmark/conf/adashard-bench.conf";
        outpath = "benchmark/res";
        skew = 1.0;
        querypath = "";
    }
  }

  // Benchmark core functions
  std::string reqpath = outpath + "/adashard-bench.req";
  std::string respath = outpath + "/adashard-bench.res";
  std::string addpath = outpath + "/adashard-bench.add";
  std::string delpath = outpath + "/adashard-bench.del";
  AdaptBenchmark d_bench(configfile, reqpath, respath, addpath, delpath, skew,
                         len, batch_size, querypath);
  d_bench.RunBenchmark();

  return 0;
}
