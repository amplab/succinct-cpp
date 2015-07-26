#ifndef SUCCINCT_UTILS_H
#define SUCCINCT_UTILS_H

#include <cstdint>
#include <string>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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
  static uint64_t popcount(uint64_t n);

  // Returns integer logarithm to the base 2
  static uint32_t int_log_2(uint64_t n);

  // Returns a modulo n
  static uint64_t modulo(int64_t a, uint64_t n);

  static void* memory_map(std::string filename);
};

#endif
