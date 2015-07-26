#ifndef OPPORTUNISTIC_LAYERED_SAMPLED_SA_HPP
#define OPPORTUNISTIC_LAYERED_SAMPLED_SA_HPP

#include "opportunistic_layered_sampled_array.h"

class OpportunisticLayeredSampledSA : public OpportunisticLayeredSampledArray {
protected:
    NPA *npa;

    void layered_sample(bitmap_t *SA, uint64_t n);

public:
    OpportunisticLayeredSampledSA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
            NPA *npa, bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator);

    OpportunisticLayeredSampledSA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
            NPA *npa, SuccinctAllocator &s_allocator);

    // Access element at index i
    uint64_t operator[](uint64_t i);

};

#endif
