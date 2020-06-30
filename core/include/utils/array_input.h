#ifndef CORE_ARRAY_INPUT_H_
#define CORE_ARRAY_INPUT_H_

#include <iostream>
#include <fstream>
#include <cstdio>
#include <inttypes.h>

#include "utils/succinct_utils.h"

class ArrayInput {
 public:
  ArrayInput(int64_t *array, uint64_t start_idx = 0) {
    current_idx_ = start_idx;
    array_ = array;
  }

  uint64_t Get() {
    uint64_t val = array_[current_idx_];
    fprintf(stderr, "item at index %" PRIu64 " is %" PRIu64 "\n", current_idx_, val);
    current_idx_++;
    return val;
  }

  uint64_t GetCurrentIndex() {
    return current_idx_;
  }

  void Reset() {
    current_idx_ = 0;
  }
  
 private:
  int64_t *array_;
  uint64_t current_idx_;
  uint64_t size_;
};

#endif // CORE_ARRAY_STREAM_H_
