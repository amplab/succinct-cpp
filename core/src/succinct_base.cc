#include "succinct_base.h"

/* TODO: Replace all new[] calls with SuccinctAllocator calls */

uint16_t *SuccinctBase::decode_table[17];
std::map<uint16_t, uint16_t> SuccinctBase::encode_table[17];
uint16_t SuccinctBase::C16[17];
uint8_t SuccinctBase::offbits[17];
uint8_t SuccinctBase::smallrank[65536][16];

/* Default constructor */
SuccinctBase::SuccinctBase() {
  InitTables();
}

/* Initializer functions */
// Initialize tables for dictionary encoding/decoding, etc.
void SuccinctBase::InitTables() {
  uint16_t q[17];

  C16[0] = 1;
  offbits[0] = 1;
  C16[1] = 16;
  offbits[1] = 4;
  C16[2] = 120;
  offbits[2] = 7;
  C16[3] = 560;
  offbits[3] = 10;
  C16[4] = 1820;
  offbits[4] = 11;
  C16[5] = 4368;
  offbits[5] = 13;
  C16[6] = 8008;
  offbits[6] = 13;
  C16[7] = 11440;
  offbits[7] = 14;
  C16[8] = 12870;
  offbits[8] = 14;

  for (uint32_t i = 0; i <= 16; i++) {
    if (i > 8) {
      C16[i] = C16[16 - i];
      offbits[i] = offbits[16 - i];
    }
    decode_table[i] = new uint16_t[C16[i]];
    q[i] = 0;
  }
  q[16] = 1;
  for (uint64_t i = 0; i <= 65535; i++) {
    uint64_t p = SuccinctUtils::PopCount(i);
    decode_table[p % 16][q[p]] = (uint16_t) i;
    encode_table[p % 16][i] = q[p];
    q[p]++;
    for (uint64_t j = 0; j < 16; j++) {
      smallrank[i][j] = (uint8_t) SuccinctUtils::PopCount(i >> (15 - j));
    }
  }
}

uint64_t SuccinctBase::GetRank1(std::vector<uint64_t> *V, uint64_t i) {
  long sp = 0, ep = V->size() - 1;
  long m;
  uint64_t *data = &(*V)[0];

  while (sp <= ep) {
    m = (sp + ep) / 2;
    uint64_t val = data[m];
    if (val == i)
      return m + 1;
    else if (i < val)
      ep = m - 1;
    else
      sp = m + 1;
  }
  return ep + 1;
}

size_t SuccinctBase::SerializeVector(std::vector<uint64_t> &v,
                                     std::ostream& out) {
  size_t out_size = 0;
  size_t v_size = v.size();
  out.write(reinterpret_cast<const char *>(&(v_size)), sizeof(size_t));
  out_size += sizeof(size_t);

  for (size_t i = 0; i < v.size(); i++) {
    out.write(reinterpret_cast<const char *>(&v[i]), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
  }

  return out_size;

}

size_t SuccinctBase::DeserializeVector(std::vector<uint64_t> &v,
                                       std::istream& in) {
  size_t in_size = 0;
  size_t v_size;
  in.read(reinterpret_cast<char *>(&v_size), sizeof(size_t));
  in_size += sizeof(size_t);
  v.reserve(v_size);
  for (size_t i = 0; i < v_size; i++) {
    uint64_t val;
    in.read(reinterpret_cast<char *>(&val), sizeof(uint64_t));
    v.push_back(val);
    in_size += sizeof(uint64_t);
  }

  return in_size;
}

size_t SuccinctBase::MemoryMapVector(std::vector<uint64_t> &v, uint8_t* buf) {
  uint8_t *data, *data_beg;
  data = data_beg = buf;
  size_t v_size = *((size_t *) data);
  data += sizeof(size_t);
  v.reserve(v_size);
  for (size_t i = 0; i < v_size; i++) {
    uint64_t val = *((uint64_t *) data);
    data += sizeof(uint64_t);
    v.push_back(val);
  }

  return data - data_beg;
}

/* Bitmap access/modifier functions */
// Initialize the bitmap with the specified size in bits
void SuccinctBase::InitBitmap(SuccinctBase::BitMap **B, uint64_t size_in_bits,
                              SuccinctAllocator& s_allocator) {
  assert(size_in_bits != 0);
  (*B)->bitmap = (uint64_t *) s_allocator.s_calloc(sizeof(uint64_t),
                                                   BITS2BLOCKS(size_in_bits));
  (*B)->size = size_in_bits;
}

// Initialize the bitmap with the specified size in bits, with all bits set
void SuccinctBase::InitBitmapSet(SuccinctBase::BitMap **B,
                                 uint64_t size_in_bits,
                                 SuccinctAllocator& s_allocator) {
  assert(size_in_bits != 0);
  (*B)->bitmap = (uint64_t *) s_allocator.s_malloc(
      sizeof(uint64_t) * BITS2BLOCKS(size_in_bits));
  int set_bits = 0xFF;
  s_allocator.s_memset((*B)->bitmap, set_bits,
                       sizeof(uint64_t) * BITS2BLOCKS(size_in_bits));
  (*B)->size = size_in_bits;
}

// Clear the contents of the bitmap and unset all bits
void SuccinctBase::ClearBitmap(BitMap **B, SuccinctAllocator& s_allocator) {
  s_allocator.s_memset((*B)->bitmap, 0,
  BITS2BLOCKS((*B)->size) * sizeof(uint64_t));
}

void SuccinctBase::DestroyBitmap(SuccinctBase::BitMap **B,
                                 SuccinctAllocator& s_allocator) {
  s_allocator.s_free((*B)->bitmap);
  delete *B;
}

// Create a bitmap array from an array with specified bit-width per element
void SuccinctBase::CreateBitmapArray(SuccinctBase::BitMap **B, uint64_t *A,
                                     uint64_t n, uint32_t b,
                                     SuccinctAllocator s_allocator) {
  if (n == 0 || b == 0) {
    assert(b == 0 || A == NULL);
    delete *B;
    *B = NULL;
    return;
  }

  InitBitmap(B, n * b, s_allocator);
  for (uint64_t i = 0; i < n; i++) {
    SetBitmapArray(B, i, A[i], b);
  }
}

// Set a value in the bitmap, treating it as an array of fixed length
void SuccinctBase::SetBitmapArray(SuccinctBase::BitMap **B, uint64_t i,
                                  uint64_t val, uint32_t b) {

  uint64_t s = i * b, e = i * b + (b - 1);
  if ((s / 64) == (e / 64)) {
    (*B)->bitmap[s / 64] |= (val << (63 - e % 64));
  } else {
    (*B)->bitmap[s / 64] |= (val >> (e % 64 + 1));
    (*B)->bitmap[e / 64] |= (val << (63 - e % 64));
  }
}

// Lookup a value in the bitmap, treating it as an array of fixed length
uint64_t SuccinctBase::LookupBitmapArray(SuccinctBase::BitMap *B, uint64_t i,
                                         uint32_t b) {
  if (b == 0)
    return 0;
  uint64_t val;
  uint64_t *bitmap = B->bitmap;
  assert(i >= 0);
  uint64_t s = i * b, e = i * b + (b - 1);
  if ((s / 64) == (e / 64)) {
    val = bitmap[s / 64] << (s % 64);
    val = val >> (63 - e % 64 + s % 64);
  } else {
    uint64_t val1 = bitmap[s / 64] << (s % 64);
    uint64_t val2 = bitmap[e / 64] >> (63 - e % 64);
    val1 = val1 >> (s % 64 - (e % 64 + 1));
    val = val1 | val2;
  }

  return val;
}

// Set a value in the bitmap at a specified offset
void SuccinctBase::SetBitmapAtPos(SuccinctBase::BitMap **B, uint64_t pos,
                                  uint64_t val, uint32_t b) {
  uint64_t s = pos, e = pos + (b - 1);
  if ((s / 64) == (e / 64)) {
    (*B)->bitmap[s / 64] |= (val << (63 - e % 64));
  } else {
    (*B)->bitmap[s / 64] |= (val >> (e % 64 + 1));
    (*B)->bitmap[e / 64] |= (val << (63 - e % 64));
  }
}

// Lookup a value in the bitmap at a specified offset
uint64_t SuccinctBase::LookupBitmapAtPos(SuccinctBase::BitMap *B, uint64_t pos,
                                         uint32_t b) {
  if (b == 0)
    return 0;
  assert(pos >= 0);
  uint64_t val;
  uint64_t s = pos, e = pos + (b - 1);
  if ((s / 64) == (e / 64)) {
    val = B->bitmap[s / 64] << (s % 64);
    val = val >> (63 - e % 64 + s % 64);
  } else {
    uint64_t val1 = B->bitmap[s / 64] << (s % 64);
    uint64_t val2 = B->bitmap[e / 64] >> (63 - e % 64);
    val1 = val1 >> (s % 64 - (e % 64 + 1));
    val = val1 | val2;
  }

  return val;
}

size_t SuccinctBase::SerializeBitmap(SuccinctBase::BitMap *B,
                                     std::ostream& out) {
  size_t out_size = 0;

  if (B == NULL) {
    out.write(reinterpret_cast<const char *>(&out_size), sizeof(uint64_t));
    return out_size;
  }

  out.write(reinterpret_cast<const char *>(&B->size), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  for (uint64_t i = 0; i < BITS2BLOCKS(B->size); i++) {
    out.write(reinterpret_cast<const char *>(&B->bitmap[i]), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
  }

  return out_size;
}

size_t SuccinctBase::DeserializeBitmap(SuccinctBase::BitMap **B,
                                       std::istream& in) {

  size_t in_size = 0;

  uint64_t bitmap_size;
  in.read(reinterpret_cast<char *>(&bitmap_size), sizeof(uint64_t));
  if (bitmap_size) {
    in_size += sizeof(uint64_t);
    (*B) = new BitMap;
    (*B)->size = bitmap_size;
    (*B)->bitmap = new uint64_t[BITS2BLOCKS(bitmap_size)];
    for (uint64_t i = 0; i < BITS2BLOCKS(bitmap_size); i++) {
      in.read(reinterpret_cast<char *>(&(*B)->bitmap[i]), sizeof(uint64_t));
      in_size += sizeof(uint64_t);
    }
  }

  return in_size;
}

size_t SuccinctBase::MemoryMapBitmap(SuccinctBase::BitMap **B, uint8_t* buf) {
  uint8_t *data, *data_beg;
  data = data_beg = buf;

  uint64_t bitmap_size = *((uint64_t *) data);
  data += sizeof(uint64_t);
  if (bitmap_size) {
    (*B) = new BitMap;
    (*B)->size = bitmap_size;
    (*B)->bitmap = (uint64_t*) data;
    uint64_t bitmap_size_bytes = (BITS2BLOCKS(bitmap_size) * sizeof(uint64_t));
    data += bitmap_size_bytes;
  }

  return data - data_beg;
}

/* Dictionary access/modifier functions */
// Create dictionary from a bitmap; returns the size of the dictionary in bits
uint64_t SuccinctBase::CreateDictionary(SuccinctBase::BitMap *B,
                                        SuccinctBase::Dictionary *D,
                                        SuccinctAllocator& s_allocator) {
  uint64_t l3_size = (B->size / kTwo32) + 1;
  uint64_t l2_size = (B->size / 2048) + 1;
  uint64_t l1_size = (B->size / 512) + 1;
  uint64_t count = 0;

  D->size = B->size;
  D->B = new BitMap;
  D->rank_l3 = new uint64_t[l3_size];
  D->pos_l3 = new uint64_t[l3_size];
  uint64_t *rank_l2 = new uint64_t[l2_size];
  uint64_t *rank_l1 = new uint64_t[l1_size];
  uint64_t *pos_l2 = new uint64_t[l2_size];
  uint64_t *pos_l1 = new uint64_t[l1_size];
  D->rank_l12 = new uint64_t[l2_size];
  D->pos_l12 = new uint64_t[l2_size];

  std::vector<uint16_t> dict;
  uint64_t sum_l1 = 0, sum_pos_l1 = 0, i, p, size = 0;
  uint32_t flag = 0;

  D->rank_l3[0] = 0;
  D->pos_l3[0] = 0;
  rank_l2[0] = 0;
  rank_l1[0] = 0;

  for (i = 0; i < B->size; i++) {
    if (i % kTwo32 == 0) {
      D->rank_l3[i / kTwo32] = count;
      D->pos_l3[i / kTwo32] = size;
    }
    if (i % 2048 == 0) {
      rank_l2[i / 2048] = count - D->rank_l3[i / kTwo32];
      pos_l2[i / 2048] = size - D->pos_l3[i / kTwo32];
      D->rank_l12[i / 2048] = rank_l2[i / 2048] << 32;
      D->pos_l12[i / 2048] = pos_l2[i / 2048] << 31;
      flag = 0;
      sum_l1 = 0;
      sum_pos_l1 = 0;
    }
    if (i % 512 == 0) {
      rank_l1[i / 512] = count - rank_l2[i / 2048] - sum_l1;
      pos_l1[i / 512] = size - pos_l2[i / 2048] - sum_pos_l1;
      sum_l1 += rank_l1[i / 512];
      sum_pos_l1 += pos_l1[i / 512];
      if (flag != 0) {
        D->rank_l12[i / 2048] |= (rank_l1[i / 512] << (32 - flag * 10));
        D->pos_l12[i / 2048] |= (pos_l1[i / 512] << (31 - flag * 10));
      }
      flag++;
    }
    if (ACCESSBIT(B, i)) {
      count++;
    }
    if (i % 64 == 0) {
      p = SuccinctUtils::PopCount((B->bitmap[i / 64] >> 48) & 65535);
      p = p % 16;
      size += offbits[p];
      dict.push_back(p);
      dict.push_back(encode_table[p][(B->bitmap[i / 64] >> 48) & 65535]);

      p = SuccinctUtils::PopCount((B->bitmap[i / 64] >> 32) & 65535);
      p = p % 16;
      size += offbits[p];
      dict.push_back(p % 16);
      dict.push_back(encode_table[p][(B->bitmap[i / 64] >> 32) & 65535]);

      p = SuccinctUtils::PopCount((B->bitmap[i / 64] >> 16) & 65535);
      p = p % 16;
      size += offbits[p];
      dict.push_back(p % 16);
      dict.push_back(encode_table[p][(B->bitmap[i / 64] >> 16) & 65535]);

      p = SuccinctUtils::PopCount(B->bitmap[i / 64] & 65535);
      p = p % 16;
      size += offbits[p];
      dict.push_back(p % 16);
      dict.push_back(encode_table[p][B->bitmap[i / 64] & 65535]);

      size += 16;
    }
  }

  InitBitmap(&(D->B), size, s_allocator);
  uint64_t numBits = 0;
  for (size_t i = 0; i < dict.size(); i++) {
    if (i % 2 == 0) {
      SetBitmapAtPos(&D->B, numBits, dict[i], 4);
      numBits += 4;
    } else {
      SetBitmapAtPos(&D->B, numBits, dict[i], offbits[dict[i - 1]]);
      numBits += offbits[dict[i - 1]];
    }
  }

  delete[] rank_l1;
  delete[] rank_l2;
  delete[] pos_l1;
  delete[] pos_l2;

  return ((l3_size * 64 + l2_size * 64) * 2 + D->B->size + 2 * 64);
}

// Get the 1-rank of the dictionary at the specified index
uint64_t SuccinctBase::GetRank1(SuccinctBase::Dictionary *D, uint64_t i) {

  assert(i < D->size);

  uint64_t l3_idx = i / kTwo32;
  uint64_t l2_idx = i / 2048;
  uint16_t l1_idx = i % 512;
  uint16_t rem = (i % 2048) / 512;
  uint16_t block_class, block_offset;

  uint64_t *rank_l12 = D->rank_l12;
  uint64_t *pos_l12 = D->pos_l12;
  BitMap *B = D->B;

  uint64_t res = D->rank_l3[l3_idx] + GETRANKL2(rank_l12[l2_idx]);
  uint64_t pos = D->pos_l3[l3_idx] + GETPOSL2(pos_l12[l2_idx]);

  switch (rem) {
    case 1:
      res += GETRANKL1(rank_l12[l2_idx], 1);
      pos += GETPOSL1(pos_l12[l2_idx], 1);
      break;

    case 2:
      res += GETRANKL1(rank_l12[l2_idx], 1) + GETRANKL1(rank_l12[l2_idx], 2);
      pos += GETPOSL1(pos_l12[l2_idx], 1) + GETPOSL1(pos_l12[l2_idx], 2);
      break;

    case 3:
      res += GETRANKL1(rank_l12[l2_idx], 1) + GETRANKL1(rank_l12[l2_idx], 2)
      + GETRANKL1(rank_l12[l2_idx], 3);
      pos += GETPOSL1(pos_l12[l2_idx], 1) + GETPOSL1(pos_l12[l2_idx], 2)
      + GETPOSL1(pos_l12[l2_idx], 3);
      break;

    default:
      break;
  }

  // Pop-count
  while (l1_idx >= 16) {
    block_class = LookupBitmapAtPos(B, pos, 4);
    pos += 4;
    block_offset = (block_class == 0) ? ACCESSBIT(B, pos) * 16 : 0;
    pos += offbits[block_class];
    res += block_class + block_offset;
    l1_idx -= 16;
  }

  block_class = LookupBitmapAtPos(B, pos, 4);
  pos += 4;
  block_offset = LookupBitmapAtPos(B, pos, offbits[block_class]);
  res += smallrank[decode_table[block_class][block_offset]][l1_idx];

  return res;
}

// Get the 0-rank of the dictionary at the specified index
uint64_t SuccinctBase::GetRank0(SuccinctBase::Dictionary *D, uint64_t i) {
  return i - GetRank1(D, i) + 1;
}

// Get the 1-select of the dictionary at the specified index
uint64_t SuccinctBase::GetSelect1(SuccinctBase::Dictionary *D, uint64_t i) {
  uint64_t val = i + 1;
  int64_t sp = 0;
  int64_t ep = D->size / kTwo32;
  uint64_t m, r;
  uint64_t pos = 0;
  uint64_t block_class, block_offset;
  uint64_t sel = 0;
  uint16_t lastblock;

  BitMap *B = D->B;
  uint64_t size = D->size;
  uint64_t *rank_l3 = D->rank_l3;
  uint64_t *rank_l12 = D->rank_l12;
  uint64_t *pos_l3 = D->pos_l3;
  uint64_t *pos_l12 = D->pos_l12;

  while (sp <= ep) {
    m = (sp + ep) / 2;
    r = rank_l3[m];
    if (val > r)
      sp = m + 1;
    else
      ep = m - 1;
  }

  ep = SuccinctUtils::Max(ep, 0LL);
  sel += ep * kTwo32;
  val -= rank_l3[ep];
  pos += pos_l3[ep];
  sp = ep * kTwo32 / 2048;
  ep = SuccinctUtils::Min(((ep + 1) * kTwo32 / 2048),
                          SuccinctUtils::NumBlocks(size, 2048UL)) - 1;

  while (sp <= ep) {
    m = (sp + ep) / 2;
    r = GETRANKL2(rank_l12[(sp + ep) / 2]);
    if (val > r)
      sp = m + 1;
    else
      ep = m - 1;
  }

  ep = SuccinctUtils::Max(ep, 0LL);
  sel += ep * 2048;
  val -= GETRANKL2(rank_l12[ep]);
  pos += GETPOSL2(pos_l12[ep]);

  assert(val <= 2048);

  r = GETRANKL1(rank_l12[ep], 1);
  if (sel + 512 < size && val > r) {
    pos += GETPOSL1(pos_l12[ep], 1);
    val -= r;
    sel += 512;
    r = GETRANKL1(rank_l12[ep], 2);
    if (sel + 512 < size && val > r) {
      pos += GETPOSL1(pos_l12[ep], 2);
      val -= r;
      sel += 512;
      r = GETRANKL1(rank_l12[ep], 3);
      if (sel + 512 < size && val > r) {
        pos += GETPOSL1(pos_l12[ep], 3);
        val -= r;
        sel += 512;
      }
    }
  }

  assert(val <= 512);

  while (true) {
    block_class = LookupBitmapAtPos(B, pos, 4);
    unsigned short tempint = offbits[block_class];
    pos += 4;
    block_offset = (block_class == 0) ? ACCESSBIT(B, pos) * 16 : 0;
    pos += tempint;

    if (val <= (block_class + block_offset)) {
      pos -= (4 + tempint);
      break;
    }
    val -= (block_class + block_offset);
    sel += 16;
  }

  block_class = LookupBitmapAtPos(B, pos, 4);
  pos += 4;
  block_offset = LookupBitmapAtPos(B, pos, offbits[block_class]);
  lastblock = decode_table[block_class][block_offset];

  uint64_t count = 0;
  for (uint8_t i = 0; i < 16; i++) {
    if ((lastblock >> (15 - i)) & 1) {
      count++;
    }
    if (count == val) {
      return sel + i;
    }
  }

  return sel;
}

// Get the 0-select of the dictionary at the specified index
uint64_t SuccinctBase::GetSelect0(SuccinctBase::Dictionary *D, uint64_t i) {
  uint64_t val = i + 1;
  int64_t sp = 0;
  int64_t ep = D->size / kTwo32;
  uint64_t m, r = 0;
  uint64_t pos = 0;
  uint64_t block_class, block_offset;
  uint64_t sel = 0;
  uint16_t lastblock;
  BitMap *B = D->B;
  uint64_t size = D->size;
  uint64_t *rank_l3 = D->rank_l3;
  uint64_t *rank_l12 = D->rank_l12;
  uint64_t *pos_l3 = D->pos_l3;
  uint64_t *pos_l12 = D->pos_l12;

  while (sp <= ep) {
    m = (sp + ep) / 2;
    r = m * kTwo32 - rank_l3[m];
    if (val > r)
      sp = m + 1;
    else
      ep = m - 1;
  }

  ep = SuccinctUtils::Max(ep, 0LL);
  sel += ep * kTwo32;
  val -= (ep * kTwo32 - rank_l3[ep]);
  pos += pos_l3[ep];
  sp = ep * kTwo32 / 2048;
  ep = SuccinctUtils::Min(((ep + 1) * kTwo32 / 2048),
                          SuccinctUtils::NumBlocks(size, 2048UL)) - 1;

  while (sp <= ep) {
    m = (sp + ep) / 2;
    r = m * 2048 - GETRANKL2(rank_l12[m]);
    if (val > r)
      sp = m + 1;
    else
      ep = m - 1;
  }

  ep = SuccinctUtils::Max(ep, 0LL);
  sel += ep * 2048;
  val -= (ep * 2048 - GETRANKL2(rank_l12[ep]));
  pos += GETPOSL2(pos_l12[ep]);

  assert(val <= 2048);
  r = (512 - GETRANKL1(rank_l12[ep], 1));
  if (sel + 512 < size && val > r) {
    pos += GETPOSL1(pos_l12[ep], 1);
    val -= r;
    sel += 512;
    r = (512 - GETRANKL1(rank_l12[ep], 2));
    if (sel + 512 < size && val > r) {
      pos += GETPOSL1(pos_l12[ep], 2);
      val -= r;
      sel += 512;
      r = (512 - GETRANKL1(rank_l12[ep], 3));
      if (sel + 512 < size && val > r) {
        pos += GETPOSL1(pos_l12[ep], 3);
        val -= r;
        sel += 512;
      }
    }
  }

  assert(val <= 512);

  while (true) {
    block_class = LookupBitmapAtPos(B, pos, 4);
    unsigned short tempint = offbits[block_class];
    pos += 4;
    block_offset = (block_class == 0) ? ACCESSBIT(B, pos) * 16 : 0;
    pos += tempint;

    if (val <= (16 - (block_class + block_offset))) {
      pos -= (4 + tempint);
      break;
    }

    val -= (16 - (block_class + block_offset));
    sel += 16;
  }

  block_class = LookupBitmapAtPos(D->B, pos, 4);
  pos += 4;
  block_offset = LookupBitmapAtPos(D->B, pos, offbits[block_class]);
  lastblock = decode_table[block_class][block_offset];

  uint64_t count = 0;
  for (uint8_t i = 0; i < 16; i++) {
    if (!((lastblock >> (15 - i)) & 1)) {
      count++;
    }
    if (count == val) {
      return sel + i;
    }
  }

  return sel;
}

size_t SuccinctBase::SerializeDictionary(Dictionary *D, std::ostream& out) {

  size_t out_size = 0;

  if (D == NULL) {
    out.write(reinterpret_cast<const char *>(&out_size), sizeof(uint64_t));
    return out_size;
  }

  out.write(reinterpret_cast<const char *>(&D->size), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  for (uint64_t i = 0; i < (D->size / L3BLKSIZE) + 1; i++) {
    out.write(reinterpret_cast<const char *>(&D->rank_l3[i]), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
  }

  for (uint64_t i = 0; i < (D->size / L2BLKSIZE) + 1; i++) {
    out.write(reinterpret_cast<const char *>(&D->rank_l12[i]),
              sizeof(uint64_t));
    out_size += sizeof(uint64_t);
  }

  for (uint64_t i = 0; i < (D->size / L3BLKSIZE) + 1; i++) {
    out.write(reinterpret_cast<const char *>(&D->pos_l3[i]), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
  }

  for (uint64_t i = 0; i < (D->size / L2BLKSIZE) + 1; i++) {
    out.write(reinterpret_cast<const char *>(&D->pos_l12[i]), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
  }

  out_size += SerializeBitmap(D->B, out);

  return out_size;
}

size_t SuccinctBase::DeserializeDictionary(Dictionary *D, std::istream& in) {

  size_t in_size = 0;
  uint64_t dictionary_size;

  in.read(reinterpret_cast<char *>(&dictionary_size), sizeof(uint64_t));
  if (dictionary_size) {
    in_size += sizeof(uint64_t);
    D->size = dictionary_size;

    D->rank_l3 = new uint64_t[(D->size / L3BLKSIZE) + 1];
    D->rank_l12 = new uint64_t[(D->size / L2BLKSIZE) + 1];
    D->pos_l3 = new uint64_t[(D->size / L3BLKSIZE) + 1];
    D->pos_l12 = new uint64_t[(D->size / L2BLKSIZE) + 1];

    for (uint64_t i = 0; i < (D->size / L3BLKSIZE) + 1; i++) {
      in.read(reinterpret_cast<char *>(&D->rank_l3[i]), sizeof(uint64_t));
      in_size += sizeof(uint64_t);
    }

    for (uint64_t i = 0; i < (D->size / L2BLKSIZE) + 1; i++) {
      in.read(reinterpret_cast<char *>(&D->rank_l12[i]), sizeof(uint64_t));
      in_size += sizeof(uint64_t);
    }

    for (uint64_t i = 0; i < (D->size / L3BLKSIZE) + 1; i++) {
      in.read(reinterpret_cast<char *>(&D->pos_l3[i]), sizeof(uint64_t));
      in_size += sizeof(uint64_t);
    }

    for (uint64_t i = 0; i < (D->size / L2BLKSIZE) + 1; i++) {
      in.read(reinterpret_cast<char *>(&D->pos_l12[i]), sizeof(uint64_t));
      in_size += sizeof(uint64_t);
    }

    in_size += DeserializeBitmap(&D->B, in);
  }

  return in_size;
}

size_t SuccinctBase::MemoryMapDictionary(Dictionary **D, uint8_t *buf) {
  uint8_t *data, *data_beg;
  data = data_beg = buf;

  uint64_t dictionary_size = *((uint64_t *) data);
  data += sizeof(uint64_t);
  if (dictionary_size) {
    (*D) = new Dictionary;
    (*D)->size = dictionary_size;

    (*D)->rank_l3 = ((uint64_t *) data);
    data += (((*D)->size / L3BLKSIZE) + 1) * sizeof(uint64_t);
    (*D)->rank_l12 = ((uint64_t *) data);
    data += (((*D)->size / L2BLKSIZE) + 1) * sizeof(uint64_t);
    (*D)->pos_l3 = ((uint64_t *) data);
    data += (((*D)->size / L3BLKSIZE) + 1) * sizeof(uint64_t);
    (*D)->pos_l12 = ((uint64_t *) data);
    data += (((*D)->size / L2BLKSIZE) + 1) * sizeof(uint64_t);

    data += MemoryMapBitmap(&(*D)->B, data);
  }

  return data - data_beg;
}

size_t SuccinctBase::VectorSize(std::vector<uint64_t> &v) {
  return sizeof(v.size()) + v.size() * sizeof(uint64_t);
}

size_t SuccinctBase::BitmapSize(BitMap *B) {
  if (B == NULL)
    return 0;
  return sizeof(B->size) + BITS2BLOCKS(B->size) * 8;
}

size_t SuccinctBase::DictionarySize(Dictionary *D) {
  if (D == NULL)
    return 0;
  return sizeof(D->size)
      + 2 * ((D->size / L3BLKSIZE) + (D->size / L2BLKSIZE) + 2)
          * sizeof(uint64_t) + BitmapSize(D->B);
}

size_t SuccinctBase::StorageSize() {
  // Size of tables + size of constants
  return 1048729 + 8 * 2;
}
