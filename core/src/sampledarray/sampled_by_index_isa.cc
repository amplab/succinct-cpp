#include "sampledarray/sampled_by_index_isa.h"

SampledByIndexISA::SampledByIndexISA(uint32_t sampling_rate, NPA *npa,
                                     DataInputStream<uint64_t>& sa_stream, uint64_t sa_n,
                                     SuccinctAllocator &s_allocator)
    : FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_INDEX, npa,
                       s_allocator) {

  this->original_size_ = sa_n;
  Sample(sa_stream, sa_n);

}

SampledByIndexISA::SampledByIndexISA(uint32_t sampling_rate, NPA *npa,
                                     SuccinctAllocator &s_allocator)
    : FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_INDEX, npa,
                       s_allocator) {

  this->original_size_ = 0;
  this->data_bits_ = 0;
  this->data_size_ = 0;
  this->data_ = NULL;

}

void SampledByIndexISA::Sample(DataInputStream<uint64_t>& sa_stream, uint64_t n) {

  data_bits_ = SuccinctUtils::IntegerLog2(n + 1);
  data_size_ = (n / sampling_rate_) + 1;

  data_ = new bitmap_t;
  SuccinctBase::InitBitmap(&data_, data_size_ * data_bits_,
                           succinct_allocator_);

  for (uint64_t i = 0; i < n; i++) {
    uint64_t sa_val = sa_stream.Get();
    if (sa_val % sampling_rate_ == 0) {
      SuccinctBase::SetBitmapArray(&data_, (sa_val / sampling_rate_), i,
                                   data_bits_);
    }
  }
}

uint64_t SampledByIndexISA::operator [](uint64_t i) {

  assert(i < original_size_);
  uint64_t sample_idx = i / sampling_rate_;
  uint64_t sample = SuccinctBase::LookupBitmapArray(data_, sample_idx,
                                                    data_bits_);
  uint64_t pos = sample;
  i -= (sample_idx * sampling_rate_);
  while (i--) {
    pos = (*npa_)[pos];
  }
  return pos;

}
