#include "npa/elias_gamma_encoded_npa.h"

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

uint16_t EliasGammaEncodedNPA::AccessDataPos16(uint16_t data, uint32_t pos,
                                               uint32_t b) {
  assert(b <= 16 && pos >= 0);
  if (b == 0)
    return 0;
  uint16_t val = data << pos;
  return val >> (16 - b);

}

uint8_t EliasGammaEncodedNPA::AccessDataPos8(uint8_t data, uint32_t pos,
                                              uint32_t b) {
  assert(b <= 8 && pos >= 0);
  assert(pos + b <= 8);
  if (b == 0)
    return 0;
  uint8_t val = data << pos;
  return val >> (8 - b);

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
    prefixsum16_[i] = (offset << 24) | (count << 16) | sum;
  }

  uint64_t idx8 = 0;
  for (uint64_t max = 1; max <= 8; max++) {
    for (uint64_t i = 0; i < 256; i++) {
      uint8_t val = (uint8_t) i;
      // for(uint64_t ii = 0; ii < 8; ii++) {
      //  fprintf(stderr, "%lu", GETBIT8(val, ii));
      // }
      uint8_t count = 0, offset = 0, sum = 0;
      while (val && offset <= 8) {
        int N = 0;
        while (!GETBIT8(val, offset)) {
          N++;
          offset++;
        }
        if (offset + (N + 1) <= 8 && count < max) {
          sum += AccessDataPos8(val, offset, N + 1);
          // fprintf(stderr, " acc = %llu", AccessDataPos8(val, offset, N + 1));
          offset += (N + 1);
          count++;
        } else {
          offset -= N;
          break;
        }
      }

      // fprintf(stderr, " max = %llu, offset = %u, count = %u, sum = %u\n", max, offset, count, sum);
      prefixsum8_[(max << 8) | i] = (offset << 12) | (count << 8) | sum;
    }
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
#if USE_BSR
  uint64_t data = B->bitmap[(*offset) / 64] << ((*offset) % 64);
  N = __builtin_clz(data);
  (*offset) += N;
#else
  while (!ACCESSBIT(B, (*offset))) {
    N++;
    (*offset)++;
  }
#endif
  long val = SuccinctBase::LookupBitmapAtPos(B, *offset, N + 1);
  *offset += (N + 1);
  return val;
}

uint64_t EliasGammaEncodedNPA::EliasGammaPrefixSum2(Bitmap *B, uint64_t offset,
                                                    uint64_t i) {
  uint64_t delta_sum = 0;
  uint64_t delta_idx = 0;
  uint64_t delta_off = offset;
  uint64_t *data = B->bitmap;

  uint64_t delta_off64 = (delta_off & 0x3F);
  uint64_t delta_idx64 = (delta_off >> 6);
  uint64_t block64_cur = data[delta_idx64];

#ifndef UPDATE_BLOCK64
#define UPDATE_BLOCK64  if(delta_off64 >= 64) {\
  delta_off64 = (delta_off & 0x3F);\
  delta_idx64 = (delta_off >> 6);\
  block64_cur = data[delta_idx64];\
}
#endif

  while (delta_idx != i) {
    uint16_t block16 =
        (delta_off64 > 48) ?
            ((block64_cur << (delta_off64 - 48)) & 0xFFFF)
                | (data[delta_idx64 + 1] >> (112 - delta_off64)) :
            (block64_cur >> (48 - delta_off64)) & 0xFFFF;

    uint16_t cnt = PREFIX_CNT16(block16);
    if (cnt == 0) {
      // If the prefixsum table for the block returns count == 0
      // this must mean the value spans more than 16 bits
      // read this manually
      uint32_t N = 0;
      while (!GETBIT(block64_cur, delta_off64)) {
        N++;
        delta_off++;
        delta_off64++;
        UPDATE_BLOCK64;
      }
      N++;

      if (delta_off64 + N > 64) {
        delta_sum += (((block64_cur << delta_off64)
            >> (delta_off64 - ((delta_off64 + N - 1) % 64 + 1)))
            | (data[delta_idx64 + 1] >> (63 - ((delta_off64 + N - 1) % 64))));
      } else {
        delta_sum += ((block64_cur << delta_off64) >> (64 - N));
      }

      delta_off64 += N;
      delta_off += N;
      UPDATE_BLOCK64;
      delta_idx += 1;
    } else if (delta_idx + cnt <= i) {
      // If sum can be computed from the prefixsum table
      delta_sum += PREFIX_SUM16(block16);
      delta_off += PREFIX_OFF16(block16);
      delta_off64 += PREFIX_OFF16(block16);
      UPDATE_BLOCK64;
      delta_idx += cnt;
    } else {
      // We decoded too many values from the last 16-bit block
      if(i - delta_idx <= 8) {
        // Decode 8-bits at a time
        uint16_t block8 = (block16 >> 8) | ((i - delta_idx) << 8);
        cnt = PREFIX_CNT8(block8);
        if(cnt > 0) {
          delta_sum += PREFIX_SUM8(block8);
          delta_idx += cnt;
          delta_off += PREFIX_OFF8(block8);
          delta_off64 += PREFIX_OFF8(block8);
          UPDATE_BLOCK64;
        }
      }
      while (delta_idx != i) {
        uint32_t N = 0;
        while (!GETBIT(block64_cur, delta_off64)) {
          N++;
          delta_off++;
          delta_off64++;
          UPDATE_BLOCK64;
        }
        N++;

        if (delta_off64 + N > 64) {
          delta_sum += (((block64_cur << delta_off64)
              >> (delta_off64 - ((delta_off64 + N - 1) % 64 + 1)))
              | (data[delta_idx64 + 1] >> (63 - ((delta_off64 + N - 1) % 64))));
        } else {
          delta_sum += ((block64_cur << delta_off64) >> (64 - N));
        }

        delta_off64 += N;
        delta_off += N;
        UPDATE_BLOCK64;
        delta_idx += 1;
      }
    }
  }

#undef UPDATE_BLOCK64

  return delta_sum;
}

// Compute the prefix sum of the elias-gamma encoded deltas
uint64_t EliasGammaEncodedNPA::EliasGammaPrefixSum(Bitmap *B, uint64_t offset,
                                                   uint64_t i) {
  uint64_t delta_sum = 0;

#if USE_PREFIXSUM_TABLE
  uint64_t delta_idx = 0;
  uint64_t delta_off = offset;
  while (delta_idx != i) {
    uint16_t block = (uint16_t) SuccinctBase::LookupBitmapAtPos(B, delta_off,
                                                                16);
    uint16_t cnt = PREFIX_CNT16(block);
    if (cnt == 0) {
      // If the prefixsum table for the block returns count == 0
      // this must mean the value spans more than 16 bits
      // read this manually
      uint32_t N = 0;
#if USE_BSR
      uint64_t data = LookupBitmapAtPos(B, delta_off, 64);
      N = __builtin_clzl(data);
      delta_off += N;
#else
      while (!ACCESSBIT(B, delta_off)) {
        N++;
        delta_off++;
      }
#endif
      delta_sum += SuccinctBase::LookupBitmapAtPos(B, delta_off, N + 1);
      delta_off += (N + 1);
      delta_idx += 1;
    } else if (delta_idx + cnt <= i) {
      // If sum can be computed from the prefixsum table
      delta_sum += PREFIX_SUM16(block);
      delta_off += PREFIX_OFF16(block);
      delta_idx += cnt;
    } else {
      // Last few values, decode them without looking up table
      while (delta_idx != i) {
        uint32_t N = 0;
#if USE_BSR
        uint64_t data = LookupBitmapAtPos(B, delta_off, 64);
        N = __builtin_clzl(data);
        delta_off += N;
#else
        while (!ACCESSBIT(B, delta_off)) {
          N++;
          delta_off++;
        }
#endif
        delta_sum += SuccinctBase::LookupBitmapAtPos(B, delta_off, N + 1);
        delta_off += (N + 1);
        delta_idx += 1;
      }
    }
  }
#else
  for(uint64_t j = 0; j < i; j++) {
    uint32_t N = 0;
#if USE_BSR
    uint64_t data = LookupBitmapAtPos(B, offset, 64);
    N = __builtin_clzl(data);
    offset += N;
#else
    while(!ACCESSBIT(B, offset)) {
      N++;
      offset++;
    }
#endif
    delta_sum += LookupBitmapAtPos(B, offset, N + 1);
    offset += (N + 1);
  }
#endif
  return delta_sum;
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
  // assert(
  //    EliasGammaPrefixSum(dv->deltas, delta_offset, delta_offset_idx)
  //        == EliasGammaPrefixSum2(dv->deltas, delta_offset, delta_offset_idx));
  val += EliasGammaPrefixSum2(dv->deltas, delta_offset, delta_offset_idx);
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

int64_t EliasGammaEncodedNPA::BinarySearch(uint64_t val, uint64_t s, uint64_t e,
                                           bool flag) {

  uint64_t col_id = SuccinctBase::GetRank1(&col_offsets_, s) - 1;
  s -= col_offsets_[col_id];
  e -= col_offsets_[col_id];
  DeltaEncodedVector *dv = &del_npa_[col_id];
  uint64_t sample_offset = BinarySearchSamples(dv, val, s / sampling_rate_,
                                               e / sampling_rate_);
  uint64_t delta_off = SuccinctBase::LookupBitmapArray(dv->delta_offsets,
                                                       sample_offset,
                                                       dv->delta_offset_bits);
  uint64_t delta_sum = SuccinctBase::LookupBitmapArray(dv->samples,
                                                       sample_offset,
                                                       dv->sample_bits);
  uint64_t delta_idx = 0;

  Bitmap *B = dv->deltas;

#if USE_PREFIXSUM_TABLE
  while (delta_sum < val && delta_off < B->size) {
    uint16_t block = (uint16_t) SuccinctBase::LookupBitmapAtPos(B, delta_off,
                                                                16);
    uint16_t block_cnt = PREFIX_CNT16(block);
    uint16_t block_sum = PREFIX_SUM16(block);
    uint16_t block_off = PREFIX_OFF16(block);

    if (block_cnt == 0) {
      // If the prefixsum table for the block returns count == 0
      // this must mean the value spans more than 16 bits
      // read this manually
      int N = 0;
#if USE_BSR
      uint64_t data = accessBMArrayPos(B, delta_off, 64);
      N = __builtin_clzl(data);
      delta_off += N;
#else
      while (!ACCESSBIT(B, delta_off)) {
        N++;
        delta_off++;
      }
#endif
      delta_sum += SuccinctBase::LookupBitmapAtPos(B, delta_off, N + 1);
      delta_off += (N + 1);
      delta_idx += 1;
    } else if (delta_sum + block_sum < val) {
      // If sum can be computed from the prefixsum table
      delta_sum += block_sum;
      delta_off += block_off;
      delta_idx += block_cnt;
    } else {
      // Last few values, decode them without looking up table
      while (delta_sum < val && delta_off < B->size) {
        int N = 0;
#if USE_BSR
        uint64_t data = SuccinctBase::lookup_bitmap_pos(B, delta_off, 64);
        N = __builtin_clzl(data);
        delta_off += N;
#else
        while (!ACCESSBIT(B, delta_off)) {
          N++;
          delta_off++;
        }
#endif
        delta_sum += SuccinctBase::LookupBitmapAtPos(B, delta_off, N + 1);
        delta_off += (N + 1);
        delta_idx += 1;
      }
    }
  }
#else
  while(delta_sum < val && delta_off < B->size) {
    int N = 0;
#if USE_BSR
    uint64_t data = SuccinctBase::lookup_bitmap_pos(B, delta_off, 64);
    N = __builtin_clzl(data);
    delta_off += N;
#else
    while(!ACCESSBIT(B, delta_off)) {
      N++;
      delta_off++;
    }
#endif
    delta_sum += SuccinctBase::lookup_bitmap_pos(B, delta_off, N + 1);
    delta_off += (N + 1);
    delta_idx++;
  }
#endif

  uint64_t res = col_offsets_[col_id] + sample_offset * sampling_rate_
      + delta_idx;
  if (val == delta_sum || delta_off == B->size)
    return res;

  return flag ? res - 1 : res;
}
