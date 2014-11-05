#ifndef SAMPLED_BY_INDEX_ISA_H
#define SAMPLED_BY_INDEX_ISA_H

#include "succinct/sampledarray/SampledArray.hpp"

class SampledByIndexISA : public SampledArray {
protected:
    // Sample by index for ISA using original SA
    virtual void sample(bitmap_t *original, uint64_t n);

public:
    // Constructor
   SampledByIndexISA(uint32_t sampling_rate, NPA *npa, bitmap_t *SA,
           uint64_t sa_n, SuccinctAllocator &s_allocator);

   SampledByIndexISA(uint32_t sampling_rate, NPA *npa,
           SuccinctAllocator &s_allocator);

    // Access element at index i
    virtual uint64_t operator[](uint64_t i);
};

#endif
