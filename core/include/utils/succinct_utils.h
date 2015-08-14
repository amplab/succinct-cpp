#ifndef SUCCINCT_UTILS_H
#define SUCCINCT_UTILS_H

#include <cstdint>
#include <string>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

#include "assertions.h"
#include "definitions.h"

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0
#endif

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

class SuccinctUtils {
 public:
  // Returns the number of set bits in a 64 bit integer
  static uint64_t PopCount(uint64_t n);

  // Returns integer logarithm to the base 2
  static uint32_t IntegerLog2(uint64_t n);

  // Returns a modulo n
  static uint64_t Modulo(int64_t a, uint64_t n);

  // Computes the number of blocks of block_size in val
  static uint64_t NumBlocks(uint64_t val, uint64_t block_size);

  // Computes the max between two integers
  static int64_t Max(int64_t first, int64_t second);

  // Computes the min between two integers
  static int64_t Min(int64_t first, int64_t second);

  // Memory maps a file and returns a pointer to it
  static void* MemoryMap(std::string filename);

  // Writes an integer array to file
  static void WriteToFile(int64_t* data, size_t size, std::string outfile);
};

#endif
