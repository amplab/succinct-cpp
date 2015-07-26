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
#define L3BLKSIZE   two32

class SuccinctBase {
 private:
  void init_tables();

 public:

  SuccinctBase();

  virtual ~SuccinctBase() {
  }

  /* ===================================================================== */
  /* Basic data structures used in Succinct */
  // Basic constants
  static const uint64_t two32 = 1L << 32;
  static const uint64_t all_ones = ~(0ULL);

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
  uint16_t *decode_table[17];
  std::map<uint16_t, uint16_t> encode_table[17];
  uint16_t C16[17];
  uint8_t offbits[17];
  uint8_t smallrank[65536][16];

  // Rank for vectors -- implemented as a binary search
  static uint64_t get_rank1(std::vector<uint64_t> *V, uint64_t val);

  // Serialize vector to output stream
  static size_t serialize_vector(std::vector<uint64_t> &v, std::ostream& out);

  // Deserialize vector from input stream
  static size_t deserialize_vector(std::vector<uint64_t> &v, std::istream& in);

  // Memory map vector from buffer
  static size_t memorymap_vector(std::vector<uint64_t> &v, uint8_t *buf);

  /* BitMap access/modifier functions */
  // Initialize a bitmap with a specified size
  static void init_bitmap(BitMap **B, uint64_t size_in_bits,
                          SuccinctAllocator s_allocator);

  // Initialize a bitmap with a specified size, with all bits set
  static void init_bitmap_set(BitMap **B, uint64_t size_in_bits,
                              SuccinctAllocator s_allocator);

  // Clear the contents of the bitmap and unset all bits
  static void clear_bitmap(BitMap **B, SuccinctAllocator s_allocator);

  // Destroy a bitmap
  static void destroy_bitmap(BitMap **B, SuccinctAllocator s_allocator);

  // Create bitmap from an array with specified bit-width
  static void create_bitmap_array(BitMap **B, uint64_t *A, uint64_t n,
                                  uint32_t b, SuccinctAllocator s_allocator);

  // Set a value in the bitmap, treating it as an array of fixed length
  static void set_bitmap_array(BitMap **B, uint64_t i, uint64_t val,
                               uint32_t b);

  // Lookup a value in the bitmap, treating it as an array of fixed length
  static uint64_t lookup_bitmap_array(BitMap *B, uint64_t i, uint32_t b);

  // Set a value in the bitmap at a specified offset
  static void set_bitmap_pos(BitMap **B, uint64_t pos, uint64_t val,
                             uint32_t b);

  // Lookup a value in the bitmap at a specified offset
  static uint64_t lookup_bitmap_pos(BitMap *B, uint64_t pos, uint32_t b);

  // Serialize bitmap to output stream
  static size_t serialize_bitmap(BitMap *B, std::ostream& out);

  // Deserialize bitmap from input stream
  static size_t deserialize_bitmap(BitMap **B, std::istream& in);

  // Memory map dictionary from buf
  static size_t memorymap_bitmap(BitMap **B, uint8_t *buf);

  /* Dictionary access/modifier functions */
  // Create dictionary from a bitmap
  uint64_t create_dictionary(BitMap *B, Dictionary *D);

  // Get the 1-rank of the dictionary at the specified index
  uint64_t get_rank1(Dictionary *D, uint64_t i);

  // Get the 0-rank of the dictionary at the specified index
  uint64_t get_rank0(Dictionary *D, uint64_t i);

  // Get the 1-select of the dictionary at the specified index
  uint64_t get_select1(Dictionary *D, uint64_t i);

  // Get the 0-select of the dictionary at the specified index
  uint64_t get_select0(Dictionary *D, uint64_t i);

  // Serialize dictionary to output stream
  size_t serialize_dictionary(Dictionary *D, std::ostream& out);

  // Deserialize dictionary from input stream
  size_t deserialize_dictionary(Dictionary **D, std::istream& in);

  // Memory map dictionary
  size_t memorymap_dictionary(Dictionary **D, uint8_t *buf);

  // Get size of bitmap
  static size_t bitmap_size(BitMap *B);

  // Get size of dictionary
  static size_t dictionary_size(Dictionary *D);

  // Get size of vector
  static size_t vector_size(std::vector<uint64_t> &v);

  // Get size of SuccinctBase
  virtual size_t storage_size();

 protected:
  SuccinctAllocator s_allocator;

};

#endif
