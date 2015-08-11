#include "../../include/npa/elias_gamma_encoded_npa.h"

EliasGammaEncodedNPA::EliasGammaEncodedNPA(uint64_t npa_size,
                                           uint64_t sigma_size,
                                           uint32_t context_len,
                                           uint32_t sampling_rate,
                                           Bitmap *data_bitmap,
                                           Bitmap *compactSA,
                                           Bitmap *compactISA,
                                           SuccinctAllocator &s_allocator)
    : DeltaEncodedNPA(npa_size, sigma_size, context_len, sampling_rate,
                      NPAEncodingScheme::ELIAS_GAMMA_ENCODED, s_allocator) {
  InitPrefixSum();
  Encode(data_bitmap, compactSA, compactISA);
}

EliasGammaEncodedNPA::EliasGammaEncodedNPA(uint32_t context_len,
                                           uint32_t sampling_rate,
                                           SuccinctAllocator &s_allocator)
    : DeltaEncodedNPA(0, 0, context_len, sampling_rate,
                      NPAEncodingScheme::ELIAS_GAMMA_ENCODED, s_allocator) {
  InitPrefixSum();
}

void EliasGammaEncodedNPA::InitPrefixSum() {
  for (uint64_t i = 0; i < 65536; i++) {
    uint16_t val = (uint16_t) i;
    uint16_t count = 0, offset = 0, sum = 0;
    while (val && offset <= 16) {
      int N = 0;
      while (!GETBIT16(val, offset)) {
        N++;
        offset++;
      }
      if (offset + (N + 1) <= 16) {
        sum += AccessDataPos16(val, offset, N + 1);
        offset += (N + 1);
        count++;
      } else {
        offset -= N;
        break;
      }
    }
    prefixsum_[i] = (offset << 24) | (count << 16) | sum;
  }
}

// Encode a sorted vector using elias gamma encoding
void EliasGammaEncodedNPA::EliasGammaEncode(Bitmap **B,
                                            std::vector<uint64_t> &deltas,
                                            uint64_t size) {
  if (size == 0) {
    delete *B;
    *B = NULL;
    return;
  }
  uint64_t pos = 0;
  SuccinctBase::InitBitmap(B, size, s_allocator_);
  for (size_t i = 0; i < deltas.size(); i++) {
    int bits = EliasGammaEncodingSize(deltas[i]);
    SuccinctBase::SetBitmapAtPos(B, pos, deltas[i], bits);
    pos += bits;
  }
}

void EliasGammaEncodedNPA::CreateDeltaEncodedVector(
    DeltaEncodedVector *dv, std::vector<uint64_t> &data) {
  if (data.size() == 0) {
    return;
  }
  assert(dv != NULL);

  uint64_t max_sample = 0;
  std::vector<uint64_t> _samples, _deltas, _delta_offsets;
  uint64_t tot_delta_count = 0, delta_count = 0;
  uint64_t delta_enc_size = 0;
  uint64_t cum_delta_size = 0, max_offset = 0;
  uint64_t last_val = 0;

  for (size_t i = 0; i < data.size(); i++) {
    if (i % sampling_rate_ == 0) {
      _samples.push_back(data[i]);
      if (data[i] > max_sample) {
        max_sample = data[i];
      }
      if (cum_delta_size > max_offset)
        max_offset = cum_delta_size;
      _delta_offsets.push_back(cum_delta_size);
      if (i != 0) {
        assert(delta_count == sampling_rate_ - 1);
        tot_delta_count += delta_count;
        delta_count = 0;
      }
    } else {
      long delta = data[i] - last_val;
      assert(delta > 0);
      _deltas.push_back(delta);

      delta_enc_size = EliasGammaEncodingSize(delta);
      cum_delta_size += delta_enc_size;
      delta_count++;
    }
    last_val = data[i];
  }
  tot_delta_count += delta_count;

  assert(tot_delta_count == _deltas.size());
  assert(_samples.size() + _deltas.size() == data.size());
  assert(_delta_offsets.size() == _samples.size());

  // Can occur at most once per context;
  // only occurs if 0 is the only value in the cell
  if (max_sample == 0)
    dv->sample_bits = 1;
  else
    dv->sample_bits = (uint8_t) SuccinctUtils::IntegerLog2(max_sample + 1);

  if (max_offset == 0)
    dv->delta_offset_bits = 1;
  else
    dv->delta_offset_bits = (uint8_t) SuccinctUtils::IntegerLog2(
        max_offset + 1);

  dv->samples = new Bitmap;
  dv->deltas = new Bitmap;
  SuccinctBase::CreateBitmapArray(&(dv->samples),
                                  (_samples.size() == 0) ?
                                  NULL :
                                                           &_samples[0],
                                  _samples.size(), dv->sample_bits,
                                  s_allocator_);

  EliasGammaEncode(&(dv->deltas), _deltas, cum_delta_size);
  dv->delta_offsets = new Bitmap;
  SuccinctBase::CreateBitmapArray(
      &(dv->delta_offsets),
      (_delta_offsets.size() == 0) ? NULL : &_delta_offsets[0],
      _delta_offsets.size(), dv->delta_offset_bits, s_allocator_);

}

uint64_t EliasGammaEncodedNPA::LookupDeltaEncodedVector(DeltaEncodedVector *dv,
                                                        uint64_t i) {
  uint64_t sample_offset = i / sampling_rate_;
  uint64_t delta_offset_idx = i % sampling_rate_;
  uint64_t val = SuccinctBase::LookupBitmapArray(dv->samples, sample_offset,
                                                 dv->sample_bits);
  if (delta_offset_idx == 0)
    return val;
  uint64_t delta_offset = SuccinctBase::LookupBitmapArray(
      dv->delta_offsets, sample_offset, dv->delta_offset_bits);
  val += EliasGammaPrefixSum(dv->deltas, delta_offset, delta_offset_idx);
  return val;
}

