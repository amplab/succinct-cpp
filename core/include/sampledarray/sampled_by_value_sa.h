#ifndef SAMPLED_BY_VALUE_SA_H
#define SAMPLED_BY_VALUE_SA_H

#include "flat_sampled_array.h"

class SampledByValueSA : public FlatSampledArray {
protected:
    // Sample original SA by value
    virtual void sample(bitmap_t *original, uint64_t n);

    // Check if index is sampled
    bool is_sampled(uint64_t i);

public:
    // Constructor
    SampledByValueSA(uint32_t sampling_rate, NPA *npa, bitmap_t *SA,
            uint64_t sa_n, SuccinctAllocator &s_allocator, SuccinctBase *_base);

    SampledByValueSA(uint32_t sampling_rate, NPA *npa,
            SuccinctAllocator &s_allocator, SuccinctBase *_base);

    // Access element at index i
    virtual uint64_t operator[](uint64_t i);

    // Get d_bpos
    dictionary_t *get_d_bpos();

    // Set d_bpos
    void set_d_bpos(dictionary_t *d_bpos);

private:
    dictionary_t *d_bpos;
    SuccinctBase *s_base;
};

#endif
