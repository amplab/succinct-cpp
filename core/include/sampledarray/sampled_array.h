#ifndef SAMPLED_ARRAY_HPP
#define SAMPLED_ARRAY_HPP

#include "sampling_scheme.h"
#include "utils/array_stream.h"

class SampledArray {
 public:
  SampledArray(SamplingScheme sampling_scheme) {
    sampling_scheme_ = sampling_scheme;
  }
  virtual ~SampledArray() {
  }

  virtual uint64_t operator[](uint64_t i) = 0;
  virtual uint64_t at(uint64_t i) {
    return operator[](i);
  }

  virtual size_t Serialize(std::ostream& out) = 0;
  virtual size_t Deserialize(std::istream& in) = 0;
  virtual size_t MemoryMap(uint8_t* data) = 0;

  SamplingScheme GetSamplingScheme() {
    return sampling_scheme_;
  }

  virtual uint32_t GetSamplingRate() = 0;

  virtual bool IsSampled(uint64_t i) = 0;

  virtual size_t StorageSize() = 0;

 private:
  SamplingScheme sampling_scheme_;
};

#endif /* SAMPLED_ARRAY_HPP */
