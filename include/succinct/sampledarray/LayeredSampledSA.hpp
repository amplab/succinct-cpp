#ifndef LAYERED_SAMPLED_SA_HPP
#define LAYERED_SAMPLED_SA_HPP

#include "succinct/sampledarray/LayeredSampledArray.hpp"

class LayeredSampledSA : public LayeredSampledArray {
protected:
    NPA *npa;

    void layered_sample(bitmap_t *SA, uint64_t n);

public:
    LayeredSampledSA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
            NPA *npa, bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator);

    LayeredSampledSA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
            NPA *npa, SuccinctAllocator &s_allocator);

    // Access element at index i
    uint64_t operator[](uint64_t i);

    size_t reconstruct_layer(uint32_t layer_id);

    size_t reconstruct_layer_fast(uint32_t layer_id);

};

#endif /* LAYEREDSAMPLEDSA_HPP */
