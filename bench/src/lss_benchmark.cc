#include <iostream>
#include <fstream>
#include <unistd.h>
#include "layered_succinct_shard_benchmark.h"


void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-m mode] [-z skew] [-i isa_sampling_rate] [-s sa_sampling_rate] [-l len] [-o output-path] file\n",
      exec);
}

int main(int argc, char **argv) {
  if (argc > 17) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  std::string outpath = "bench/res";
  double skew = 0.01;  // Pure uniform
  uint32_t isa_sampling_rate = 8, sa_sampling_rate = 8, len = 100, mod = 1;
  SuccinctMode s_mode = SuccinctMode::LOAD_IN_MEMORY;
  std::string querypath = "";
  while ((c = getopt(argc, argv, "m:o:z:i:s:l:q:f:")) != -1) {
    switch (c) {
      case 'm':
        s_mode = (SuccinctMode) atoi(optarg);
        break;
      case 'o':
        outpath = std::string(optarg);
        break;
      case 'z':
        skew = atof(optarg);
        break;
      case 'i':
        isa_sampling_rate = atoi(optarg);
        break;
      case 's':
        sa_sampling_rate = atoi(optarg);
        break;
      case 'l':
        len = atoi(optarg);
        break;
      case 'q':
        querypath = optarg;
        break;
      case 'f':
        mod = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Incorrect parameter : %c\n", c);
        exit(-1);
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string inputpath = std::string(argv[optind]);

  // Benchmark core functions
  std::string respath = outpath + "/ls-bench";
  LayeredSuccinctShardBenchmark ls_bench(inputpath, s_mode == SuccinctMode::CONSTRUCT_IN_MEMORY, isa_sampling_rate,
                                         sa_sampling_rate, respath, skew, mod,
                                         querypath);
  for (int32_t i = -1; i < 10; i++) {
    ls_bench.DeleteLayer(i);
    ls_bench.MeasureSearchGetThroughput();
  }

  return 0;
}
