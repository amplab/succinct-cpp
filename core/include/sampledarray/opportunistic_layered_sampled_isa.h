#ifndef OPPORTUNISTIC_LAYERED_SAMPLED_ISA_HPP
#define OPPORTUNISTIC_LAYERED_SAMPLED_ISA_HPP

#include "opportunistic_layered_sampled_array.h"

class OpportunisticLayeredSampledISA : public OpportunisticLayeredSampledArray {
 public:
  OpportunisticLayeredSampledISA(uint32_t target_sampling_rate,
                                 uint32_t base_sampling_rate, NPA *npa,
                                 DataInputStream<uint64_t>& sa_stream, uint64_t sa_n,
                                 SuccinctAllocator &s_allocator);

  OpportunisticLayeredSampledISA(uint32_t target_sampling_rate,
                                 uint32_t base_sampling_rate, NPA *npa,
                                 SuccinctAllocator &s_allocator);

  // Access element at index i
  uint64_t operator[](uint64_t i);

 protected:
  void SampleLayered(DataInputStream<uint64_t>& sa_stream, uint64_t n);

  NPA *npa_;
};

#endif
