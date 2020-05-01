#include <iostream>
#include <unistd.h>

#include "succinct_shard.h"
#include "succinct_file.h"
#include "npa/npa.h"

#include <boost/python.hpp>
using namespace boost::python;

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

struct File{
  File(std::string inputpath, uint32_t sa_sampling_rate, uint32_t isa_sampling_rate,
    uint32_t npa_sampling_rate, int sampling_opt, int npa_opt){
      this->inputpath = inputpath;
      this->sa_sampling_rate = sa_sampling_rate;
      this->isa_sampling_rate = isa_sampling_rate;
      this->npa_sampling_rate = npa_sampling_rate;
      this->sampling_scheme = SamplingSchemeFromOption(sampling_opt);
      this->npa_encoding_scheme = EncodingSchemeFromOption(npa_opt);
    }

    //File members
    std::string inputpath;
    uint32_t sa_sampling_rate;
    uint32_t isa_sampling_rate;
    uint32_t npa_sampling_rate;
    SamplingScheme sampling_scheme;
    NPA::NPAEncodingScheme npa_encoding_scheme;

    void compressFile(){
      // The following compresses an input file at "inputpath" in memory
      // as a flat file (no structure) using the compression parameters
      // passed in (sampling rates, etc.).
      // Leave the arguments unspecified to use default values.
      auto *fd = new SuccinctFile(inputpath,
                                  SuccinctMode::CONSTRUCT_IN_MEMORY,
                                  sa_sampling_rate, isa_sampling_rate,
                                  npa_sampling_rate, sampling_scheme,
                                  sampling_scheme, npa_encoding_scheme);

      // Serialize the compressed representation to disk at the location <inputpath>.succinct
      fd->Serialize(inputpath + ".succinct");
      delete fd;
    }

    void compressShard(){
      // The following compresses an input file at "inputpath" in memory
      // as a buffer containing key-value pairs. It uses newline '\n' to
      // differentiate between successive values, and assigns the line number
      // as the key for the corresponding value.
      auto *fd = new SuccinctShard(0, inputpath,
                                  SuccinctMode::CONSTRUCT_IN_MEMORY,
                                  sa_sampling_rate, isa_sampling_rate,
                                  npa_sampling_rate, sampling_scheme,
                                  sampling_scheme, npa_encoding_scheme);

      // Serialize the compressed representation to disk at the location <inputpath>.succinct
      fd->Serialize(inputpath + ".succinct");
      delete fd;
    }

};


BOOST_PYTHON_MODULE(pycompress){
  class_<File>("File", init<std::string, uint32_t, uint32_t, uint32_t, int, int>())
    .def("compressFile", &File::compressFile)
    .def("compressShard", &File::compressShard)
    ;
}