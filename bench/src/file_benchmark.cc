#include "file_benchmark.h"

#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct_file.h"

SamplingScheme SamplingSchemeFromOption(int opt) {
  switch (opt) {
    case 0:
      return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
    case 1:
      return SamplingScheme::FLAT_SAMPLE_BY_VALUE;
    case 2:
      return SamplingScheme::LAYERED_SAMPLE_BY_INDEX;
    case 3:
      return SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX;
    default:
      return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
  }
}

NPA::NPAEncodingScheme EncodingSchemeFromOption(int opt) {
  switch (opt) {
    case 0:
      return NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED;
    case 1:
      return NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
    case 2:
      return NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED;
    default:
      return NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
  }
}

void print_usage(char *exec) {
  fprintf(stderr,
          "Usage: %s [-m mode] [-q query_file] [-t bench_type] [file]\n", exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 18) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t mode = 0;
  std::string querypath = "";
  std::string benchtype = "all";
  uint32_t sa_sampling_rate = 32;
  uint32_t isa_sampling_rate = 32;
  uint32_t npa_sampling_rate = 128;
  SamplingScheme scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX;
  NPA::NPAEncodingScheme npa_scheme =
      NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
  while ((c = getopt(argc, argv, "m:q:t:s:i:x:n:r:")) != -1) {
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
      case 's':
        sa_sampling_rate = atoi(optarg);
        break;
      case 'i':
        isa_sampling_rate = atoi(optarg);
        break;
      case 'x':
        scheme = SamplingSchemeFromOption(atoi(optarg));
        break;
      case 'n':
        npa_sampling_rate = atoi(optarg);
        break;
      case 'r':
        npa_scheme = EncodingSchemeFromOption(atoi(optarg));
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
    SuccinctFile *fd = new SuccinctFile(inputpath,
                                        SuccinctMode::CONSTRUCT_IN_MEMORY,
                                        sa_sampling_rate, isa_sampling_rate,
                                        npa_sampling_rate, scheme, scheme,
                                        npa_scheme);

    // Serialize and save to file
    std::ofstream s_out(inputpath + ".succinct");
    fd->Serialize();
    s_out.close();

    // Create benchmark
    file_bench = new FileBenchmark(fd, querypath);
  } else if (mode == 1) {
    SuccinctFile *fd = new SuccinctFile(inputpath, SuccinctMode::LOAD_IN_MEMORY,
                                        sa_sampling_rate, isa_sampling_rate,
                                        npa_sampling_rate, scheme, scheme,
                                        npa_scheme);

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
  } else {
    fprintf(stderr, "Unsupported benchmark type %s.\n", benchtype.c_str());
    return -1;
  }

  return 0;
}
