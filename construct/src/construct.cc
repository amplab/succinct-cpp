#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct_shard.h"
#include "npa/npa.h"

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-s sa_sampling_rate] [-i isa_sampling_rate] [-x sampling_scheme] [-n npa_sampling_rate] [-r npa_encoding_scheme] [file]\n",
      exec);
}

SamplingScheme SamplingSchemeFromOption(int opt) {
  switch (opt) {
    case 0: {
      fprintf(stderr, "Sampling Scheme = Flat Sample by Index\n");
      return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
    }
    case 1: {
      fprintf(stderr, "Sampling Scheme = Flat Sample by Value\n");
      return SamplingScheme::FLAT_SAMPLE_BY_VALUE;
    }
    case 2: {
      fprintf(stderr, "Sampling Scheme = Layered Sample by Index\n");
      return SamplingScheme::LAYERED_SAMPLE_BY_INDEX;
    }
    case 3: {
      fprintf(stderr,
              "Sampling Scheme = Opportunistic Layered Sample by Index\n");
      return SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX;
    }
    default: {
      fprintf(stderr, "Sampling Scheme = Flat Sample by Index\n");
      return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
    }
  }
}

NPA::NPAEncodingScheme EncodingSchemeFromOption(int opt) {
  switch (opt) {
    case 0: {
      fprintf(stderr, "NPA Encoding Scheme = Elias Delta\n");
      return NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED;
    }
    case 1: {
      fprintf(stderr, "NPA Encoding Scheme = Elias Gamma\n");
      return NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
    }
    case 2: {
      fprintf(stderr, "NPA Encoding Scheme = Wavelet Tree\n");
      return NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED;
    }
    default: {
      fprintf(stderr, "NPA Encoding Scheme = Elias Gamma\n");
      return NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 12) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  uint32_t sa_sampling_rate = 32;
  uint32_t isa_sampling_rate = 32;
  uint32_t npa_sampling_rate = 128;
  SamplingScheme sampling_scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX;
  NPA::NPAEncodingScheme npa_encoding_scheme =
      NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;

  while ((c = getopt(argc, argv, "s:i:x:n:r:")) != -1) {
    switch (c) {
      case 's': {
        sa_sampling_rate = atoi(optarg);
        break;
      }
      case 'i': {
        isa_sampling_rate = atoi(optarg);
        break;
      }
      case 'x': {
        sampling_scheme = SamplingSchemeFromOption(atoi(optarg));
        break;
      }
      case 'n': {
        npa_sampling_rate = atoi(optarg);
        break;
      }
      case 'r': {
        npa_encoding_scheme = EncodingSchemeFromOption(atoi(optarg));
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
  std::string inputpath = std::string(argv[optind]);

  SuccinctShard *fd = new SuccinctShard(0, inputpath,
                                        SuccinctMode::CONSTRUCT_IN_MEMORY,
                                        sa_sampling_rate, isa_sampling_rate,
                                        npa_sampling_rate, sampling_scheme,
                                        sampling_scheme, npa_encoding_scheme);
  // Serialize
  fd->Serialize();

  fprintf(stderr, "Shard Size = %lu\n", fd->StorageSize());

  return 0;
}
