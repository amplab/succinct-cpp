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
  if (argc > 19) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  std::string outpath = "bench/res";
  double skew = 1.0;  // Pure uniform
  uint32_t isa_sampling_rate = 32, sa_sampling_rate = 32, len = 100, mod = 1;
  SuccinctMode s_mode = SuccinctMode::LOAD_IN_MEMORY;
  std::string querypath = "", benchmark = "throughput";
  while ((c = getopt(argc, argv, "m:o:z:i:s:l:q:f:b:")) != -1) {
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
      case 'b':
        benchmark = optarg;
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
  LayeredSuccinctShardBenchmark ls_bench(
      inputpath, s_mode == SuccinctMode::CONSTRUCT_IN_MEMORY, isa_sampling_rate,
      sa_sampling_rate, respath, skew, mod, querypath);

  if (benchmark == "throughput") {
    for (int32_t i = -1; i < 10; i++) {
      ls_bench.DeleteLayer(i);
      ls_bench.MeasureSearchGetThroughput();
    }
  } else if (benchmark == "reconstruct") {
    ls_bench.DeleteLayer(0);  // Now at 16
    ls_bench.DeleteLayer(1);  // Now at 32
    ls_bench.DeleteLayer(2);  // Now at 64
    for (uint32_t num_threads = 4; num_threads <= 16; num_threads++) {
      Benchmark::TimeStamp t0 = Benchmark::GetTimestamp();
      ls_bench.ReconstructLayer(2, num_threads);
      Benchmark::TimeStamp t1 = Benchmark::GetTimestamp();
      fprintf(stderr, "%u\t%llu\n", num_threads, (t1 - t0) / (1000 * 1000));
      ls_bench.DeleteLayer(2);
    }
  }

  return 0;
}
