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
  SuccinctCore(const char *filename, SuccinctMode s_mode =
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

  virtual ~SuccinctCore() {
  }

  /* Lookup functions for each of the core data structures */
  // Lookup NPA at index i
  uint64_t lookupNPA(uint64_t i);

  // Lookup SA at index i
  uint64_t lookupSA(uint64_t i);

  // Lookup ISA at index i
  uint64_t lookupISA(uint64_t i);

  // Get index of value v in C
  uint64_t lookupC(uint64_t val);

  char charAt(uint64_t i);

  // Serialize succinct data structures
  virtual size_t serialize();

  // Deserialize succinct data structures
  virtual size_t deserialize();

  // Memory map succinct data structures
  virtual size_t memorymap();

  // Get size of original input
  uint64_t original_size();

  // Get succinct core size
  virtual size_t storage_size();

  virtual void print_storage_breakdown();

  // Get SA
  SampledArray *getSA();

  // Get ISA
  SampledArray *getISA();

  // Get NPA
  NPA *getNPA();

  // Get alphabet
  char *getAlphabet();

  inline int compare(std::string mgram, int64_t pos);
  inline int compare(std::string mgram, int64_t pos, size_t offset);

  Range bw_search(std::string mgram);
  Range continue_bw_search(std::string mgram, Range range);

  Range fw_search(std::string mgram);
  Range continue_fw_search(std::string mgram, Range range, size_t len);

 protected:

  /* Metadata */
  std::string filename;               // Name of input file
  std::string succinct_path;          // Name of succinct path
  uint64_t input_size;                // Size of input

  /* Primary data structures */
  SampledArray *SA;                   // Suffix Array
  SampledArray *ISA;                  // Inverse Suffix Array
  NPA *npa;                           // Next Pointer Array
  std::vector<uint64_t> Cinv_idx;     // Indexes into Cinv;

  /* Auxiliary data structures */
  char *alphabet;
  AlphabetMap alphabet_map;
  uint32_t alphabet_size;             // Size of the input alphabet

 private:
  /* Construct functions */
  // Create all auxiliary data structures
  void construct_aux(BitMap *compactSA, const char *data);

  // Parent construct function
  void construct(const char* filename, uint32_t sa_sampling_rate,
                 uint32_t isa_sampling_rate, uint32_t npa_sampling_rate,
                 uint32_t context_len, SamplingScheme sa_sampling_scheme,
                 SamplingScheme isa_sampling_scheme,
                 NPA::NPAEncodingScheme npa_encoding_scheme,
                 uint32_t sampling_range);

  // Helper functions
  bool compare_data_bitmap(BitMap *T, uint64_t i, uint64_t j, uint64_t k);
  uint64_t get_context_val(BitMap *T, uint32_t i);

  bool is_sampled(uint64_t i);
};
#endif
