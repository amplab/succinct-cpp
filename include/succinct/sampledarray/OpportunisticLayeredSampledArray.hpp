#ifndef OPPORTUNISTIC_LAYERED_SAMPLED_ARRAY_HPP
#define OPPORTUNISTIC_LAYERED_SAMPLED_ARRAY_HPP

#include "succinct/sampledarray/LayeredSampledArray.hpp"

class OpportunisticLayeredSampledArray : public LayeredSampledArray {
private:
    bitmap_t **is_layer_value_sampled;
    uint64_t LAYER_TO_CREATE;

    std::atomic<uint64_t> num_sampled_values;

#define IS_LAYER_VAL_SAMPLED(l, i)  ACCESSBIT(this->is_layer_value_sampled[(l)], (i))
#define SET_LAYER_VAL_SAMPLED(l, i) SETBITVAL(this->is_layer_value_sampled[(l)], (i))
#define MARK_FOR_CREATION(i)        SETBIT((this->LAYER_TO_CREATE), (i))
#define UNMARK_FOR_CREATION(i)      CLEARBIT((this->LAYER_TO_CREATE), (i))
#define IS_MARKED_FOR_CREATION(i)   GETBIT((this->LAYER_TO_CREATE), (i))

public:
    OpportunisticLayeredSampledArray(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
            bitmap_t *SA, uint64_t n, SuccinctAllocator &s_allocator) :
            LayeredSampledArray(target_sampling_rate, base_sampling_rate, SA, n, s_allocator) {
        this->is_layer_value_sampled = new bitmap_t*[this->num_layers];
        this->LAYER_TO_CREATE = 0;
        this->num_sampled_values = 0;
        for(uint32_t i = 0; i < this->num_layers; i++) {
            this->is_layer_value_sampled[i] = new bitmap_t;
            uint32_t layer_sampling_rate = (1 << i) * target_sampling_rate;
            layer_sampling_rate = (i == (this->num_layers - 1)) ?
                    layer_sampling_rate : layer_sampling_rate * 2;
            uint64_t num_entries = (n / layer_sampling_rate) + 1;
            this->num_sampled_values += num_entries;
            SuccinctBase::init_bitmap_set(&is_layer_value_sampled[i], num_entries, s_allocator);
        }
    }

    OpportunisticLayeredSampledArray(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
            SuccinctAllocator &s_allocator) :
            LayeredSampledArray(target_sampling_rate, base_sampling_rate, s_allocator) {
        this->is_layer_value_sampled = NULL;
        this->LAYER_TO_CREATE = 0;
        this->num_sampled_values = 0;
    }

    virtual inline uint64_t get_layer_leq(layer_t *l, int64_t i) {
        int32_t layer_offset = (i / target_sampling_rate) % sampling_range;
        uint64_t n_hops = (i % target_sampling_rate);
        i -= n_hops;
        int64_t l_idx = (i / base_sampling_rate) * count[layer[layer_offset]] + layer_idx[layer_offset];
        while(!EXISTS_LAYER(layer[layer_offset]) ||
                !IS_LAYER_VAL_SAMPLED(layer[layer_offset], l_idx)) {
            layer_offset--;
            i -= target_sampling_rate;
            n_hops += target_sampling_rate;
            if(layer_offset < 0) layer_offset += num_layers;
            if(i < 0) i += original_size;
            l_idx = (i / base_sampling_rate) * count[layer[layer_offset]] + layer_idx[layer_offset];
        }
        l->layer_id = layer[layer_offset];
        l->layer_idx = l_idx;
        return n_hops;
    }

    virtual bool is_sampled(uint64_t i) {
        uint32_t layer_offset = (i / target_sampling_rate) % sampling_range;
        uint32_t layer_id = layer[layer_offset];
        uint64_t l_idx = (i / base_sampling_rate) * count[layer_id] + layer_idx[layer_offset];
        return (i % target_sampling_rate == 0) &&
                        EXISTS_LAYER(layer_id) &&
                        IS_LAYER_VAL_SAMPLED(layer_id, l_idx);
    }

    bool store(uint64_t i, uint64_t val) {
        if(i % target_sampling_rate == 0) {
            layer_t l;
            get_layer(&l, i);
            if(IS_MARKED_FOR_CREATION(l.layer_id) &&
                    !IS_LAYER_VAL_SAMPLED(l.layer_id, l.layer_idx)) {
                SuccinctBase::set_bitmap_array(&layer_data[l.layer_id], l.layer_idx, val, data_bits);
                SET_LAYER_VAL_SAMPLED(l.layer_id, l.layer_idx);
                this->num_sampled_values++;
            }
        }
        return false;
    }

    virtual size_t delete_layer(uint32_t layer_id) {
        size_t size = 0;
        if(EXISTS_LAYER(layer_id)) {
            if(IS_MARKED_FOR_CREATION(layer_id)) {
                UNMARK_FOR_CREATION(layer_id);
            }
            DELETE_LAYER(layer_id);
            size = layer_data[layer_id]->size;
            for(uint64_t i = 0; i < is_layer_value_sampled[layer_id]->size; i++) {
                this->num_sampled_values -= IS_LAYER_VAL_SAMPLED(layer_id, i);
            }
            SuccinctBase::clear_bitmap(&is_layer_value_sampled[layer_id], s_allocator);
            usleep(10000);                      // TODO: I don't like this!
            SuccinctBase::destroy_bitmap(&layer_data[layer_id], s_allocator);
            layer_data[layer_id] = NULL;
        }
        return size;
    }

    virtual size_t reconstruct_layer(uint32_t layer_id) {
        size_t add_size = 0;
        if(!EXISTS_LAYER(layer_id)) {
            layer_data[layer_id] = new bitmap_t;
            uint32_t layer_sampling_rate = (1 << layer_id) * target_sampling_rate;
            layer_sampling_rate = (layer_id == (num_layers - 1)) ?
                    layer_sampling_rate : layer_sampling_rate * 2;
            uint64_t num_entries = (original_size / layer_sampling_rate) + 1;
            SuccinctBase::init_bitmap(&layer_data[layer_id], num_entries * data_bits, s_allocator);
            add_size = layer_data[layer_id]->size;
            CREATE_LAYER(layer_id);
            if(!IS_MARKED_FOR_CREATION(layer_id)) {
                MARK_FOR_CREATION(layer_id);
            }
        }
        return add_size;
    }

    virtual size_t deserialize(std::istream& in) {
        size_t in_size = LayeredSampledArray::deserialize(in);
        this->is_layer_value_sampled = new bitmap_t*[this->num_layers];
        for(uint32_t i = 0; i < this->num_layers; i++) {
            this->is_layer_value_sampled[i] = new bitmap_t;
            uint32_t layer_sampling_rate = (1 << i) * target_sampling_rate;
            layer_sampling_rate = (i == (this->num_layers - 1)) ?
                    layer_sampling_rate : layer_sampling_rate * 2;
            uint64_t num_entries = (original_size / layer_sampling_rate) + 1;
            SuccinctBase::init_bitmap_set(&is_layer_value_sampled[i], num_entries, s_allocator);
            this->num_sampled_values += num_entries;
        }
        return in_size;
    }

    uint64_t get_num_sampled_values() {
        uint64_t count = 0;
        for(uint32_t i = 0; i< num_layers; i++) {
            for(uint64_t j = 0; j < is_layer_value_sampled[i]->size; i++) {
                count++;
            }
        }
        fprintf(stderr, "Count = %lu\n", count);
        return this->num_sampled_values;
    }
};
#endif
