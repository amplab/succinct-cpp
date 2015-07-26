#include "sampledarray/LayeredSampledISA.hpp"

LayeredSampledISA::LayeredSampledISA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
        NPA *npa, bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator) :
        LayeredSampledArray(target_sampling_rate, base_sampling_rate, SA, sa_n, s_allocator)
{
    this->npa = npa;
    layered_sample(SA, sa_n);
}

LayeredSampledISA::LayeredSampledISA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
        NPA *npa, SuccinctAllocator &s_allocator) :
        LayeredSampledArray(target_sampling_rate, base_sampling_rate, s_allocator)
{
    this->npa = npa;
}

void LayeredSampledISA::layered_sample(bitmap_t *SA, uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
        uint64_t sa_val = SuccinctBase::lookup_bitmap_array(SA, i, data_bits);
        if(sa_val % target_sampling_rate == 0) {
            layer_t l;
            get_layer(&l, sa_val);
            bitmap_t *data = layer_data[l.layer_id];
            SuccinctBase::set_bitmap_array(&data, l.layer_idx, i,
                                data_bits);
        }
    }
}

uint64_t LayeredSampledISA::operator[](uint64_t i) {
    assert(i < original_size);

    layer_t l;
    i = get_layer_leq(&l, i);
    uint64_t pos = SuccinctBase::lookup_bitmap_array(layer_data[l.layer_id], l.layer_idx, data_bits);

    while(i--) {
        pos = (*npa)[pos];
    }
    return pos;
}

size_t LayeredSampledISA::reconstruct_layer(uint32_t layer_id) {
    size_t size = 0;
    if(!EXISTS_LAYER(layer_id)) {
        layer_data[layer_id] = new bitmap_t;
        uint32_t layer_sampling_rate = (1 << layer_id) * target_sampling_rate;
        layer_sampling_rate = (layer_id == (num_layers - 1)) ?
                layer_sampling_rate : layer_sampling_rate * 2;
        uint64_t num_entries = (original_size / layer_sampling_rate) + 1;
        SuccinctBase::init_bitmap(&layer_data[layer_id], num_entries * data_bits, s_allocator);
        uint64_t idx, offset;
        std::vector<bool> is_computed(num_entries, false);
        offset = (layer_id == this->num_layers - 1) ? 0 : (layer_sampling_rate / 2);
        for(uint64_t i = 0; i < num_entries; i++) {
            idx = i * layer_sampling_rate + offset;
            if(idx > original_size) break;
            if(is_computed[i]) continue;
            layer_t l;
            idx = get_layer_leq(&l, idx);
            uint64_t pos = SuccinctBase::lookup_bitmap_array(layer_data[l.layer_id], l.layer_idx, data_bits);
            while(idx--) {
                pos = (*npa)[pos];
            }
            SuccinctBase::set_bitmap_array(&layer_data[layer_id], i, pos, data_bits);
        }
        size = layer_data[layer_id]->size;
        CREATE_LAYER(layer_id);
    }
    return size;
}
