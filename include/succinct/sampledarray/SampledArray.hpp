#ifndef SAMPLED_ARRAY_HPP
#define SAMPLED_ARRAY_HPP

#include "succinct/sampledarray/SamplingScheme.hpp"

class SampledArray {
private:
    SamplingScheme scheme;

public:
    SampledArray(SamplingScheme scheme) { this->scheme = scheme; }
    virtual ~SampledArray() {}

    virtual uint64_t operator[](uint64_t i) = 0;
    virtual uint64_t at(uint64_t i) { return operator[](i); }

    virtual size_t serialize(std::ostream& out) = 0;
    virtual size_t deserialize(std::istream& in) = 0;

    SamplingScheme get_sampling_scheme() { return scheme; }

    virtual uint32_t get_sampling_rate() = 0;

    virtual bool is_sampled(uint64_t i) = 0;

    virtual size_t storage_size() = 0;
};

#endif /* SAMPLED_ARRAY_HPP */
