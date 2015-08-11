#include "file_benchmark.h"

#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct_file.h"

void print_usage(char *exec) {
  fprintf(stderr,
          "Usage: %s [-m mode] [-q query_file] [-t bench_type] [file]\n", exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 8) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t mode = 0;
  std::string querypath = "";
  std::string benchtype = "all";
  while ((c = getopt(argc, argv, "m:q:t:")) != -1) {
    switch (c) {
      case 'm':
        mode = atoi(optarg);
        break;
      case 'q':
        querypath = std::string(optarg);
        break;
      case 't':
        benchtype = std::string(optarg);
        break;
      default:
        fprintf(stderr, "Error parsing arguments.\n");
        return -1;
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string inputpath = std::string(argv[optind]);

  FileBenchmark *file_bench;
  if (mode == 0) {
    SuccinctFile *fd = new SuccinctFile(inputpath);

    // Serialize and save to file
    std::ofstream s_out(inputpath + ".succinct");
    fd->Serialize();
    s_out.close();

    // Create benchmark
    file_bench = new FileBenchmark(fd, querypath);
  } else if (mode == 1) {
    SuccinctFile *fd = new SuccinctFile(inputpath, SuccinctMode::LOAD_IN_MEMORY);

    // Create benchmark
    file_bench = new FileBenchmark(fd, querypath);
  } else {
    // Only modes 0, 1 supported for now
    fprintf(stderr, "Unsupported mode %u.\n", mode);
    return -1;
  }

  if (benchtype == "all") {
    file_bench->BenchmarkCore();
    file_bench->BenchmarkFile();
  } else if (benchtype == "core") {
    file_bench->BenchmarkCore();
  } else if (benchtype == "file") {
    file_bench->BenchmarkFile();
  } else if (benchtype == "search") {
    file_bench->BenchmarkSearch(inputpath + ".search");
  } else if (benchtype == "count") {
    file_bench->BenchmarkCount(inputpath + ".count");
  } else if (benchtype == "extract") {
    file_bench->BenchmarkExtract(inputpath + ".extract");
  } else if (benchtype == "extract2") {
    file_bench->BenchmarkExtract2(inputpath + ".extract2");
  } else if (benchtype == "extract3") {
    file_bench->BenchmarkExtract2(inputpath + ".extract3");
  } else {
    fprintf(stderr, "Unsupported benchmark type %s.\n", benchtype.c_str());
    return -1;
  }

  return 0;
}
