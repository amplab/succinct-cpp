#ifndef SAMPLED_BY_VALUE_ISA_H
#define SAMPLED_BY_VALUE_ISA_H

#include "flat_sampled_array.h"

class SampledByValueISA : public FlatSampledArray {
 public:
  // Constructor
  SampledByValueISA(uint32_t sampling_rate, NPA *npa, bitmap_t *SA,
                    uint64_t sa_n, Dictionary *d_bpos,
                    SuccinctAllocator &s_allocator);

  SampledByValueISA(uint32_t sampling_rate, NPA *npa,
                    SuccinctAllocator &s_allocator);

  // Access element at index i
  virtual uint64_t operator[](uint64_t i);

  // Is the value sampled at index i
  virtual bool IsSampled(uint64_t i);

  // Set d_bpos
  void SetSampledPositions(Dictionary *sampled_positions);

 protected:
  // Sample by value for ISA using original SA
  virtual void Sample(bitmap_t *original, uint64_t n);

 private:
  Dictionary *sampled_positions_;
};

#endif
