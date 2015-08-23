#include "sampledarray/opportunistic_layered_sampled_sa.h"

OpportunisticLayeredSampledSA::OpportunisticLayeredSampledSA(
    uint32_t target_sampling_rate, uint32_t base_sampling_rate, NPA *npa,
    ArrayStream& sa_stream, uint64_t sa_n, SuccinctAllocator &s_allocator)
    : OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate,
                                       sa_n, s_allocator) {
  this->npa = npa;
  SampleLayered(sa_stream, sa_n);
}

OpportunisticLayeredSampledSA::OpportunisticLayeredSampledSA(
    uint32_t target_sampling_rate, uint32_t base_sampling_rate, NPA *npa,
    SuccinctAllocator &s_allocator)
    : OpportunisticLayeredSampledArray(target_sampling_rate, base_sampling_rate,
                                       s_allocator) {
  this->npa = npa;
}

void OpportunisticLayeredSampledSA::SampleLayered(ArrayStream& sa_stream,
                                                  uint64_t n) {
  for (uint64_t i = 0; i < n; i++) {
    uint64_t sa_val = sa_stream.Get();
    if (i % target_sampling_rate_ == 0) {
      Layer l;
      GetLayer(&l, i);
      bitmap_t *data = layer_data_[l.layer_id];
      SuccinctBase::SetBitmapArray(&data, l.layer_idx, sa_val, data_bits_);
    }
  }
}

uint64_t OpportunisticLayeredSampledSA::operator[](uint64_t i) {
  assert(i < original_size_);

  uint64_t j = 0;
  uint64_t original_i = i;
  std::vector<std::pair<uint64_t, uint64_t>> opt;
  while (!IsSampled(i)) {
    i = (*npa)[i];
    j++;
    if (IsIndexMarkedForCreation(i)) {
      opt.push_back(std::pair<uint64_t, uint64_t>(i, j));
    }
  }

  uint64_t sa_val = GetSampleAt(i / target_sampling_rate_);

  for (size_t k = 0; k < opt.size(); k++) {
    uint64_t count = j - opt[k].second;
    uint64_t pos = opt[k].first;
    uint64_t cur_val =
        (sa_val < count) ? original_size_ - (count - sa_val) : sa_val - count;
    StoreWithoutCheck(pos, cur_val);
  }

  if (sa_val < j) {
    Store(original_i, (original_size_ - (j - sa_val)));
    return original_size_ - (j - sa_val);
  } else {
    Store(original_i, (sa_val - j));
    return sa_val - j;
  }

  return 0;
}
