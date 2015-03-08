#include "succinct/sampledarray/OpportunisticLayeredSampledSA.hpp"

OpportunisticLayeredSampledSA::OpportunisticLayeredSampledSA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
        NPA *npa, bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator) :
        OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate, SA, sa_n, s_allocator)
{
    this->npa = npa;
    layered_sample(SA, sa_n);
}

OpportunisticLayeredSampledSA::OpportunisticLayeredSampledSA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
        NPA *npa, SuccinctAllocator &s_allocator) :
        OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate, s_allocator)
{
    this->npa = npa;
}

void OpportunisticLayeredSampledSA::layered_sample(bitmap_t *SA, uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
        uint64_t sa_val = SuccinctBase::lookup_bitmap_array(SA, i, data_bits);
        if(i % target_sampling_rate == 0) {
            layer_t l;
            get_layer(&l, i);
            bitmap_t *data = layer_data[l.layer_id];
            SuccinctBase::set_bitmap_array(&data, l.layer_idx, sa_val,
                                data_bits);
        }
    }
}

uint64_t OpportunisticLayeredSampledSA::operator[](uint64_t i) {
    assert(i < original_size);

    uint64_t j = 0;
    while (!is_sampled(i)) {
        i = (*npa)[i];
        j++;
    }

    uint64_t sa_val = sampled_at(i / target_sampling_rate);
    if(sa_val < j)
        return original_size - (j - sa_val);
    else
        return sa_val - j;

    return 0;
}
