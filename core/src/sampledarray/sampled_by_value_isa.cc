#include "sampledarray/sampled_by_value_isa.h"

SampledByValueISA::SampledByValueISA(uint32_t sampling_rate, NPA *npa,
                                     DataInputStream<uint64_t>& sa_stream, uint64_t sa_n,
                                     Dictionary *d_bpos,
                                     SuccinctAllocator &s_allocator)
    : FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_VALUE, npa,
                       s_allocator) {

  assert(ISPOWOF2(sampling_rate));

  this->sampled_positions_ = d_bpos;
  this->original_size_ = sa_n;
  Sample(sa_stream, sa_n);
}

SampledByValueISA::SampledByValueISA(uint32_t sampling_rate, NPA *npa,
                                     SuccinctAllocator &s_allocator)
    : FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_VALUE, npa,
                       s_allocator) {

  assert(ISPOWOF2(sampling_rate));

  this->sampled_positions_ = NULL;
  this->original_size_ = 0;
  this->data_size_ = 0;
  this->data_bits_ = 0;
  this->data_ = NULL;

}

void SampledByValueISA::Sample(DataInputStream<uint64_t>& sa_stream, uint64_t n) {
  data_size_ = (n / sampling_rate_) + 1;
  data_bits_ = SuccinctUtils::IntegerLog2(data_size_ + 1);
  uint64_t sa_val, pos = 0;
  uint32_t orig_bits = SuccinctUtils::IntegerLog2(n + 1);

  data_ = new bitmap_t;
  SuccinctBase::InitBitmap(&data_, data_size_ * data_bits_,
                           succinct_allocator_);

  for (uint64_t i = 0; i < n; i++) {
    sa_val = sa_stream.Get();
    if (sa_val % sampling_rate_ == 0) {
      SuccinctBase::SetBitmapArray(&data_, sa_val / sampling_rate_, pos++,
                                   data_bits_);
    }
  }

}

uint64_t SampledByValueISA::operator [](uint64_t i) {
  uint64_t a, pos;
  uint64_t v = i % sampling_rate_;

  a = SuccinctBase::LookupBitmapArray(data_, i / sampling_rate_, data_bits_);
  pos = SuccinctBase::GetSelect1(sampled_positions_, a);

  while (v) {
    pos = (*npa_)[pos];
    v--;
  }

  return pos;
}

bool SampledByValueISA::IsSampled(uint64_t i) {
  return i % sampling_rate_ == 0;
}

void SampledByValueISA::SetSampledPositions(
    SampledByValueISA::Dictionary *d_bpos) {
  this->sampled_positions_ = d_bpos;
}
