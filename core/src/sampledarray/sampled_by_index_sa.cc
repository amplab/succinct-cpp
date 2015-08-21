#include "sampledarray/sampled_by_index_sa.h"

SampledByIndexSA::SampledByIndexSA(uint32_t sampling_rate, NPA *npa,
                                   DataInputStream<uint64_t>& sa_stream, uint64_t sa_n,
                                   SuccinctAllocator &s_allocator)
    : FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_INDEX, npa,
                       s_allocator) {

  this->original_size_ = sa_n;
  Sample(sa_stream, sa_n);

}

SampledByIndexSA::SampledByIndexSA(uint32_t sampling_rate, NPA *npa,
                                   SuccinctAllocator &s_allocator)
    : FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_INDEX, npa,
                       s_allocator) {

  this->original_size_ = 0;
  this->data_bits_ = 0;
  this->data_size_ = 0;
  this->data_ = NULL;

}

void SampledByIndexSA::Sample(DataInputStream<uint64_t>& sa_stream, uint64_t n) {

  data_bits_ = SuccinctUtils::IntegerLog2(n + 1);
  data_size_ = (n / sampling_rate_) + 1;

  data_ = new bitmap_t;
  SuccinctBase::InitBitmap(&data_, data_size_ * data_bits_,
                           succinct_allocator_);

  for (uint64_t i = 0; i < n; i++) {
    uint64_t sa_val = sa_stream.Get();
    if (i % sampling_rate_ == 0) {
      SuccinctBase::SetBitmapArray(&data_, (i / sampling_rate_), sa_val,
                                   data_bits_);
    }
  }
}

uint64_t SampledByIndexSA::operator [](uint64_t i) {

  assert(i < original_size_);

  uint64_t j = 0;
  while (i % sampling_rate_ != 0) {
    i = (*npa_)[i];
    j++;
  }
  uint64_t sa_val = SuccinctBase::LookupBitmapArray(data_, i / sampling_rate_,
                                                    data_bits_);
  if (sa_val < j)
    return original_size_ - (j - sa_val);
  else
    return sa_val - j;
}
