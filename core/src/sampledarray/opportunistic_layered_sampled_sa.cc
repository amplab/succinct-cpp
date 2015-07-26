#include "../../include/sampledarray/opportunistic_layered_sampled_sa.h"

OpportunisticLayeredSampledSA::OpportunisticLayeredSampledSA(
    uint32_t target_sampling_rate, uint32_t base_sampling_rate, NPA *npa,
    bitmap_t *SA, uint64_t sa_n, SuccinctAllocator &s_allocator)
    : OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate,
                                       SA, sa_n, s_allocator) {
  this->npa = npa;
  layered_sample(SA, sa_n);
}

OpportunisticLayeredSampledSA::OpportunisticLayeredSampledSA(
    uint32_t target_sampling_rate, uint32_t base_sampling_rate, NPA *npa,
    SuccinctAllocator &s_allocator)
    : OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate,
                                       s_allocator) {
  this->npa = npa;
}

void OpportunisticLayeredSampledSA::layered_sample(bitmap_t *SA, uint64_t n) {
  for (uint64_t i = 0; i < n; i++) {
    uint64_t sa_val = SuccinctBase::lookup_bitmap_array(SA, i, data_bits);
    if (i % target_sampling_rate == 0) {
      layer_t l;
      get_layer(&l, i);
      bitmap_t *data = layer_data[l.layer_id];
      SuccinctBase::set_bitmap_array(&data, l.layer_idx, sa_val, data_bits);
    }
  }
}

uint64_t OpportunisticLayeredSampledSA::operator[](uint64_t i) {
  assert(i < original_size);

  uint64_t j = 0;
  uint64_t original_i = i;
  std::vector<std::pair<uint64_t, uint64_t>> opt;
  while (!is_sampled(i)) {
    i = (*npa)[i];
    j++;
    if (is_idx_marked_for_creation(i)) {
      opt.push_back(std::pair<uint64_t, uint64_t>(i, j));
    }
  }

  uint64_t sa_val = sampled_at(i / target_sampling_rate);

  for (size_t k = 0; k < opt.size(); k++) {
    uint64_t count = j - opt[k].second;
    uint64_t pos = opt[k].first;
    uint64_t cur_val =
        (sa_val < count) ? original_size - (count - sa_val) : sa_val - count;
    store_without_check(pos, cur_val);
  }

  if (sa_val < j) {
    store(original_i, (original_size - (j - sa_val)));
    return original_size - (j - sa_val);
  } else {
    store(original_i, (sa_val - j));
    return sa_val - j;
  }

  return 0;
}
