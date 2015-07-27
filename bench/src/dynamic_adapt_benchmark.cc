#include <iostream>
#include <fstream>
#include <unistd.h>

#include "dynamic_adapt_benchmark.h"

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-c test-config-file] [-o output-path] [-p num_partitions]\n",
      exec);
}

int main(int argc, char **argv) {
  if (argc > 7) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  std::string configfile = "benchmark/conf/adashard-bench.conf";
  std::string outpath = "benchmark/res";
  double skew = 1.0;  // Pure uniform
  uint32_t num_partitions = 1;
  while ((c = getopt(argc, argv, "c:o:z:p:")) != -1) {
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
      case 'p':
        num_partitions = atoi(optarg);
        break;
      default:
        configfile = "benchmark/conf/adashard-bench.conf";
        outpath = "benchmark/res";
        skew = 1.0;
        num_partitions = 1;
    }
  }

  // Benchmark core functions
  std::string reqpath = outpath + "/adashard-bench.req";
  std::string respath = outpath + "/adashard-bench.res";
  std::string addpath = outpath + "/adashard-bench.add";
  std::string delpath = outpath + "/adashard-bench.del";
  DynamicAdaptBenchmark q_bench(configfile, reqpath, respath, addpath, delpath,
                                skew, num_partitions);
  q_bench.RunBenchmark();

  return 0;
}
