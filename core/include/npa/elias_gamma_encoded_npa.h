#ifndef ELIAS_GAMMA_ENCODED_NPA_H
#define ELIAS_GAMMA_ENCODED_NPA_H

#include "delta_encoded_npa.h"

/* Gamma prefix sum table functions */
#define PREFIX_OFF(i)   ((prefixsum_[(i)] >> 24) & 0xFF)
#define PREFIX_CNT(i)   ((prefixsum_[i] >> 16) & 0xFF)
#define PREFIX_SUM(i)   (prefixsum_[i] & 0xFFFF)

class EliasGammaEncodedNPA : public DeltaEncodedNPA {
 public:
  EliasGammaEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
                       uint32_t context_len, uint32_t sampling_rate,
                       std::string& isa_file,
                       std::vector<uint64_t>& col_offsets,
                       std::string npa_file, SuccinctAllocator &s_allocator);
  
  EliasGammaEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
                       uint32_t context_len, uint32_t sampling_rate,
                       int64_t* lISA,
                       std::vector<uint64_t>& col_offsets,
                       SuccinctAllocator &s_allocator);

  EliasGammaEncodedNPA(uint32_t context_len, uint32_t sampling_rate,
                       SuccinctAllocator &s_allocator);

  // Virtual destructor
  ~EliasGammaEncodedNPA() {
  }

  // Compute the elias gamma encoding size in bits for a 64 bit integer
  static uint32_t EliasGammaEncodingSize(uint64_t n);

  virtual int64_t BinarySearch(int64_t val, uint64_t s, uint64_t e, bool flag);

 protected:
  // Create elias-gamma delta encoded vector
  virtual void CreateDeltaEncodedVector(DeltaEncodedVector *dv,
                                        std::vector<uint64_t> &data);

  // Lookup elias-gamma delta encoded vector at index i
  virtual uint64_t LookupDeltaEncodedVector(DeltaEncodedVector *dv, uint64_t i);

 private:
  // Accesses data from a 64 bit integer represented as a bit map
  // from a specified position and for a specified number of bits
  uint16_t AccessDataPos16(uint16_t data, uint32_t pos, uint32_t b);

  int64_t BinarySearchSamples(DeltaEncodedVector *dv, uint64_t val, uint64_t s,
                              uint64_t e);

  // Initialize pre-computed prefix sums
  void InitPrefixSum();

  // Encode a sorted vector using elias-gamma encoding
  void EliasGammaEncode(Bitmap **B, std::vector<uint64_t> &deltas,
                        uint64_t size);

  // Decode a particular elias-gamma encoded delta value at a provided offset
  // in the deltas bitmap
  uint64_t EliasGammaDecode(Bitmap *B, uint64_t *offset);

  // Compute the prefix sum of elias-gamma encoded delta values
  uint64_t EliasGammaPrefixSum(Bitmap *B, uint64_t offset, uint64_t i);

  // Pre-computed prefix sums for elias gamma encoded NPA
  uint32_t prefixsum_[65536];

};

#endif
