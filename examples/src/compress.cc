#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct_shard.h"
#include "succinct_file.h"
#include "npa/npa.h"

/**
 * Example program that takes an input file and compresses it using Succinct.
 */

/**
 * Prints usage
 */
void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-s sa_sampling_rate] [-i isa_sampling_rate] [-x sampling_scheme] [-n npa_sampling_rate] [-r npa_encoding_scheme] [-t input_type] [file]\n",
      exec);
}

/**
 * Converts integer option to SamplingScheme
 */
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

/**
 * Converts integer option to NPAEncodingScheme
 */
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
  // Logic to parse command line arguments

  // starts here ==>
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
  std::string type = "file";

  while ((c = getopt(argc, argv, "s:i:x:n:r:t:")) != -1) {
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
      case 't': {
        type = optarg;
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
  // <== ends here

  if (type == "file") {
    // The following compresses an input file at "inputpath" in memory
    // as a flat file (no structure) using the compression parameters
    // passed in (sampling rates, etc.).
    // Leave the arguments unspecified to use default values.
    SuccinctFile *fd = new SuccinctFile(inputpath,
                                        SuccinctMode::CONSTRUCT_IN_MEMORY,
                                        sa_sampling_rate, isa_sampling_rate,
                                        npa_sampling_rate, sampling_scheme,
                                        sampling_scheme, npa_encoding_scheme);

    // Serialize the compressed representation to disk at the location <inputpath>.succinct
    fd->Serialize(inputpath + ".succinct");
    delete fd;
  } else if (type == "kv") {
    // The following compresses an input file at "inputpath" in memory
    // as a buffer containing key-value pairs. It uses newline '\n' to
    // differentiate between successive values, and assigns the line number
    // as the key for the corresponding value.
    SuccinctShard *fd = new SuccinctShard(0, inputpath,
                                          SuccinctMode::CONSTRUCT_IN_MEMORY,
                                          sa_sampling_rate, isa_sampling_rate,
                                          npa_sampling_rate, sampling_scheme,
                                          sampling_scheme, npa_encoding_scheme);

    // Serialize the compressed representation to disk at the location <inputpath>.succinct
    fd->Serialize(inputpath + ".succinct");
    delete fd;
  } else {
    fprintf(stderr, "Invalid type: %s\n", type.c_str());
  }

  return 0;
}
