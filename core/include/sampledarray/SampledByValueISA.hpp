#ifndef SAMPLED_BY_VALUE_ISA_H
#define SAMPLED_BY_VALUE_ISA_H

#include "sampledarray/FlatSampledArray.hpp"

class SampledByValueISA : public FlatSampledArray {
protected:
    // Sample by value for ISA using original SA
    virtual void sample(bitmap_t *original, uint64_t n);

public:
    // Constructor
    SampledByValueISA(uint32_t sampling_rate, NPA *npa, bitmap_t *SA,
                uint64_t sa_n, dictionary_t *d_bpos,
                SuccinctAllocator &s_allocator, SuccinctBase *s_base);

    SampledByValueISA(uint32_t sampling_rate, NPA *npa,
            SuccinctAllocator &s_allocator, SuccinctBase *s_base);

    // Access element at index i
    virtual uint64_t operator[](uint64_t i);

    // Is the value sampled at index i
    virtual bool is_sampled(uint64_t i);

    // Set d_bpos
    void set_d_bpos(dictionary_t *d_bpos);

private:
    dictionary_t *d_bpos;
    SuccinctBase *_base;

};

#endif
