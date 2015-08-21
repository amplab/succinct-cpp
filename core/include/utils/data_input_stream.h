#ifndef CORE_ARRAY_STREAM_H_
#define CORE_ARRAY_STREAM_H_

#include <iostream>
#include <fstream>
#include <cstdio>

#include "utils/succinct_utils.h"

template<typename T>
class DataInputStream {
 public:
  typedef size_t pos_type;

  DataInputStream(std::string filename, bool memory_map = false) {
    current_idx_ = 0;
    memory_map_ = memory_map;
    filename_ = filename;

    if (!memory_map_) {
      in_.open(filename_);
      data_ = NULL;
    } else {
      data_ = (T *) SuccinctUtils::MemoryMap(filename_);
    }
  }

  T Get() {
    if (memory_map_) {
      return data_[current_idx_++];
    }
    T val;
    in_.read(reinterpret_cast<char *>(&(val)), sizeof(T));
    current_idx_++;
    return val;
  }

  pos_type GetCurrentIndex() {
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
  T *data_;
  pos_type current_idx_;
  bool memory_map_;

};

#endif // CORE_ARRAY_STREAM_H_
