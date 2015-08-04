#include <iostream>
#include <fstream>
#include <unistd.h>

#include "file_benchmark.h"

#include "../../core/include/succinct_file.h"

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-m mode] [file]\n", exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 6) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t mode = 0;
  std::string querypath = "";
  while ((c = getopt(argc, argv, "m:q:")) != -1) {
    switch (c) {
      case 'm':
        mode = atoi(optarg);
        break;
      case 'q':
        querypath = std::string(optarg);
        break;
      default:
        mode = 0;
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string inputpath = std::string(argv[optind]);

  if (mode == 0) {
    SuccinctFile fd(inputpath);

    // Serialize and save to file
    std::ofstream s_out(inputpath + ".succinct");
    fd.Serialize();
    s_out.close();

    // Benchmark core and file functions
    FileBenchmark s_bench(&fd, querypath);
    s_bench.BenchmarkCore();
    s_bench.BenchmarkFile();
  } else if (mode == 1) {
    SuccinctFile fd(inputpath, SuccinctMode::LOAD_IN_MEMORY);

    // Benchmark core and file functions
    FileBenchmark s_bench(&fd, querypath);
    s_bench.BenchmarkCore();
    s_bench.BenchmarkFile();
  } else {
    // Only modes 0, 1 supported for now
    assert(0);
  }

  return 0;
}
