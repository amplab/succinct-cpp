#include "../../include/sampledarray/sampled_by_value_sa.h"

SampledByValueSA::SampledByValueSA(uint32_t sampling_rate, NPA *npa,
                                   bitmap_t *SA, uint64_t sa_n,
                                   SuccinctAllocator &s_allocator,
                                   SuccinctBase *s_base)
    : FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_VALUE, npa,
                       s_allocator) {

  assert(ISPOWOF2(sampling_rate));

  this->succinct_base_ = s_base;
  this->sampled_positions_ = NULL;
  this->original_size_ = sa_n;
  Sample(SA, sa_n);
}

SampledByValueSA::SampledByValueSA(uint32_t sampling_rate, NPA *npa,
                                   SuccinctAllocator &s_allocator,
                                   SuccinctBase *s_base)
    : FlatSampledArray(sampling_rate, SamplingScheme::FLAT_SAMPLE_BY_VALUE, npa,
                       s_allocator) {

  assert(ISPOWOF2(sampling_rate));

  this->succinct_base_ = s_base;
  this->sampled_positions_ = NULL;
  this->original_size_ = 0;
  this->data_size_ = 0;
  this->data_bits_ = 0;
  this->data_ = NULL;

}

void SampledByValueSA::Sample(bitmap_t *SA, uint64_t n) {
  data_size_ = (n / sampling_rate_) + 1;
  data_bits_ = SuccinctUtils::IntegerLog2(data_size_ + 1);
  uint64_t sa_val, pos = 0;
  uint32_t orig_bits = SuccinctUtils::IntegerLog2(n + 1);

  bitmap_t *BPos = new bitmap_t;
  data_ = new bitmap_t;
  SuccinctBase::init_bitmap(&data_, data_size_ * data_bits_, succinct_allocator_);

  SuccinctBase::init_bitmap(&BPos, n, succinct_allocator_);

  for (uint64_t i = 0; i < n; i++) {
    sa_val = SuccinctBase::lookup_bitmap_array(SA, i, orig_bits);
    if (sa_val % sampling_rate_ == 0) {
      SuccinctBase::set_bitmap_array(&data_, pos++, sa_val / sampling_rate_,
                                     data_bits_);
      SETBITVAL(BPos, i);
    }
  }

  sampled_positions_ = new Dictionary;
  succinct_base_->create_dictionary(BPos, sampled_positions_);
  succinct_base_->destroy_bitmap(&BPos, succinct_allocator_);
}

bool SampledByValueSA::IsSampled(uint64_t i) {
  return
      (i == 0) ?
          succinct_base_->get_rank1(sampled_positions_, i) :
          succinct_base_->get_rank1(sampled_positions_, i) - succinct_base_->get_rank1(sampled_positions_, i - 1);
}

uint64_t SampledByValueSA::operator [](uint64_t i) {
  uint64_t v = 0, r, a;
  while (!IsSampled(i)) {
    i = (*npa_)[i];
    v++;
  }

  r = SuccinctUtils::Modulo((int64_t) succinct_base_->get_rank1(sampled_positions_, i) - 1,
                            data_size_);
  a = SuccinctBase::lookup_bitmap_array(data_, r, data_bits_);
  return SuccinctUtils::Modulo((sampling_rate_ * a) - v, original_size_);
}

SampledByValueSA::Dictionary* SampledByValueSA::GetSampledPositions() {
  return sampled_positions_;
}

void SampledByValueSA::SetSampledPositions(SampledByValueSA::Dictionary *d_bpos) {
  this->sampled_positions_ = d_bpos;
}

