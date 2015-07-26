#include "sampledarray/SampledByValueSA.hpp"

SampledByValueSA::SampledByValueSA(uint32_t sampling_rate, NPA *npa,
        bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator,
        SuccinctBase *s_base) : FlatSampledArray(sampling_rate,
                SamplingScheme::FLAT_SAMPLE_BY_VALUE, npa, s_allocator) {

    assert(ISPOWOF2(sampling_rate));

    this->s_base = s_base;
    this->d_bpos = NULL;
    this->original_size = sa_n;
    sample(SA, sa_n);
}

SampledByValueSA::SampledByValueSA(uint32_t sampling_rate, NPA *npa,
        SuccinctAllocator &s_allocator, SuccinctBase *s_base) :
                FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_VALUE,
                        npa, s_allocator) {

    assert(ISPOWOF2(sampling_rate));

    this->s_base = s_base;
    this->d_bpos = NULL;
    this->original_size = 0;
    this->data_size = 0;
    this->data_bits = 0;
    this->data = NULL;

}

void SampledByValueSA::sample(bitmap_t *SA, uint64_t n) {
    data_size = (n / sampling_rate) + 1;
    data_bits = SuccinctUtils::int_log_2(data_size + 1);
    uint64_t sa_val, pos = 0;
    uint32_t orig_bits = SuccinctUtils::int_log_2(n + 1);

    bitmap_t *BPos = new bitmap_t;
    data = new bitmap_t;
    SuccinctBase::init_bitmap(&data, data_size * data_bits, s_allocator);

    SuccinctBase::init_bitmap(&BPos, n, s_allocator);

    for (uint64_t i = 0; i < n; i++) {
        sa_val = SuccinctBase::lookup_bitmap_array(SA, i, orig_bits);
        if (sa_val % sampling_rate == 0) {
            SuccinctBase::set_bitmap_array(&data, pos++, sa_val / sampling_rate,
                                            data_bits);
            SETBITVAL(BPos, i);
        }
    }

    d_bpos = new dictionary_t;
    s_base->create_dictionary(BPos, d_bpos);
    s_base->destroy_bitmap(&BPos, s_allocator);
}

bool SampledByValueSA::is_sampled(uint64_t i) {
    return (i == 0) ? s_base->get_rank1(d_bpos, i) :
            s_base->get_rank1(d_bpos, i) - s_base->get_rank1(d_bpos, i - 1);
}

uint64_t SampledByValueSA::operator [](uint64_t i) {
    uint64_t v = 0, r, a;
    while (!is_sampled(i)) {
        i = (*npa)[i];
        v++;
    }

    r = SuccinctUtils::modulo((int64_t)s_base->get_rank1(d_bpos, i) - 1,
                                data_size);
    a = SuccinctBase::lookup_bitmap_array(data, r, data_bits);
    return SuccinctUtils::modulo((sampling_rate * a) - v, original_size);
}

SampledByValueSA::dictionary_t* SampledByValueSA::get_d_bpos() {
    return d_bpos;
}


void SampledByValueSA::set_d_bpos(SampledByValueSA::dictionary_t *d_bpos) {
    this->d_bpos = d_bpos;
}

