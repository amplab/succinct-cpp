#ifndef ELIAS_DELTA_ENCODED_NPA_H
#define ELIAS_DELTA_ENCODED_NPA_H

#include "npa/DeltaEncodedNPA.hpp"
#include "npa/EliasGammaEncodedNPA.hpp"

class EliasDeltaEncodedNPA : public DeltaEncodedNPA {

 protected:
  // Create elias-gamma delta encoded vector
  virtual void createDEV(DeltaEncodedVector *dv, std::vector<uint64_t> &data);

  // Lookup elias-gamma delta encoded vector at index i
  virtual uint64_t lookupDEV(DeltaEncodedVector *dv, uint64_t i);

 public:
  EliasDeltaEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
                       uint32_t context_len, uint32_t sampling_rate,
                       bitmap_t *data_bitmap, bitmap_t *compactSA,
                       bitmap_t *compactISA, SuccinctAllocator &s_allocator);

  EliasDeltaEncodedNPA(uint32_t context_len, uint32_t sampling_rate,
                       SuccinctAllocator &s_allocator);

  // Virtual destructor
  ~EliasDeltaEncodedNPA() {
  }

 private:
  uint32_t elias_delta_encoding_size(uint64_t n);

  void elias_delta_encode(bitmap_t **B, std::vector<uint64_t> &deltas,
                          uint64_t size);

  // Decode a particular elias-delta encoded delta value at a provided offset
  // in the deltas bitmaps
  uint64_t elias_delta_decode(bitmap_t *B, uint64_t *offset);

  // Compute the prefix sum for the elias delta encoded deltas
  uint64_t elias_delta_prefix_sum(bitmap_t *B, uint64_t offset, uint64_t i);

};

#endif
