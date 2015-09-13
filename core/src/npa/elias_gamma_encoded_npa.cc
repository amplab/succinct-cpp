#include "npa/elias_gamma_encoded_npa.h"

EliasGammaEncodedNPA::EliasGammaEncodedNPA(uint64_t npa_size,
                                           uint64_t sigma_size,
                                           uint32_t context_len,
                                           uint32_t sampling_rate,
                                           std::string& isa_file,
                                           std::vector<uint64_t>& col_offsets,
                                           std::string npa_file,
                                           SuccinctAllocator &s_allocator)
    : DeltaEncodedNPA(npa_size, sigma_size, context_len, sampling_rate,
                      NPAEncodingScheme::ELIAS_GAMMA_ENCODED, s_allocator) {
  InitPrefixSum();
  Encode(isa_file, col_offsets, npa_file);
}

EliasGammaEncodedNPA::EliasGammaEncodedNPA(uint32_t context_len,
                                           uint32_t sampling_rate,
                                           SuccinctAllocator &s_allocator)
    : DeltaEncodedNPA(0, 0, context_len, sampling_rate,
                      NPAEncodingScheme::ELIAS_GAMMA_ENCODED, s_allocator) {
  InitPrefixSum();
}

uint16_t EliasGammaEncodedNPA::AccessDataPos16(uint16_t data, uint32_t pos,
                                               uint32_t b) {
  assert(b <= 16 && pos >= 0);
  if (b == 0)
    return 0;
  uint16_t val = data << pos;
  return val >> (16 - b);

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

// Compute the elias gamma encoding size in bits for a 64 bit integer
uint32_t EliasGammaEncodedNPA::EliasGammaEncodingSize(uint64_t n) {
  uint32_t N = LowerLog2(n);
  return 2 * N + 1;
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

// Decode a particular elias-gamma encoded delta value at a provided offset
// in the deltas bitmap
uint64_t EliasGammaEncodedNPA::EliasGammaDecode(Bitmap *B, uint64_t *offset) {
  uint32_t N = 0;
  while (!ACCESSBIT(B, (*offset))) {
    N++;
    (*offset)++;
  }
  long val = SuccinctBase::LookupBitmapAtPos(B, *offset, N + 1);
  *offset += (N + 1);
  return val;
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

int64_t EliasGammaEncodedNPA::BinarySearchSamples(DeltaEncodedVector *dv,
                                                  uint64_t val, uint64_t s,
                                                  uint64_t e) {
  int64_t sp = s;
  int64_t ep = e;
  uint64_t m, dv_val;

  while (sp <= ep) {
    m = (sp + ep) / 2;

    dv_val = SuccinctBase::LookupBitmapArray(dv->samples, m, dv->sample_bits);
    if (dv_val == val) {
      sp = ep = m;
      break;
    } else if (val < dv_val)
      ep = m - 1;
    else
      sp = m + 1;
  }
  ep = SuccinctUtils::Max(ep, 0);

  return ep;
}

// Compute the prefix sum of the elias-gamma encoded deltas
uint64_t EliasGammaEncodedNPA::EliasGammaPrefixSum(Bitmap *delta_values,
                                                   uint64_t offset,
                                                   uint64_t i) {
  uint64_t delta_sum = 0;

  uint64_t delta_idx = 0;
  uint64_t delta_off = offset;
  while (delta_idx != i) {
    uint16_t block = (uint16_t) SuccinctBase::LookupBitmapAtPos(delta_values,
                                                                delta_off, 16);
    uint16_t cnt = PREFIX_CNT(block);
    if (cnt == 0) {
      // If the prefixsum table for the block returns count == 0
      // this must mean the value spans more than 16 bits
      // read this manually
      uint32_t N = 0;
      while (!ACCESSBIT(delta_values, delta_off)) {
        N++;
        delta_off++;
      }
      delta_sum += SuccinctBase::LookupBitmapAtPos(delta_values, delta_off,
                                                   N + 1);
      delta_off += (N + 1);
      delta_idx += 1;
    } else if (delta_idx + cnt <= i) {
      // If sum can be computed from the prefixsum table
      delta_sum += PREFIX_SUM(block);
      delta_off += PREFIX_OFF(block);
      delta_idx += cnt;
    } else {
      // Last few values, decode them without looking up table
      while (delta_idx != i) {
        uint32_t N = 0;
        while (!ACCESSBIT(delta_values, delta_off)) {
          N++;
          delta_off++;
        }
        delta_sum += SuccinctBase::LookupBitmapAtPos(delta_values, delta_off,
                                                     N + 1);
        delta_off += (N + 1);
        delta_idx += 1;
      }
    }
  }
  return delta_sum;
}

int64_t EliasGammaEncodedNPA::BinarySearch(int64_t val, uint64_t start_idx,
                                           uint64_t end_idx, bool flag) {

  if (end_idx < start_idx)
    return end_idx;

  // Get column-id
  uint64_t col_id = SuccinctBase::GetRank1(&col_offsets_, start_idx) - 1;

  // Adjust start and end indexes for binary search
  start_idx -= col_offsets_[col_id];
  end_idx -= col_offsets_[col_id];

  // Fetch relevant delta encoded vector
  DeltaEncodedVector *dv = &del_npa_[col_id];

  // Binary search within samples to get the nearest smaller sample
  uint64_t sample_offset = BinarySearchSamples(dv, val,
                                               start_idx / sampling_rate_,
                                               end_idx / sampling_rate_);

  // Set the limit on the number of delta values to decode
  uint64_t delta_limit = SuccinctUtils::Min(
      end_idx - (sample_offset * sampling_rate_), sampling_rate_);

  // Get offset into delta bitmap where decoding should start
  uint64_t delta_off = SuccinctBase::LookupBitmapArray(dv->delta_offsets,
                                                       sample_offset,
                                                       dv->delta_offset_bits);
  // Adjust the value being searched for
  val -= SuccinctBase::LookupBitmapArray(dv->samples, sample_offset,
                                         dv->sample_bits);
  // Initialize the delta index and sum accumulated
  int64_t delta_idx = 0, delta_sum = 0;

  // Fetch delta values
  Bitmap *delta_values = dv->deltas;

  // Keep decoding delta values until either:
  // (a) the accumulated sum exceeds the value itself, or,
  // (b) the delta index exceeds the limit on number of delta values to decode
  while (delta_sum < val && delta_idx < delta_limit) {
    uint16_t block = (uint16_t) SuccinctBase::LookupBitmapAtPos(delta_values,
                                                                delta_off, 16);
    uint16_t block_cnt = PREFIX_CNT(block);
    uint16_t block_sum = PREFIX_SUM(block);

    if (block_cnt == 0) {
      // If the prefixsum table for the block returns count == 0
      // this must mean the value spans more than 16 bits
      // read this manually
      int N = 0;
      while (!ACCESSBIT(delta_values, delta_off)) {
        N++;
        delta_off++;
      }
      uint64_t decoded_value = SuccinctBase::LookupBitmapAtPos(delta_values,
                                                           delta_off, N + 1);
      delta_sum += decoded_value;
      delta_off += (N + 1);
      delta_idx += 1;
      // Roll back
      if (delta_idx == sampling_rate_) {
        delta_idx--;
        delta_sum -= decoded_value;
        break;
      }
    } else if (delta_sum + block_sum < val
        && delta_idx + block_cnt < delta_limit) {
      // If sum can be computed from the prefixsum table
      delta_sum += block_sum;
      delta_off += PREFIX_OFF(block);
      delta_idx += block_cnt;
    } else {
      // Last few values, decode them without looking up table
      uint64_t last_decoded_value = 0;
      while (delta_sum < val && delta_idx < delta_limit) {
        int N = 0;
        while (!ACCESSBIT(delta_values, delta_off)) {
          N++;
          delta_off++;
        }
        last_decoded_value = SuccinctBase::LookupBitmapAtPos(delta_values,
                                                             delta_off, N + 1);
        delta_sum += last_decoded_value;
        delta_off += (N + 1);
        delta_idx += 1;
      }

      // Roll back
      if (delta_idx == sampling_rate_) {
        delta_idx--;
        delta_sum -= last_decoded_value;
        break;
      }
    }
  }

  // Obtain the required index for the binary search
  uint64_t res = col_offsets_[col_id] + sample_offset * sampling_rate_
      + delta_idx;

  // If it is an exact match, return the value
  if (val == delta_sum)
    return res;

  // Adjust the index based on whether we wanted lower bound or upper bound
  if (flag) {
    return (delta_sum < val) ? res : res - 1;
  } else {
    return (delta_sum > val) ? res : res + 1;
  }
}
