#include "../../include/sampledarray/opportunistic_layered_sampled_isa.h"

OpportunisticLayeredSampledISA::OpportunisticLayeredSampledISA(
    uint32_t target_sampling_rate, uint32_t base_sampling_rate, NPA *npa,
    bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator)
    : OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate,
                                       SA, sa_n, s_allocator) {
  this->npa_ = npa;
  SampleLayered(SA, sa_n);
}

OpportunisticLayeredSampledISA::OpportunisticLayeredSampledISA(
    uint32_t target_sampling_rate, uint32_t base_sampling_rate, NPA *npa,
    SuccinctAllocator &s_allocator)
    : OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate,
                                       s_allocator) {
  this->npa_ = npa;
}

void OpportunisticLayeredSampledISA::SampleLayered(bitmap_t *SA, uint64_t n) {
  for (uint64_t i = 0; i < n; i++) {
    uint64_t sa_val = SuccinctBase::LookupBitmapArray(SA, i, data_bits_);
    if (sa_val % target_sampling_rate_ == 0) {
      Layer l;
      GetLayer(&l, sa_val);
      bitmap_t *data = layer_data_[l.layer_id];
      SuccinctBase::SetBitmapArray(&data, l.layer_idx, i, data_bits_);
    }
  }
}

uint64_t OpportunisticLayeredSampledISA::operator[](uint64_t i) {
  assert(i < original_size_);

  Layer l;
  i = GetLayerLeq(&l, i);
  uint64_t pos = SuccinctBase::LookupBitmapArray(layer_data_[l.layer_id],
                                                   l.layer_idx, data_bits_);

  while (i--) {
    pos = (*npa_)[pos];
  }

  return pos;
}
