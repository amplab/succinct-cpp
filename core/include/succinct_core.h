#ifndef SUCCINCT_CORE_H
#define SUCCINCT_CORE_H

#include <vector>
#include <fstream>

#include "npa/elias_delta_encoded_npa.h"
#include "npa/elias_gamma_encoded_npa.h"
#include "npa/npa.h"
#include "npa/wavelet_tree_encoded_npa.h"
#include "sampledarray/flat_sampled_array.h"
#include "sampledarray/layered_sampled_array.h"
#include "sampledarray/layered_sampled_isa.h"
#include "sampledarray/layered_sampled_sa.h"
#include "sampledarray/opportunistic_layered_sampled_array.h"
#include "sampledarray/opportunistic_layered_sampled_isa.h"
#include "sampledarray/opportunistic_layered_sampled_sa.h"
#include "sampledarray/sampled_by_index_isa.h"
#include "sampledarray/sampled_by_index_sa.h"
#include "sampledarray/sampled_by_value_isa.h"
#include "sampledarray/sampled_by_value_sa.h"
#include "succinct_base.h"
#include "utils/array_stream.h"
#include "utils/divsufsortxx.h"
#include "utils/divsufsortxx_utility.h"

typedef enum {
  CONSTRUCT_IN_MEMORY = 0,
  CONSTRUCT_MEMORY_MAPPED = 1,
  LOAD_IN_MEMORY = 2,
  LOAD_MEMORY_MAPPED = 3
} SuccinctMode;

class SuccinctCore : public SuccinctBase {
 public:
  typedef std::map<char, std::pair<uint64_t, uint32_t>> AlphabetMap;
  typedef std::pair<int64_t, int64_t> Range;

  /* Constructors */
  SuccinctCore(const std::string& filename, SuccinctMode s_mode =
                   SuccinctMode::CONSTRUCT_IN_MEMORY,
               uint32_t sa_sampling_rate = 32, uint32_t isa_sampling_rate = 32,
               uint32_t npa_sampling_rate = 128, uint32_t context_len = 3,
               SamplingScheme sa_sampling_scheme =
                   SamplingScheme::FLAT_SAMPLE_BY_INDEX,
               SamplingScheme isa_sampling_scheme =
                   SamplingScheme::FLAT_SAMPLE_BY_INDEX,
               NPA::NPAEncodingScheme npa_encoding_scheme =
                   NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED,
               uint32_t sampling_range = 1024);

  SuccinctCore() {
    this->alphabet_ = NULL;
    this->sa_ = NULL;
    this->isa_ = NULL;
    this->npa_ = NULL;
    this->alphabet_size_ = 0;
    this->input_size_ = 0;
  }

  virtual ~SuccinctCore() {
  }

  /* Lookup functions for each of the core data structures */
  // Lookup NPA at index i
  uint64_t LookupNPA(uint64_t i);

  // Lookup SA at index i
  uint64_t LookupSA(uint64_t i);

  // Lookup ISA at index i
  uint64_t LookupISA(uint64_t i);

  // Get index of value v in C
  uint64_t LookupC(uint64_t val);

  // Get the character at index i
  char CharAt(uint64_t i);

  // Serialize succinct data structures
  virtual size_t Serialize(const std::string& filename);

  // Deserialize succinct data structures
  virtual size_t Deserialize(const std::string& filename);

  // Memory map succinct data structures
  virtual size_t MemoryMap(const std::string& filename);

  // Get size of original input
  uint64_t GetOriginalSize();

  // Get succinct core size
  virtual size_t StorageSize();

  virtual void PrintStorageBreakdown();

  // Get SA
  SampledArray *GetSA();

  // Get ISA
  SampledArray *GetISA();

  // Get NPA
  NPA *GetNPA();

  // Get alphabet
  char *GetAlphabet();

  inline int Compare(std::string mgram, int64_t pos);
  inline int Compare(std::string mgram, int64_t pos, size_t offset);

  Range BwdSearch(std::string mgram);
  Range ContinueBwdSearch(std::string mgram, Range range);

  Range FwdSearch(std::string mgram);
  Range ContinueFwdSearch(std::string mgram, Range range, size_t len);

 protected:

  // Allocates high level containers
  void Allocate(uint32_t sa_sampling_rate, uint32_t isa_sampling_rate,
                uint32_t npa_sampling_rate, uint32_t context_len,
                SamplingScheme sa_sampling_scheme,
                SamplingScheme isa_sampling_scheme,
                NPA::NPAEncodingScheme npa_encoding_scheme,
                uint32_t sampling_range);

  // Constructs the core data structures
  void Construct(const std::string& filename, uint32_t sa_sampling_rate,
                 uint32_t isa_sampling_rate, uint32_t npa_sampling_rate,
                 uint32_t context_len, SamplingScheme sa_sampling_scheme,
                 SamplingScheme isa_sampling_scheme,
                 NPA::NPAEncodingScheme npa_encoding_scheme,
                 uint32_t sampling_range);

  // Constructs the core data structures
  void Construct(uint8_t* input, size_t input_size, uint32_t sa_sampling_rate,
                 uint32_t isa_sampling_rate, uint32_t npa_sampling_rate,
                 uint32_t context_len, SamplingScheme sa_sampling_scheme,
                 SamplingScheme isa_sampling_scheme,
                 NPA::NPAEncodingScheme npa_encoding_scheme,
                 uint32_t sampling_range);

  /* Metadata */
  uint64_t input_size_;                // Size of input

  /* Primary data structures */
  SampledArray *sa_;                   // Suffix Array
  SampledArray *isa_;                  // Inverse Suffix Array
  NPA *npa_;                           // Next Pointer Array

  /* Auxiliary data structures */
  char *alphabet_;
  AlphabetMap alphabet_map_;
  uint32_t alphabet_size_;             // Size of the input alphabet_

 private:

  uint64_t ComputeContextValue(char* data, uint32_t i, uint32_t context_len) {
    uint64_t val = 0;
    uint64_t max = SuccinctUtils::Min(i + context_len, input_size_);
    for (uint64_t t = i; t < max; t++) {
      val = val * 256 + data[t];
    }

    if (max < i + context_len) {
      for (uint64_t t = 0; t < (i + context_len) % input_size_; t++) {
        val = val * 256 + data[t];
      }
    }

    return val;
  }

  bool CompareContexts(char *data, uint64_t pos1, uint64_t pos2,
                       uint32_t context_len) {
    for (uint64_t t = pos1; t < pos1 + context_len; t++) {
      if (data[t] != data[pos2++]) {
        return false;
      }
    }
    return true;
  }

//  bool IsSampled(SamplingScheme scheme, uint32_t sampling_rate, uint64_t i,
//                 uint64_t val) {
//    switch (scheme) {
//      case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
//      case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
//      case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX: {
//        if (i % sampling_rate == 0) {
//          return true;
//        }
//        return false;
//      }
//      case SamplingScheme::FLAT_SAMPLE_BY_VALUE: {
//        if (val % sampling_rate == 0) {
//          return true;
//        }
//        return false;
//      }
//    }
//    return false;
//  }

  static Bitmap* ReadAsBitmap(size_t size, uint8_t bits,
                              SuccinctAllocator& s_allocator,
                              std::string& infile) {
    Bitmap* B = new Bitmap;
    InitBitmap(&B, size * bits, s_allocator);
    std::ifstream in(infile);
    for (uint64_t i = 0; i < size; i++) {
      uint64_t val;
      in.read(reinterpret_cast<char *>(&val), size * sizeof(uint64_t));
      SetBitmapArray(&B, i, val, bits);
    }
    return B;
  }
};
#endif
