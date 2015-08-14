#ifndef CORE_ARRAY_STREAM_H_
#define CORE_ARRAY_STREAM_H_

#include <iostream>
#include <fstream>
#include <cstdio>

#include "utils/succinct_utils.h"

class ArrayStream {
 public:
  ArrayStream(std::string filename, bool memory_map = false) {
    current_idx_ = 0;
    memory_map_ = memory_map;
    filename_ = filename;

    if (!memory_map_) {
      in_ = std::ifstream(filename_);
      data_ = NULL;
    } else {
      data_ = (uint64_t *) SuccinctUtils::MemoryMap(filename_);
    }
  }

  uint64_t Get() {
    if (memory_map_) {
      return data_[current_idx_++];
    }
    uint64_t val;
    in_.read(reinterpret_cast<char *>(&(val)), sizeof(uint64_t));
    current_idx_++;
    return val;
  }

  uint64_t GetCurrentIndex() {
    return current_idx_;
  }

  void Reset() {
    if (!memory_map_) {
      in_.clear();
      in_.seekg(0, std::ios::beg);
    }
    current_idx_ = 0;
  }

  void Close() {
    if (!memory_map_) {
      in_.close();
    }
    remove(filename_.c_str());
  }

 private:
  std::string filename_;
  std::ifstream in_;
  uint64_t *data_;
  uint64_t current_idx_;
  bool memory_map_;

};

#endif // CORE_ARRAY_STREAM_H_
