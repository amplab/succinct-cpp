#include "succinct/sampledarray/SampledByValueISA.hpp"

SampledByValueISA::SampledByValueISA(uint32_t sampling_rate, NPA *npa,
        bitmap_t *SA, uint64_t sa_n, dictionary_t *d_bpos,
        SuccinctAllocator &s_allocator, SuccinctBase *s_base) :
        FlatSampledArray(sampling_rate,SamplingScheme::FLAT_SAMPLE_BY_VALUE, npa,
                s_allocator) {

    assert(ISPOWOF2(sampling_rate));

    this->_base = s_base;
    this->d_bpos = d_bpos;
    this->original_size = sa_n;
    sample(SA, sa_n);
}

SampledByValueISA::SampledByValueISA(uint32_t sampling_rate, NPA *npa,
        SuccinctAllocator &s_allocator, SuccinctBase *s_base) :
        FlatSampledArray(sampling_rate,SamplingScheme::FLAT_SAMPLE_BY_VALUE, npa,
                s_allocator) {

    assert(ISPOWOF2(sampling_rate));

    this->_base = s_base;
    this->d_bpos = NULL;
    this->original_size = 0;
    this->data_size = 0;
    this->data_bits = 0;
    this->data = NULL;

}

void SampledByValueISA::sample(bitmap_t *SA, uint64_t n) {
    data_size = (n / sampling_rate) + 1;
    data_bits = SuccinctUtils::int_log_2(data_size + 1);
    uint64_t sa_val, pos = 0;
    uint32_t orig_bits = SuccinctUtils::int_log_2(n + 1);

    data = new bitmap_t;
    SuccinctBase::init_bitmap(&data, data_size * data_bits, s_allocator);

    for (uint64_t i = 0; i < n; i++) {
        sa_val = SuccinctBase::lookup_bitmap_array(SA, i, orig_bits);
        if (sa_val % sampling_rate == 0) {
            SuccinctBase::set_bitmap_array(&data, sa_val / sampling_rate, pos++,
                                            data_bits);
        }
    }

}

uint64_t SampledByValueISA::operator [](uint64_t i) {
    uint64_t a, pos;
    uint64_t v = i % sampling_rate;

    a = SuccinctBase::lookup_bitmap_array(data, i / sampling_rate, data_bits);
    pos = _base->get_select1(d_bpos, a);

    while (v) {
        pos = (*npa)[pos];
        v--;
    }

    return pos;
}

bool SampledByValueISA::is_sampled(uint64_t i) {
    return i % sampling_rate == 0;
}

void SampledByValueISA::set_d_bpos(SampledByValueISA::dictionary_t *d_bpos) {
    this->d_bpos = d_bpos;
}
