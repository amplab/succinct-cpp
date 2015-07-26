#include "../../include/sampledarray/opportunistic_layered_sampled_isa.h"

OpportunisticLayeredSampledISA::OpportunisticLayeredSampledISA(
    uint32_t target_sampling_rate, uint32_t base_sampling_rate, NPA *npa,
    bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator)
    : OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate,
                                       SA, sa_n, s_allocator) {
  this->npa = npa;
  layered_sample(SA, sa_n);
}

OpportunisticLayeredSampledISA::OpportunisticLayeredSampledISA(
    uint32_t target_sampling_rate, uint32_t base_sampling_rate, NPA *npa,
    SuccinctAllocator &s_allocator)
    : OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate,
                                       s_allocator) {
  this->npa = npa;
}

void OpportunisticLayeredSampledISA::layered_sample(bitmap_t *SA, uint64_t n) {
  for (uint64_t i = 0; i < n; i++) {
    uint64_t sa_val = SuccinctBase::lookup_bitmap_array(SA, i, data_bits);
    if (sa_val % target_sampling_rate == 0) {
      layer_t l;
      get_layer(&l, sa_val);
      bitmap_t *data = layer_data[l.layer_id];
      SuccinctBase::set_bitmap_array(&data, l.layer_idx, i, data_bits);
    }
  }
}

uint64_t OpportunisticLayeredSampledISA::operator[](uint64_t i) {
  assert(i < original_size);

  layer_t l;
  i = get_layer_leq(&l, i);
  uint64_t pos = SuccinctBase::lookup_bitmap_array(layer_data[l.layer_id],
                                                   l.layer_idx, data_bits);

  while (i--) {
    pos = (*npa)[pos];
  }

  return pos;
}
