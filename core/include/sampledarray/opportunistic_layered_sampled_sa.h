#ifndef OPPORTUNISTIC_LAYERED_SAMPLED_SA_HPP
#define OPPORTUNISTIC_LAYERED_SAMPLED_SA_HPP

#include "opportunistic_layered_sampled_array.h"

class OpportunisticLayeredSampledSA : public OpportunisticLayeredSampledArray {
 public:
  OpportunisticLayeredSampledSA(uint32_t target_sampling_rate,
                                uint32_t base_sampling_rate, NPA *npa,
                                ArrayStream& sa_stream, uint64_t sa_n,
                                SuccinctAllocator &s_allocator);

  OpportunisticLayeredSampledSA(uint32_t target_sampling_rate,
                                uint32_t base_sampling_rate, NPA *npa,
                                SuccinctAllocator &s_allocator);

  // Access element at index i
  uint64_t operator[](uint64_t i);

 protected:
  NPA *npa;

  void SampleLayered(ArrayStream& sa_stream, uint64_t n);
};

#endif
