#ifndef ELIAS_GAMMA_ENCODED_NPA_H
#define ELIAS_GAMMA_ENCODED_NPA_H

#include "delta_encoded_npa.h"

#define USE_BSR             0
#define USE_PREFIXSUM_TABLE 1

/* Gamma prefix sum table functions */
#define PREFIX_OFF16(i)   ((prefixsum16_[(i)] >> 24) & 0xFF)
#define PREFIX_CNT16(i)   ((prefixsum16_[(i)] >> 16) & 0xFF)
#define PREFIX_SUM16(i)   (prefixsum16_[(i)] & 0xFFFF)

#define PREFIX_OFF8(i)   ((prefixsum16_[(i)] >> 12) & 0xF)
#define PREFIX_CNT8(i)   ((prefixsum16_[(i)] >> 8) & 0xF)
#define PREFIX_SUM8(i)   (prefixsum16_[(i)] & 0xFF)

class EliasGammaEncodedNPA : public DeltaEncodedNPA {
 public:
  EliasGammaEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
                       uint32_t context_len, uint32_t sampling_rate,
                       Bitmap *data_bitmap, Bitmap *compactSA,
                       Bitmap *compactISA, SuccinctAllocator &s_allocator);

  EliasGammaEncodedNPA(uint32_t context_len, uint32_t sampling_rate,
                       SuccinctAllocator &s_allocator);

  // Virtual destructor
  ~EliasGammaEncodedNPA() {
  }

  // Compute the elias gamma encoding size in bits for a 64 bit integer
  static uint32_t EliasGammaEncodingSize(uint64_t n);

  virtual int64_t BinarySearch(uint64_t val, uint64_t s, uint64_t e, bool flag);

 protected:
  // Create elias-gamma delta encoded vector
  virtual void CreateDeltaEncodedVector(DeltaEncodedVector *dv,
                                        std::vector<uint64_t> &data);

  // Lookup elias-gamma delta encoded vector at index i
  virtual uint64_t LookupDeltaEncodedVector(DeltaEncodedVector *dv, uint64_t i);

 private:
  // Accesses data from a 16 bit integer represented as a bit map
  // from a specified position and for a specified number of bits
  uint16_t AccessDataPos16(uint16_t data, uint32_t pos, uint32_t b);

  // Accesses data from a 8 bit integer represented as a bit map
  // from a specified position and for a specified number of bits
  uint16_t AccessDataPos8(uint8_t data, uint32_t pos, uint32_t b);

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

  // Test
  uint64_t EliasGammaPrefixSum2(Bitmap *B, uint64_t offset, uint64_t i);

  // Pre-computed 16-bit prefix sums for elias gamma encoded NPA
  uint32_t prefixsum16_[65536];

  // Pre-computed 8-bit prefix sums for elias gamma encoded NPA
  uint16_t prefixsum8_[2048];

};

#endif
