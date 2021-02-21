#ifndef ELIAS_DELTA_ENCODED_NPA_H
#define ELIAS_DELTA_ENCODED_NPA_H

#include "delta_encoded_npa.h"
#include "elias_gamma_encoded_npa.h"

class EliasDeltaEncodedNPA : public DeltaEncodedNPA {

 protected:
  // Create Elias-Delta encoded vector
  virtual void CreateDeltaEncodedVector(DeltaEncodedVector *dv,
                                        std::vector<uint64_t> &data);

  // Lookup Elias-Delta encoded vector at index i
  virtual uint64_t LookupDeltaEncodedVector(DeltaEncodedVector *dv, uint64_t i);

 public:
  EliasDeltaEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
                       uint32_t context_len, uint32_t sampling_rate,
                       std::string& isa_file,
                       std::vector<uint64_t>& col_offsets, std::string npa_file,
                       SuccinctAllocator &s_allocator);

  EliasDeltaEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
                       uint32_t context_len, uint32_t sampling_rate,
                       int64_t* lISA,
                       std::vector<uint64_t>& col_offsets,
                       SuccinctAllocator &s_allocator);

  EliasDeltaEncodedNPA(uint32_t context_len, uint32_t sampling_rate,
                       SuccinctAllocator &s_allocator);

  // Virtual destructor
  ~EliasDeltaEncodedNPA();

 private:
  uint32_t EliasDeltaEncodingSize(uint64_t n);

  void EliasDeltaEncode(Bitmap **B, std::vector<uint64_t> &deltas,
                        uint64_t size);

  // Decode a particular Elias-Delta encoded delta value at a provided offset
  // in the deltas bitmaps
  uint64_t EliasDeltaDecode(Bitmap *B, uint64_t *offset);

  // Compute the prefix sum for the Elias-Delta encoded deltas
  uint64_t EliasDeltaPrefixSum(Bitmap *B, uint64_t offset, uint64_t i);

};

#endif
