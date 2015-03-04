#include "succinct/sampledarray/LayeredSampledSA.hpp"

LayeredSampledSA::LayeredSampledSA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
        NPA *npa, bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator) :
        LayeredSampledArray(target_sampling_rate, base_sampling_rate, SA, sa_n, s_allocator)
{
    this->npa = npa;
    layered_sample(SA, sa_n);
}

LayeredSampledSA::LayeredSampledSA(uint32_t target_sampling_rate, uint32_t base_sampling_rate,
        NPA *npa, SuccinctAllocator &s_allocator) :
        LayeredSampledArray(target_sampling_rate, base_sampling_rate, s_allocator)
{
    this->npa = npa;
}

void LayeredSampledSA::layered_sample(bitmap_t *SA, uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
        uint64_t sa_val = SuccinctBase::lookup_bitmap_array(SA, i, data_bits);
        if(i % target_sampling_rate == 0) {
            layer_t l;
            get_layer(&l, i);
            bitmap_t *data = layer_data[l.layer_id];
            SuccinctBase::set_bitmap_array(&data, l.layer_idx, sa_val,
                                data_bits);
        }
    }
}

uint64_t LayeredSampledSA::operator[](uint64_t i) {
    assert(i < original_size);

    uint64_t j = 0;
    while (!is_sampled(i)) {
        i = (*npa)[i];
        j++;
    }

    uint64_t sa_val = sampled_at(i / target_sampling_rate);
    if(sa_val < j)
        return original_size - (j - sa_val);
    else
        return sa_val - j;

    return 0;
}

size_t LayeredSampledSA::reconstruct_layer(uint32_t layer_id) {
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
            uint64_t j = 0;
            std::vector<std::pair<uint64_t, uint64_t>> opt;
            opt.push_back(std::pair<uint64_t, uint64_t>(idx, j));
            while (!is_sampled(idx)) {
                idx = (*npa)[idx];
                j++;
                if((idx - offset) % layer_sampling_rate == 0) {
                    opt.push_back(std::pair<uint64_t, uint64_t>(idx, j));
                }
            }
            uint64_t sa_val = sampled_at(idx / target_sampling_rate);
            for(size_t k = 0; k < opt.size(); k++) {
                uint64_t count = j - opt[k].second;
                uint64_t pos = (idx - offset) / layer_sampling_rate;
                uint64_t cur_val = (sa_val < count) ? original_size - (count - sa_val) : sa_val - count;
                SuccinctBase::set_bitmap_array(&layer_data[layer_id], pos, cur_val, data_bits);
                is_computed[pos] = true;
            }
        }
        size = layer_data[layer_id]->size;
        CREATE_LAYER(layer_id);
    }
    return size;
}
