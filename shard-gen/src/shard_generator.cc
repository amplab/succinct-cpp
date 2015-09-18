#include <iostream>
#include <fstream>
#include <unistd.h>

#include "layered_succinct_shard.h"

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-s input-sampling-rate] [-m max-sampling-rate] [-o output] [file]\n",
      exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 12) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  std::string output = "data";
  uint32_t max_sr = 128;
  uint32_t input_sr = 8;
  while ((c = getopt(argc, argv, "s:m:o:")) != -1) {
    switch (c) {
      case 's': {
        input_sr = atoi(optarg);
        break;
      }
      case 'm': {
        max_sr = atoi(optarg);
        break;
      }
      case 'o': {
        output = optarg;
        break;
      }
      default: {
        fprintf(stderr, "Error parsing options\n");
        exit(-1);
      }
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }
  std::string input = std::string(argv[optind]);

  LayeredSuccinctShard *fd = new LayeredSuccinctShard(
      0, input, SuccinctMode::LOAD_IN_MEMORY, input_sr, input_sr);

  // Serialize
  for (uint32_t sr = 8; sr <= max_sr; sr *= 2) {
    fprintf(stderr, "Serializing for sampling rate = %u\n", sr);
    std::string path = output + std::string("_") + std::to_string(sr)
        + ".succinct";
    fd->SerializeStatic(path, sr);
  }

  return 0;
}
