#ifndef LAYERED_SUCCINCT_SHARD_HPP
#define LAYERED_SUCCINCT_SHARD_HPP

#include "succinct/SuccinctShard.hpp"

class LayeredSuccinctShard : public SuccinctShard {
private:
    bool opportunistic;

public:
    LayeredSuccinctShard(uint32_t id, std::string datafile, bool construct = true, uint32_t sa_sampling_rate = 32,
            uint32_t isa_sampling_rate = 32, uint32_t sampling_range = 1024, bool opportunistic = false,
            uint32_t npa_sampling_rate = 128,
            NPA::NPAEncodingScheme npa_encoding_scheme = NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED,
            uint32_t context_len = 3);

    virtual ~LayeredSuccinctShard() {}

    size_t remove_layer(uint32_t layer_id);

    size_t reconstruct_layer(uint32_t layer_id);

    void get(std::string &result, int64_t key);

    uint64_t num_sampled_values();
};

#endif
