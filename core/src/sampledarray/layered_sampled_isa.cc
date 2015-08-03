#include "../../include/sampledarray/layered_sampled_isa.h"

LayeredSampledISA::LayeredSampledISA(uint32_t target_sampling_rate,
                                     uint32_t base_sampling_rate, NPA *npa,
                                     bitmap_t *SA, uint64_t sa_n,
                                     SuccinctAllocator &s_allocator)
    : LayeredSampledArray(target_sampling_rate, base_sampling_rate, SA, sa_n,
                          s_allocator) {
  this->npa = npa;
  SampleLayered(SA, sa_n);
}

LayeredSampledISA::LayeredSampledISA(uint32_t target_sampling_rate,
                                     uint32_t base_sampling_rate, NPA *npa,
                                     SuccinctAllocator &s_allocator)
    : LayeredSampledArray(target_sampling_rate, base_sampling_rate, s_allocator) {
  this->npa = npa;
}

void LayeredSampledISA::SampleLayered(bitmap_t *SA, uint64_t n) {
  for (uint64_t i = 0; i < n; i++) {
    uint64_t sa_val = SuccinctBase::lookup_bitmap_array(SA, i, data_bits_);
    if (sa_val % target_sampling_rate_ == 0) {
      Layer l;
      GetLayer(&l, sa_val);
      bitmap_t *data = layer_data_[l.layer_id];
      SuccinctBase::set_bitmap_array(&data, l.layer_idx, i, data_bits_);
    }
  }
}

uint64_t LayeredSampledISA::operator[](uint64_t i) {
  assert(i < original_size_);

  Layer l;
  i = GetLayerLeq(&l, i);
  uint64_t pos = SuccinctBase::lookup_bitmap_array(layer_data_[l.layer_id],
                                                   l.layer_idx, data_bits_);

  while (i--) {
    pos = (*npa)[pos];
  }
  return pos;
}

size_t LayeredSampledISA::ReconstructLayer(uint32_t layer_id) {
  size_t size = 0;
  if (!EXISTS_LAYER(layer_id)) {
    layer_data_[layer_id] = new bitmap_t;
    uint32_t layer_sampling_rate = (1 << layer_id) * target_sampling_rate_;
    layer_sampling_rate =
        (layer_id == (num_layers_ - 1)) ?
            layer_sampling_rate : layer_sampling_rate * 2;
    uint64_t num_entries = (original_size_ / layer_sampling_rate) + 1;
    SuccinctBase::init_bitmap(&layer_data_[layer_id], num_entries * data_bits_,
                              succinct_allocator_);
    uint64_t idx, offset;
    std::vector<bool> is_computed(num_entries, false);
    offset = (layer_id == this->num_layers_ - 1) ? 0 : (layer_sampling_rate / 2);
    for (uint64_t i = 0; i < num_entries; i++) {
      idx = i * layer_sampling_rate + offset;
      if (idx > original_size_)
        break;
      if (is_computed[i])
        continue;
      Layer l;
      idx = GetLayerLeq(&l, idx);
      uint64_t pos = SuccinctBase::lookup_bitmap_array(layer_data_[l.layer_id],
                                                       l.layer_idx, data_bits_);
      while (idx--) {
        pos = (*npa)[pos];
      }
      SuccinctBase::set_bitmap_array(&layer_data_[layer_id], i, pos, data_bits_);
    }
    size = layer_data_[layer_id]->size;
    CREATE_LAYER(layer_id);
  }
  return size;
}
