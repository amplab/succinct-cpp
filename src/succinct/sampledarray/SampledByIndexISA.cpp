#include "succinct/sampledarray/SampledByIndexISA.hpp"

SampledByIndexISA::SampledByIndexISA(uint32_t sampling_rate, NPA *npa,
            bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator) :
            FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_INDEX,
                    npa, s_allocator) {

    this->original_size = sa_n;
    sample(SA, sa_n);

}

SampledByIndexISA::SampledByIndexISA(uint32_t sampling_rate, NPA *npa,
        SuccinctAllocator &s_allocator) : FlatSampledArray(sampling_rate,
                SamplingScheme::FLAT_SAMPLE_BY_INDEX, npa, s_allocator) {

    this->original_size = 0;
    this->data_bits = 0;
    this->data_size = 0;
    this->data = NULL;

}

void SampledByIndexISA::sample(bitmap_t *SA, uint64_t n) {

    data_bits = SuccinctUtils::int_log_2(n + 1);
    data_size = (n / sampling_rate) + 1;

    data = new bitmap_t;
    SuccinctBase::init_bitmap(&data, data_size * data_bits, s_allocator);

    for (uint64_t i = 0; i < n; i++) {
        uint64_t sa_val = SuccinctBase::lookup_bitmap_array(SA, i, data_bits);
        /*
        if(i % sampling_rate == 0) {
            SuccinctBase::set_bitmap_array(&data, (i / sampling_rate), sa_val,
                                            data_bits);
        }
        */
        if(sa_val % sampling_rate == 0) {
            SuccinctBase::set_bitmap_array(&data, (sa_val / sampling_rate), i,
                                            data_bits);
        }
    }
}

uint64_t SampledByIndexISA::operator [](uint64_t i) {

    assert(i < original_size);
    uint64_t sample_idx = i / sampling_rate;
    uint64_t sample = SuccinctBase::lookup_bitmap_array(data, sample_idx, data_bits);
    uint64_t pos = sample;
    i -= (sample_idx * sampling_rate);
    while(i--) {
        pos = (*npa)[pos];
    }
    return pos;

}
