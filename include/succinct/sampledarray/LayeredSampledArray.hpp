#ifndef LAYERED_SAMPLED_ARRAY_HPP
#define LAYERED_SAMPLED_ARRAY_HPP

#include "succinct/utils/assertions.hpp"
#include "succinct/utils/definitions.hpp"
#include "succinct/utils/SuccinctUtils.hpp"
#include "succinct/utils/SuccinctAllocator.hpp"
#include "succinct/SuccinctBase.hpp"
#include "succinct/npa/NPA.hpp"
#include "succinct/sampledarray/SampledArray.hpp"

#include <cstdio>
#include <map>

class LayeredSampledArray : public SampledArray {
protected:
    typedef SuccinctBase::BitMap bitmap_t;
    typedef SuccinctBase::Dictionary dictionary_t;

#define CREATE_LAYER(i)    SETBIT((this->LAYER_MAP), (i))
#define DELETE_LAYER(i)    CLEARBIT((this->LAYER_MAP), (i))
#define EXISTS_LAYER(i)    GETBIT((this->LAYER_MAP), (i))

    uint32_t num_layers;                // Number of layers (Do not serialize)
    uint64_t LAYER_MAP;                 // Bitmap indicating which layers are stored
    std::map<uint32_t, uint64_t> count; // Count of each layer in the sampling_ranges (Do not serialize)
    uint32_t *layer;                    // Sample offset to layer mapping (Do not serialize)
    uint64_t *layer_idx;                // Sample offset to index into layer mapping (Do not serialize)
    uint32_t sampling_range;            // base_sampling_rate / target_sampling_rate (Do not serialize)
    uint32_t target_sampling_rate;      // The target sampling rate (i.e., top most layer) (Do not serialize)
    uint32_t base_sampling_rate;        // The base sampling rate (i.e, lower most layer) (Do not serialize)

    bitmap_t **layer_data;              // All of the layered data content
    uint8_t data_bits;                  // Width of each data entry in bits
    uint64_t original_size;             // Number of elements in original array
    SuccinctAllocator s_allocator;

public:

    typedef struct {
        uint32_t layer_id;
        uint64_t layer_idx;
    } layer_t;

    LayeredSampledArray(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
            bitmap_t *SA, uint64_t n, SuccinctAllocator &s_allocator)
            : SampledArray(SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {

        assert(target_sampling_rate < base_sampling_rate);
        assert(ISPOWOF2(target_sampling_rate));
        assert(ISPOWOF2(base_sampling_rate));
        this->target_sampling_rate = target_sampling_rate;
        this->base_sampling_rate = base_sampling_rate;
        this->sampling_range = base_sampling_rate / target_sampling_rate;
        this->num_layers = SuccinctUtils::int_log_2(sampling_range) + 1;
        this->layer = new uint32_t[sampling_range];
        this->layer_idx = new uint64_t[sampling_range];

        for(uint32_t i = 0; i < sampling_range; i++) {
            layer[i] = SuccinctUtils::int_log_2(gcd(i, sampling_range));
            layer_idx[i] = count[layer[i]];
            count[layer[i]]++;
        }

        this->LAYER_MAP = 0;
        this->layer_data = new bitmap_t*[this->num_layers];
        this->original_size = n;
        this->data_bits = SuccinctUtils::int_log_2(n + 1);

        for(uint32_t i = 0; i < this->num_layers; i++) {
            CREATE_LAYER(i);
            layer_data[i] = new bitmap_t;
            uint32_t layer_sampling_rate = (1 << i) * target_sampling_rate;
            uint64_t num_entries = (n / layer_sampling_rate) + 1;
            SuccinctBase::init_bitmap(&layer_data[i], num_entries * data_bits, s_allocator);
        }

        this->s_allocator = s_allocator;
    }

    LayeredSampledArray(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
                SuccinctAllocator &s_allocator)
                : SampledArray(SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {
        assert(target_sampling_rate < base_sampling_rate);
        assert(ISPOWOF2(target_sampling_rate));
        assert(ISPOWOF2(base_sampling_rate));
        this->target_sampling_rate = target_sampling_rate;
        this->base_sampling_rate = base_sampling_rate;
        this->sampling_range = base_sampling_rate / target_sampling_rate;
        this->num_layers = SuccinctUtils::int_log_2(sampling_range) + 1;
        this->layer = new uint32_t[sampling_range];
        this->layer_idx = new uint64_t[sampling_range];

        for(uint32_t i = 0; i < sampling_range; i++) {
            layer[i] = SuccinctUtils::int_log_2(gcd(i, sampling_range));
            layer_idx[i] = count[layer[i]];
            count[layer[i]]++;
        }

        this->LAYER_MAP = 0;
        this->layer_data = NULL;
        this->original_size = 0;
        this->data_bits = 0;
        this->s_allocator = s_allocator;
    }

    inline void get_layer(layer_t *l, uint64_t i) {
        uint32_t layer_offset = (i / target_sampling_rate) % sampling_range;
        l->layer_id = layer[layer_offset];
        l->layer_idx = (i / base_sampling_rate) * count[l->layer_id] + layer_idx[layer_offset];
    }

    inline void get_layer_leq(layer_t *l, uint64_t i) {
        int32_t layer_offset = (i / target_sampling_rate) % sampling_range;
        while(!EXISTS_LAYER(layer[layer_offset])) {
            // fprintf(stderr, "Layer does not exist! layer = %u\n", layer[layer_offset]);
            layer_offset--; i--;
            if(layer_offset < 0) layer_offset += num_layers;
        }
        l->layer_id = layer[layer_offset];
        l->layer_idx = (i / base_sampling_rate) * count[l->layer_id] + layer_idx[layer_offset];
    }

    inline uint32_t get_base_sampling_rate() {
        return base_sampling_rate;
    }

    inline uint32_t get_target_sampling_rate() {
        return target_sampling_rate;
    }

    inline uint32_t get_sampling_rate() {
        return get_target_sampling_rate();
    }

    inline uint32_t get_sampling_range() {
        return sampling_range;
    }

    inline uint64_t get_original_size() {
        return original_size;
    }

    inline uint8_t get_data_bits() {
        return data_bits;
    }

    bool is_sampled(uint64_t i) {
        return (i % target_sampling_rate == 0) &&
                EXISTS_LAYER(layer[(i / target_sampling_rate) % sampling_range]);
    }

    uint64_t sampled_at(uint64_t i) {
        i *= target_sampling_rate;

        layer_t l;
        get_layer(&l, i);
        return SuccinctBase::lookup_bitmap_array(layer_data[l.layer_id],
                l.layer_idx, data_bits);
    }

    void delete_layer(uint32_t layer_id) {
        if(EXISTS_LAYER(layer_id)) {
            DELETE_LAYER(layer_id);
            SuccinctBase::destroy_bitmap(&layer_data[layer_id], s_allocator);
        }
    }

    void reconstruct_layer(uint32_t layer_id) {
        if(!EXISTS_LAYER(layer_id)) {
            layer_data[layer_id] = new bitmap_t;
            uint32_t layer_sampling_rate = (1 << layer_id) * target_sampling_rate;
            uint64_t num_entries = (original_size / layer_sampling_rate) + 1;
            SuccinctBase::init_bitmap(&layer_data[layer_id], num_entries * data_bits, s_allocator);
            CREATE_LAYER(layer_id);
        }
    }

    virtual uint64_t operator[](uint64_t i) = 0;

    virtual size_t serialize(std::ostream& out) {
        size_t out_size = 0;

        out.write(reinterpret_cast<const char *>(&LAYER_MAP), sizeof(uint64_t));
        out_size += sizeof(uint64_t);
        out.write(reinterpret_cast<const char *>(&data_bits), sizeof(uint8_t));
        out_size += sizeof(uint8_t);
        out.write(reinterpret_cast<const char *>(&original_size), sizeof(uint64_t));
        out_size += sizeof(uint64_t);

        for(uint32_t i = 0; i < this->num_layers; i++) {
            out_size += SuccinctBase::serialize_bitmap(layer_data[i], out);
        }

        return out_size;
    }

    virtual size_t deserialize(std::istream& in) {
        size_t in_size = 0;

        in.read(reinterpret_cast<char *>(&LAYER_MAP), sizeof(uint64_t));
        in_size += sizeof(uint64_t);
        in.read(reinterpret_cast<char *>(&data_bits), sizeof(uint8_t));
        in_size += sizeof(uint8_t);
        in.read(reinterpret_cast<char *>(&original_size), sizeof(uint64_t));
        in_size += sizeof(uint64_t);

        layer_data = new bitmap_t*[this->num_layers];
        for(uint32_t i = 0; i < this->num_layers; i++) {
            in_size += SuccinctBase::deserialize_bitmap(&layer_data[i], in);
        }

        return in_size;
    }

private:
    uint32_t gcd(uint32_t a, uint32_t b) {
        return b == 0 ? a : gcd(b, a % b);
    }

};

#endif /* LAYERED_SAMPLED_ARRAY_HPP */
