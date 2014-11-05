#ifndef SAMPLED_ARRAY_H
#define SAMPLED_ARRAY_H

#include <ostream>

#include "succinct/npa/NPA.hpp"

#include "succinct/sampledarray/SamplingScheme.hpp"

#include "succinct/utils/SuccinctAllocator.hpp"

#include "succinct/SuccinctBase.hpp"

class SampledArray {
public:
    typedef SuccinctBase::BitMap bitmap_t;
    typedef SuccinctBase::Dictionary dictionary_t;

protected:
    bitmap_t *data;
    uint8_t data_bits;
    uint64_t data_size;
    uint64_t original_size;
    uint32_t sampling_rate;

    NPA *npa;
    SamplingScheme scheme;
    SuccinctAllocator s_allocator;

    // Sample original array using the sampling scheme
    virtual void sample(bitmap_t *original, uint64_t n) = 0;

public:
    // Constructor
    SampledArray(uint32_t sampling_rate, SamplingScheme scheme, NPA *npa,
                    SuccinctAllocator &s_allocator) {
        this->data = NULL;
        this->data_bits = 0;
        this->sampling_rate = sampling_rate;
        this->data_size = 0;
        this->original_size = 0;
        this->scheme = scheme;
        this->npa = npa;
        this->s_allocator = s_allocator;
    }

    // Virtual destructor
    virtual ~SampledArray() {};

    // Return sampling scheme
    SamplingScheme get_sampling_scheme() {
        return scheme;
    }

    uint64_t size() {
        return original_size;
    }

    uint64_t sampled_size() {
        return data_size;
    }

    void set_npa(NPA *npa) {
        this->npa = npa;
    }

    // Access sampled array at index i
    virtual uint64_t operator[](uint64_t i) = 0;

    // Access sampled array at index i
    virtual uint64_t at(uint64_t i) {
        return operator[](i);
    }

    virtual size_t serialize(std::ostream& out) {
        size_t out_size = 0;

        out.write(reinterpret_cast<const char *>(&data_size), sizeof(uint64_t));
        out_size += sizeof(uint64_t);
        out.write(reinterpret_cast<const char *>(&data_bits), sizeof(uint8_t));
        out_size += sizeof(uint8_t);
        out.write(reinterpret_cast<const char *>(&original_size), sizeof(uint64_t));
        out_size += sizeof(uint64_t);
        out.write(reinterpret_cast<const char *>(&sampling_rate), sizeof(uint32_t));
        out_size += sizeof(uint32_t);

        out_size += SuccinctBase::serialize_bitmap(data, out);

        return out_size;
    }

    virtual size_t deserialize(std::istream& in) {
        size_t in_size = 0;

        in.read(reinterpret_cast<char *>(&data_size), sizeof(uint64_t));
        in_size += sizeof(uint64_t);
        in.read(reinterpret_cast<char *>(&data_bits), sizeof(uint8_t));
        in_size += sizeof(uint8_t);
        in.read(reinterpret_cast<char *>(&original_size), sizeof(uint64_t));
        in_size += sizeof(uint64_t);
        in.read(reinterpret_cast<char *>(&sampling_rate), sizeof(uint32_t));
        in_size += sizeof(uint32_t);

        in_size += SuccinctBase::deserialize_bitmap(&data, in);

        return in_size;
    }

};

#endif
