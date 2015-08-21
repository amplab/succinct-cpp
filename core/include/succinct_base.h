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

static const uint64_t high_bits_set[65] = { 0x0000000000000000ULL,
    0x8000000000000000ULL, 0xC000000000000000ULL, 0xE000000000000000ULL,
    0xF000000000000000ULL, 0xF800000000000000ULL, 0xFC00000000000000ULL,
    0xFE00000000000000ULL, 0xFF00000000000000ULL, 0xFF80000000000000ULL,
    0xFFC0000000000000ULL, 0xFFE0000000000000ULL, 0xFFF0000000000000ULL,
    0xFFF8000000000000ULL, 0xFFFC000000000000ULL, 0xFFFE000000000000ULL,
    0xFFFF000000000000ULL, 0xFFFF800000000000ULL, 0xFFFFC00000000000ULL,
    0xFFFFE00000000000ULL, 0xFFFFF00000000000ULL, 0xFFFFF80000000000ULL,
    0xFFFFFC0000000000ULL, 0xFFFFFE0000000000ULL, 0xFFFFFF0000000000ULL,
    0xFFFFFF8000000000ULL, 0xFFFFFFC000000000ULL, 0xFFFFFFE000000000ULL,
    0xFFFFFFF000000000ULL, 0xFFFFFFF800000000ULL, 0xFFFFFFFC00000000ULL,
    0xFFFFFFFE00000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFFFFFF80000000ULL,
    0xFFFFFFFFC0000000ULL, 0xFFFFFFFFE0000000ULL, 0xFFFFFFFFF0000000ULL,
    0xFFFFFFFFF8000000ULL, 0xFFFFFFFFFC000000ULL, 0xFFFFFFFFFE000000ULL,
    0xFFFFFFFFFF000000ULL, 0xFFFFFFFFFF800000ULL, 0xFFFFFFFFFFC00000ULL,
    0xFFFFFFFFFFE00000ULL, 0xFFFFFFFFFFF00000ULL, 0xFFFFFFFFFFF80000ULL,
    0xFFFFFFFFFFFC0000ULL, 0xFFFFFFFFFFFE0000ULL, 0xFFFFFFFFFFFF0000ULL,
    0xFFFFFFFFFFFF8000ULL, 0xFFFFFFFFFFFFC000ULL, 0xFFFFFFFFFFFFE000ULL,
    0xFFFFFFFFFFFFF000ULL, 0xFFFFFFFFFFFFF800ULL, 0xFFFFFFFFFFFFFC00ULL,
    0xFFFFFFFFFFFFFE00ULL, 0xFFFFFFFFFFFFFF00ULL, 0xFFFFFFFFFFFFFF80ULL,
    0xFFFFFFFFFFFFFFC0ULL, 0xFFFFFFFFFFFFFFE0ULL, 0xFFFFFFFFFFFFFFF0ULL,
    0xFFFFFFFFFFFFFFF8ULL, 0xFFFFFFFFFFFFFFFCULL, 0xFFFFFFFFFFFFFFFEULL,
    0xFFFFFFFFFFFFFFFFULL };

static const uint64_t high_bits_unset[65] = { 0xFFFFFFFFFFFFFFFFULL,
    0x7FFFFFFFFFFFFFFFULL, 0x3FFFFFFFFFFFFFFFULL, 0x1FFFFFFFFFFFFFFFULL,
    0x0FFFFFFFFFFFFFFFULL, 0x07FFFFFFFFFFFFFFULL, 0x03FFFFFFFFFFFFFFULL,
    0x01FFFFFFFFFFFFFFULL, 0x00FFFFFFFFFFFFFFULL, 0x007FFFFFFFFFFFFFULL,
    0x003FFFFFFFFFFFFFULL, 0x001FFFFFFFFFFFFFULL, 0x000FFFFFFFFFFFFFULL,
    0x0007FFFFFFFFFFFFULL, 0x0003FFFFFFFFFFFFULL, 0x0001FFFFFFFFFFFFULL,
    0x0000FFFFFFFFFFFFULL, 0x00007FFFFFFFFFFFULL, 0x00003FFFFFFFFFFFULL,
    0x00001FFFFFFFFFFFULL, 0x00000FFFFFFFFFFFULL, 0x000007FFFFFFFFFFULL,
    0x000003FFFFFFFFFFULL, 0x000001FFFFFFFFFFULL, 0x000000FFFFFFFFFFULL,
    0x0000007FFFFFFFFFULL, 0x0000003FFFFFFFFFULL, 0x0000001FFFFFFFFFULL,
    0x0000000FFFFFFFFFULL, 0x00000007FFFFFFFFULL, 0x00000003FFFFFFFFULL,
    0x00000001FFFFFFFFULL, 0x00000000FFFFFFFFULL, 0x000000007FFFFFFFULL,
    0x000000003FFFFFFFULL, 0x000000001FFFFFFFULL, 0x000000000FFFFFFFULL,
    0x0000000007FFFFFFULL, 0x0000000003FFFFFFULL, 0x0000000001FFFFFFULL,
    0x0000000000FFFFFFULL, 0x00000000007FFFFFULL, 0x00000000003FFFFFULL,
    0x00000000001FFFFFULL, 0x00000000000FFFFFULL, 0x000000000007FFFFULL,
    0x000000000003FFFFULL, 0x000000000001FFFFULL, 0x000000000000FFFFULL,
    0x0000000000007FFFULL, 0x0000000000003FFFULL, 0x0000000000001FFFULL,
    0x0000000000000FFFULL, 0x00000000000007FFULL, 0x00000000000003FFULL,
    0x00000000000001FFULL, 0x00000000000000FFULL, 0x000000000000007FULL,
    0x000000000000003FULL, 0x000000000000001FULL, 0x000000000000000FULL,
    0x0000000000000007ULL, 0x0000000000000003ULL, 0x0000000000000001ULL,
    0x0000000000000000ULL };

static const uint64_t low_bits_set[65] = { 0x0000000000000000ULL,
    0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000007ULL,
    0x000000000000000FULL, 0x000000000000001FULL, 0x000000000000003FULL,
    0x000000000000007FULL, 0x00000000000000FFULL, 0x00000000000001FFULL,
    0x00000000000003FFULL, 0x00000000000007FFULL, 0x0000000000000FFFULL,
    0x0000000000001FFFULL, 0x0000000000003FFFULL, 0x0000000000007FFFULL,
    0x000000000000FFFFULL, 0x000000000001FFFFULL, 0x000000000003FFFFULL,
    0x000000000007FFFFULL, 0x00000000000FFFFFULL, 0x00000000001FFFFFULL,
    0x00000000003FFFFFULL, 0x00000000007FFFFFULL, 0x0000000000FFFFFFULL,
    0x0000000001FFFFFFULL, 0x0000000003FFFFFFULL, 0x0000000007FFFFFFULL,
    0x000000000FFFFFFFULL, 0x000000001FFFFFFFULL, 0x000000003FFFFFFFULL,
    0x000000007FFFFFFFULL, 0x00000000FFFFFFFFULL, 0x00000001FFFFFFFFULL,
    0x00000003FFFFFFFFULL, 0x00000007FFFFFFFFULL, 0x0000000FFFFFFFFFULL,
    0x0000001FFFFFFFFFULL, 0x0000003FFFFFFFFFULL, 0x0000007FFFFFFFFFULL,
    0x000000FFFFFFFFFFULL, 0x000001FFFFFFFFFFULL, 0x000003FFFFFFFFFFULL,
    0x000007FFFFFFFFFFULL, 0x00000FFFFFFFFFFFULL, 0x00001FFFFFFFFFFFULL,
    0x00003FFFFFFFFFFFULL, 0x00007FFFFFFFFFFFULL, 0x0000FFFFFFFFFFFFULL,
    0x0001FFFFFFFFFFFFULL, 0x0003FFFFFFFFFFFFULL, 0x0007FFFFFFFFFFFFULL,
    0x000FFFFFFFFFFFFFULL, 0x001FFFFFFFFFFFFFULL, 0x003FFFFFFFFFFFFFULL,
    0x007FFFFFFFFFFFFFULL, 0x00FFFFFFFFFFFFFFULL, 0x01FFFFFFFFFFFFFFULL,
    0x03FFFFFFFFFFFFFFULL, 0x07FFFFFFFFFFFFFFULL, 0x0FFFFFFFFFFFFFFFULL,
    0x1FFFFFFFFFFFFFFFULL, 0x3FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFFULL };

static const uint64_t low_bits_unset[65] = { 0xFFFFFFFFFFFFFFFFULL,
    0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFCULL, 0xFFFFFFFFFFFFFFF8ULL,
    0xFFFFFFFFFFFFFFF0ULL, 0xFFFFFFFFFFFFFFE0ULL, 0xFFFFFFFFFFFFFFC0ULL,
    0xFFFFFFFFFFFFFF80ULL, 0xFFFFFFFFFFFFFF00ULL, 0xFFFFFFFFFFFFFE00ULL,
    0xFFFFFFFFFFFFFC00ULL, 0xFFFFFFFFFFFFF800ULL, 0xFFFFFFFFFFFFF000ULL,
    0xFFFFFFFFFFFFE000ULL, 0xFFFFFFFFFFFFC000ULL, 0xFFFFFFFFFFFF8000ULL,
    0xFFFFFFFFFFFF0000ULL, 0xFFFFFFFFFFFE0000ULL, 0xFFFFFFFFFFFC0000ULL,
    0xFFFFFFFFFFF80000ULL, 0xFFFFFFFFFFF00000ULL, 0xFFFFFFFFFFE00000ULL,
    0xFFFFFFFFFFC00000ULL, 0xFFFFFFFFFF800000ULL, 0xFFFFFFFFFF000000ULL,
    0xFFFFFFFFFE000000ULL, 0xFFFFFFFFFC000000ULL, 0xFFFFFFFFF8000000ULL,
    0xFFFFFFFFF0000000ULL, 0xFFFFFFFFE0000000ULL, 0xFFFFFFFFC0000000ULL,
    0xFFFFFFFF80000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFFFFFE00000000ULL,
    0xFFFFFFFC00000000ULL, 0xFFFFFFF800000000ULL, 0xFFFFFFF000000000ULL,
    0xFFFFFFE000000000ULL, 0xFFFFFFC000000000ULL, 0xFFFFFF8000000000ULL,
    0xFFFFFF0000000000ULL, 0xFFFFFE0000000000ULL, 0xFFFFFC0000000000ULL,
    0xFFFFF80000000000ULL, 0xFFFFF00000000000ULL, 0xFFFFE00000000000ULL,
    0xFFFFC00000000000ULL, 0xFFFF800000000000ULL, 0xFFFF000000000000ULL,
    0xFFFE000000000000ULL, 0xFFFC000000000000ULL, 0xFFF8000000000000ULL,
    0xFFF0000000000000ULL, 0xFFE0000000000000ULL, 0xFFC0000000000000ULL,
    0xFF80000000000000ULL, 0xFF00000000000000ULL, 0xFE00000000000000ULL,
    0xFC00000000000000ULL, 0xF800000000000000ULL, 0xF000000000000000ULL,
    0xE000000000000000ULL, 0xC000000000000000ULL, 0x8000000000000000ULL,
    0x0000000000000000ULL };

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
  static void SetBitmapArray(BitMap **B, uint64_t i, uint64_t val, uint32_t bits) {
    SuccinctBase::SetBitmapAtPos(B, i * bits, val, bits);
  }

  // Lookup a value in the bitmap, treating it as an array of fixed length
  static uint64_t LookupBitmapArray(BitMap *B, uint64_t i, uint32_t bits) {
    return SuccinctBase::LookupBitmapAtPos(B, i * bits, bits);
  }

  // Set a value in the bitmap at a specified offset
  static void SetBitmapAtPos(BitMap **B, uint64_t pos, uint64_t val,
                             uint32_t bits) {
    uint64_t s_off = pos % 64;
    uint64_t s_idx = pos / 64;

    if (s_off + bits <= 64) {
      // Can be accommodated in 1 bitmap block
      (*B)->bitmap[s_idx] = ((*B)->bitmap[s_idx]
          & (low_bits_set[s_off] | low_bits_unset[s_off + bits]))
          | val << s_off;
    } else {
      // Must use 2 bitmap blocks
      (*B)->bitmap[s_idx] = ((*B)->bitmap[s_idx] & low_bits_set[s_off])
          | val << s_off;
      (*B)->bitmap[s_idx + 1] = ((*B)->bitmap[s_idx + 1]
          & low_bits_unset[(s_off + bits) % 64]) | (val >> (64 - s_off));
    }
  }

  // Lookup a value in the bitmap at a specified offset
  static uint64_t LookupBitmapAtPos(BitMap *B, uint64_t pos, uint32_t bits) {
    uint64_t s_off = pos % 64;
    uint64_t s_idx = pos / 64;

    if (s_off + bits <= 64) {
      // Can be read from a single block
      return (B->bitmap[s_idx] >> s_off) & low_bits_set[bits];
    } else {
      // Must be read from two blocks
      return ((B->bitmap[s_idx] >> s_off)
          | (B->bitmap[s_idx + 1] << (64 - s_off))) & low_bits_set[bits];
    }
  }

  // Serialize bitmap to output stream
  static size_t SerializeBitmap(BitMap *B, std::ostream& out);

  // Deserialize bitmap from input stream
  static size_t DeserializeBitmap(BitMap **B, std::istream& in);

  // Memory map dictionary from buf
  static size_t MemoryMapBitmap(BitMap **B, uint8_t *buf);

  /* Dictionary access/modifier functions */
  // Create dictionary from a bitmap
  static uint64_t CreateDictionary(BitMap *B, Dictionary *D,
                                   SuccinctAllocator& s_allocator);

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

#endif
