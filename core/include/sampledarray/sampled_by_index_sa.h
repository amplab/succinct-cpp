#ifndef SAMPLED_BY_INDEX_SA_H
#define SAMPLED_BY_INDEX_SA_H

#include "flat_sampled_array.h"

class SampledByIndexSA : public FlatSampledArray {
 public:
  // Constructor
  SampledByIndexSA(uint32_t sampling_rate, NPA *npa, ArrayStream& sa_stream,
                   uint64_t sa_n, SuccinctAllocator &s_allocator);

  SampledByIndexSA(uint32_t sampling_rate, NPA *npa, ArrayInput& sa_array,
                   uint64_t sa_n, SuccinctAllocator &s_allocator);

  SampledByIndexSA(uint32_t sampling_rate, NPA *npa,
                   SuccinctAllocator &s_allocator);

  // Access element at index i
  virtual uint64_t operator[](uint64_t i);

 protected:
  // Sample original SA by index
  virtual void Sample(ArrayStream& sa_stream, uint64_t n);
  virtual void SampleInMem(ArrayInput& sa_array, uint64_t n);
};

#endif
