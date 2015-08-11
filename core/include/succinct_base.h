#ifndef SUCCINCT_BASE_H
#define SUCCINCT_BASE_H

#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <ostream>
#include <istream>

#include "utils/assertions.h"
#include "utils/buf_allocator.h"
#include "utils/definitions.h"
#include "utils/succinct_allocator.h"
#include "utils/succinct_utils.h"

/* Bitmap Operations */
#define ACCESSBIT(B, i) GETBIT((B)->bitmap[(i) / 64], (i) % 64)
#define SETBITVAL(B, i) SETBIT((B)->bitmap[(i) / 64], (i) % 64)

/* Get rank operations */
#define GETRANKL2(n)    (n >> 32)
#define GETRANKL1(n, i) (((n & 0xffffffff) >> (32 - i * 10)) & 0x3ff)

#define GETPOSL2(n)     (n >> 31)
#define GETPOSL1(n, i)  (((n & 0x7fffffff) >> (31 - i * 10)) & 0x3ff)

/* Rank Data structure constants */
#define L1BLKSIZE   512
#define L2BLKSIZE   2048
#define L3BLKSIZE   kTwo32

class SuccinctBase {
 public:
  /* ===================================================================== */
  /* Basic data structures used in Succinct */
  // Basic constants
  static const uint64_t kTwo32 = 1L << 32;
  static const uint64_t kAllOnes = ~(0ULL);

  // Bitmap stored as a 64-bit array
  typedef struct _bitmap {
    uint64_t *bitmap;
    uint64_t size;
  } BitMap;

  // Dictionary stored as a bitmap with rank data structures
  typedef struct _dict {
    BitMap *B;
    uint64_t size;
    uint64_t *rank_l12;
    uint64_t *rank_l3;
    uint64_t *pos_l12;
    uint64_t *pos_l3;
  } Dictionary;

  /* Constant tables used for encoding/decoding
   * dictionary and delta encoded vector */
  static uint16_t *decode_table[17];
  static std::map<uint16_t, uint16_t> encode_table[17];
  static uint16_t C16[17];
  static uint8_t offbits[17];
  static uint8_t smallrank[65536][16];

  SuccinctBase();

  virtual ~SuccinctBase() {
  }

  // Rank for vectors -- implemented as a binary search
  static uint64_t GetRank1(std::vector<uint64_t> *V, uint64_t val);

  // Serialize vector to output stream
  static size_t SerializeVector(std::vector<uint64_t> &v, std::ostream& out);

  // Deserialize vector from input stream
  static size_t DeserializeVector(std::vector<uint64_t> &v, std::istream& in);

  // Memory map vector from buffer
  static size_t MemoryMapVector(std::vector<uint64_t> &v, uint8_t *buf);

  /* BitMap access/modifier functions */
  // Initialize a bitmap with a specified size
  static void InitBitmap(BitMap **B, uint64_t size_in_bits,
                         SuccinctAllocator& s_allocator);

  // Initialize a bitmap with a specified size, with all bits set
  static void InitBitmapSet(BitMap **B, uint64_t size_in_bits,
                            SuccinctAllocator& s_allocator);

  // Clear the contents of the bitmap and unset all bits
  static void ClearBitmap(BitMap **B, SuccinctAllocator& s_allocator);

  // Destroy a bitmap
  static void DestroyBitmap(BitMap **B, SuccinctAllocator& s_allocator);

  // Create bitmap from an array with specified bit-width
  static void CreateBitmapArray(BitMap **B, uint64_t *A, uint64_t n, uint32_t b,
                                SuccinctAllocator s_allocator);

  // Set a value in the bitmap, treating it as an array of fixed length
  static void SetBitmapArray(BitMap **B, uint64_t i, uint64_t val, uint32_t b);

  // Lookup a value in the bitmap, treating it as an array of fixed length
  static uint64_t LookupBitmapArray(BitMap *B, uint64_t i, uint32_t b);

  // Set a value in the bitmap at a specified offset
  static void SetBitmapAtPos(BitMap **B, uint64_t pos, uint64_t val,
                             uint32_t b);

  // Lookup a value in the bitmap at a specified offset
  static uint64_t LookupBitmapAtPos(BitMap *B, uint64_t pos, uint32_t b);

  // Serialize bitmap to output stream
  static size_t SerializeBitmap(BitMap *B, std::ostream& out);

  // Deserialize bitmap from input stream
  static size_t DeserializeBitmap(BitMap **B, std::istream& in);

  // Memory map dictionary from buf
  static size_t MemoryMapBitmap(BitMap **B, uint8_t *buf);

  /* Dictionary access/modifier functions */
  // Create dictionary from a bitmap
  static uint64_t CreateDictionary(BitMap *B, Dictionary *D, SuccinctAllocator& s_allocator);

  // Get the 1-rank of the dictionary at the specified index
  static uint64_t GetRank1(Dictionary *D, uint64_t i);

  // Get the 0-rank of the dictionary at the specified index
  static uint64_t GetRank0(Dictionary *D, uint64_t i);

  // Get the 1-select of the dictionary at the specified index
  static uint64_t GetSelect1(Dictionary *D, uint64_t i);

  // Get the 0-select of the dictionary at the specified index
  static uint64_t GetSelect0(Dictionary *D, uint64_t i);

  // Serialize dictionary to output stream
  static size_t SerializeDictionary(Dictionary *D, std::ostream& out);

  // Deserialize dictionary from input stream
  static size_t DeserializeDictionary(Dictionary *D, std::istream& in);

  // Memory map dictionary
  static size_t MemoryMapDictionary(Dictionary **D, uint8_t *buf);

  // Get size of bitmap
  static size_t BitmapSize(BitMap *B);

  // Get size of dictionary
  static size_t DictionarySize(Dictionary *D);

  // Get size of vector
  static size_t VectorSize(std::vector<uint64_t> &v);

  // Get size of SuccinctBase
  virtual size_t StorageSize();

 protected:
  SuccinctAllocator s_allocator;

 private:
  static void InitTables();

};

inline uint64_t SuccinctBase::GetRank1(std::vector<uint64_t> *V, uint64_t i) {
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

// Lookup a value in the bitmap, treating it as an array of fixed length
inline uint64_t SuccinctBase::LookupBitmapArray(SuccinctBase::BitMap *B, uint64_t i,
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

// Lookup a value in the bitmap at a specified offset
inline uint64_t SuccinctBase::LookupBitmapAtPos(SuccinctBase::BitMap *B, uint64_t pos,
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

#endif
