#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <sys/time.h>
#include <sstream>

#include "succinct_shard.h"

#include <boost/python.hpp>
using namespace boost::python;

/**
 * Program that wraps succinct's query kv functions for python use via boost
 */

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

/**
 * PyKv struct that wraps query and compress functions for boost python
 */
struct PyKv{
    // Constructor that loads from kv
    PyKv(const std::string& filename) {
        s_file_ = nullptr;
        // Read the serialized data structures from disk.
        // The serialized data structures must exist at <filename>.succinct.
        std::cout << "De-serializing Succinct data structures...\n";
        s_file_ = new SuccinctShard(0, filename, SuccinctMode::LOAD_IN_MEMORY);
    }

    // Constructor that compresses kv given the sampling rate arguments
    PyKv(const std::string& inputpath, uint32_t sa_sampling_rate, uint32_t isa_sampling_rate,
    uint32_t npa_sampling_rate, int sampling_opt, int npa_opt){
      s_file_ = nullptr;
      // The following compresses an input file at "inputpath" in memory
      // as a buffer containing key-value pairs. It uses newline '\n' to
      // differentiate between successive values, and assigns the line number
      // as the key for the corresponding value.
      std::cout << "Constructing Succinct data structures...\n";
      s_file_ = new SuccinctShard(0, inputpath,
                                  SuccinctMode::CONSTRUCT_IN_MEMORY,
                                  sa_sampling_rate, isa_sampling_rate,
                                  npa_sampling_rate, SamplingSchemeFromOption(sampling_opt),
                                  SamplingSchemeFromOption(sampling_opt), EncodingSchemeFromOption(npa_opt));
      std::cout << "Serializing Succinct data structures...\n";
      // Serialize the compressed representation to disk at the location <inputpath>.succinct
      s_file_->Serialize(inputpath + ".succinct");
    }
    
    // Wrapped search command, that returns a python list
    boost::python::list PySearch(const std::string& arg) {
        std::set<int64_t> results;
        s_file_->Search(results, arg);
        boost::python::list ret;
        for (const auto& i :results){
          ret.append(i);
        }
        return ret;
    }

    //Wrapped search command
    std::set<int64_t> Search(const std::string &arg) {
        std::set<int64_t> results;
        s_file_->Search(results, arg);
        return results;
    }

    //Wrapped count command
    int64_t Count(const std::string &arg) {
        int64_t count = s_file_->Count(arg);
        return count;
    }

    //Wrapped get command
    std::string Get(uint64_t key) {
        std::string result;
        s_file_->Get(result, key);
        return result;
    }

    //PyKv members
    SuccinctShard *s_file_;

};

//Boost Python module
BOOST_PYTHON_MODULE(pykv){
    class_<PyKv>("PyKv", init<std::string>())
    .def(init<std::string, uint32_t, uint32_t, uint32_t, int, int>())
    .def("PySearch", &PyKv::PySearch)
    .def("Search", &PyKv::Search)
    .def("Count", &PyKv::Count)
    .def("Get", &PyKv::Get)
    ;

}
