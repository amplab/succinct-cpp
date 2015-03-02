#include "succinct/LayeredSuccinctShard.hpp"

LayeredSuccinctShard::LayeredSuccinctShard(uint32_t id, std::string datafile, bool construct, uint32_t sa_sampling_rate,
            uint32_t isa_sampling_rate, uint32_t sampling_range, uint32_t npa_sampling_rate,
            NPA::NPAEncodingScheme npa_encoding_scheme, uint32_t context_len)
            : SuccinctShard(id, datafile, construct, sa_sampling_rate, isa_sampling_rate, npa_sampling_rate,
                    SamplingScheme::LAYERED_SAMPLE_BY_INDEX, SamplingScheme::LAYERED_SAMPLE_BY_INDEX,
                    npa_encoding_scheme, context_len, sampling_range) {

}

size_t LayeredSuccinctShard::remove_layer(uint32_t layer_id) {
    size_t size = 0;
    size += ((LayeredSampledArray *)SA)->delete_layer(layer_id);
    size += ((LayeredSampledArray *)ISA)->delete_layer(layer_id);
    return size;
}

size_t LayeredSuccinctShard::reconstruct_layer(uint32_t layer_id) {
    size_t size = 0;
    size += ((LayeredSampledArray *)SA)->reconstruct_layer(layer_id);
    size += ((LayeredSampledArray *)ISA)->reconstruct_layer(layer_id);
    return size;
}
