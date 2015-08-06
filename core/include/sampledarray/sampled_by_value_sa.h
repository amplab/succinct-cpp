#ifndef SAMPLED_BY_VALUE_SA_H
#define SAMPLED_BY_VALUE_SA_H

#include "flat_sampled_array.h"

class SampledByValueSA : public FlatSampledArray {
 public:
  // Constructor
  SampledByValueSA(uint32_t sampling_rate, NPA *npa, bitmap_t *SA,
                   uint64_t sa_n, SuccinctAllocator &s_allocator);

  SampledByValueSA(uint32_t sampling_rate, NPA *npa,
                   SuccinctAllocator &s_allocator);

  // Access element at index i
  virtual uint64_t operator[](uint64_t i);

  // Get d_bpos
  Dictionary *GetSampledPositions();

  // Set d_bpos
  void SetSampledPositions(Dictionary *sampled_positions);

 protected:
  // Sample original SA by value
  virtual void Sample(bitmap_t *original, uint64_t n);

  // Check if index is sampled
  bool IsSampled(uint64_t i);

 private:
  Dictionary *sampled_positions_;
};

#endif
