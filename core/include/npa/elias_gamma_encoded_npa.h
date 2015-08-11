#ifndef ELIAS_GAMMA_ENCODED_NPA_H
#define ELIAS_GAMMA_ENCODED_NPA_H

#include "delta_encoded_npa.h"

#define USE_BSR             0
#define USE_PREFIXSUM_TABLE 1

/* Gamma prefix sum table functions */
#define PREFIX_OFF(i)   ((prefixsum_[(i)] >> 24) & 0xFF)
#define PREFIX_CNT(i)   ((prefixsum_[i] >> 16) & 0xFF)
#define PREFIX_SUM(i)   (prefixsum_[i] & 0xFFFF)

class EliasGammaEncodedNPA : public DeltaEncodedNPA {
 public:
  EliasGammaEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
                       uint32_t context_len, uint32_t sampling_rate,
                       Bitmap *data_bitmap, Bitmap *compactSA,
                       Bitmap *compactISA, SuccinctAllocator &s_allocator);

  EliasGammaEncodedNPA(uint32_t context_len, uint32_t sampling_rate,
                       SuccinctAllocator &s_allocator);

  // Virtual destructor
  ~EliasGammaEncodedNPA() {
  }

  // Compute the elias gamma encoding size in bits for a 64 bit integer
  static uint32_t EliasGammaEncodingSize(uint64_t n);

  virtual int64_t BinarySearch(uint64_t val, uint64_t s, uint64_t e, bool flag);

 protected:
  // Create elias-gamma delta encoded vector
  virtual void CreateDeltaEncodedVector(DeltaEncodedVector *dv,
                                        std::vector<uint64_t> &data);

  // Lookup elias-gamma delta encoded vector at index i
  virtual uint64_t LookupDeltaEncodedVector(DeltaEncodedVector *dv, uint64_t i);

 private:
  // Accesses data from a 64 bit integer represented as a bit map
  // from a specified position and for a specified number of bits
  uint16_t AccessDataPos16(uint16_t data, uint32_t pos, uint32_t b);

  int64_t BinarySearchSamples(DeltaEncodedVector *dv, uint64_t val, uint64_t s,
                              uint64_t e);

  // Initialize pre-computed prefix sums
  void InitPrefixSum();

  // Encode a sorted vector using elias-gamma encoding
  void EliasGammaEncode(Bitmap **B, std::vector<uint64_t> &deltas,
                        uint64_t size);

  // Decode a particular elias-gamma encoded delta value at a provided offset
  // in the deltas bitmap
  uint64_t EliasGammaDecode(Bitmap *B, uint64_t *offset);

  // Compute the prefix sum of elias-gamma encoded delta values
  uint64_t EliasGammaPrefixSum(Bitmap *B, uint64_t offset, uint64_t i);

  // Pre-computed prefix sums for elias gamma encoded NPA
  uint32_t prefixsum_[65536];

};

inline uint16_t EliasGammaEncodedNPA::AccessDataPos16(uint16_t data,
                                                      uint32_t pos,
                                                      uint32_t b) {
  assert(b <= 16 && pos >= 0);
  if (b == 0)
    return 0;
  uint16_t val = data << pos;
  return val >> (16 - b);

}

// Compute the elias gamma encoding size in bits for a 64 bit integer
inline uint32_t EliasGammaEncodedNPA::EliasGammaEncodingSize(uint64_t n) {
  uint32_t N = LowerLog2(n);
  return 2 * N + 1;
}

// Decode a particular elias-gamma encoded delta value at a provided offset
// in the deltas bitmap
inline uint64_t EliasGammaEncodedNPA::EliasGammaDecode(Bitmap *B,
                                                       uint64_t *offset) {
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

// Compute the prefix sum of the elias-gamma encoded deltas
inline uint64_t EliasGammaEncodedNPA::EliasGammaPrefixSum(Bitmap *B,
                                                          uint64_t offset,
                                                          uint64_t i) {
  uint64_t delta_sum = 0;

#if USE_PREFIXSUM_TABLE
  uint64_t delta_idx = 0;
  uint64_t delta_off = offset;
  while (delta_idx != i) {
    uint16_t block = (uint16_t) SuccinctBase::LookupBitmapAtPos(B, delta_off,
                                                                16);
    uint16_t cnt = PREFIX_CNT(block);
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
      delta_sum += PREFIX_SUM(block);
      delta_off += PREFIX_OFF(block);
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

inline int64_t EliasGammaEncodedNPA::BinarySearchSamples(DeltaEncodedVector *dv,
                                                         uint64_t val,
                                                         uint64_t s,
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

inline int64_t EliasGammaEncodedNPA::BinarySearch(uint64_t val, uint64_t s,
                                                  uint64_t e, bool flag) {

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
    // cout << "delta_sum = " << delta_sum << endl;
    uint16_t block = (uint16_t) SuccinctBase::LookupBitmapAtPos(B, delta_off,
                                                                16);
    uint16_t block_cnt = PREFIX_CNT(block);
    uint16_t block_sum = PREFIX_SUM(block);
    uint16_t block_off = PREFIX_OFF(block);

    // cout << "block_cnt = " << block_cnt << " block_sum = " << block_sum << " block_off = " << block_off << endl;

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

#endif
