#ifndef FLAT_SAMPLED_ARRAY_H
#define FLAT_SAMPLED_ARRAY_H

#include <ostream>

#include "succinct_base.h"
#include "utils/succinct_allocator.h"
#include "npa/npa.h"
#include "sampled_array.h"

class FlatSampledArray : public SampledArray {
 public:
  typedef SuccinctBase::BitMap bitmap_t;
  typedef SuccinctBase::Dictionary Dictionary;

  // Constructor
  FlatSampledArray(uint32_t sampling_rate, SamplingScheme scheme, NPA *npa,
                   SuccinctAllocator &s_allocator)
      : SampledArray(scheme) {
    this->data_ = NULL;
    this->data_bits_ = 0;
    this->sampling_rate_ = sampling_rate;
    this->data_size_ = 0;
    this->original_size_ = 0;
    this->npa_ = npa;
    this->succinct_allocator_ = s_allocator;
  }

  // Virtual destructor
  virtual ~FlatSampledArray() {
  }
  ;

  // Return sampling rate
  uint32_t GetSamplingRate() {
    return sampling_rate_;
  }

  bool IsSampled(uint64_t i) {
    return i % sampling_rate_ == 0;
  }

  uint64_t OriginalSize() {
    return original_size_;
  }

  uint64_t SampledSize() {
    return data_size_;
  }

  // Access sampled array at index i
  virtual uint64_t operator[](uint64_t i) = 0;

  virtual uint64_t GetSampleAt(uint64_t i) {
    return SuccinctBase::LookupBitmapArray(data_, i, data_bits_);
  }

  virtual size_t Serialize(std::ostream& out) {
    size_t out_size = 0;

    out.write(reinterpret_cast<const char *>(&data_size_), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
    out.write(reinterpret_cast<const char *>(&data_bits_), sizeof(uint8_t));
    out_size += sizeof(uint8_t);
    out.write(reinterpret_cast<const char *>(&original_size_),
              sizeof(uint64_t));
    out_size += sizeof(uint64_t);
    out.write(reinterpret_cast<const char *>(&sampling_rate_),
              sizeof(uint32_t));
    out_size += sizeof(uint32_t);

    out_size += SuccinctBase::SerializeBitmap(data_, out);

    return out_size;
  }

  virtual size_t Deserialize(std::istream& in) {
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(&data_size_), sizeof(uint64_t));
    in_size += sizeof(uint64_t);
    in.read(reinterpret_cast<char *>(&data_bits_), sizeof(uint8_t));
    in_size += sizeof(uint8_t);
    in.read(reinterpret_cast<char *>(&original_size_), sizeof(uint64_t));
    in_size += sizeof(uint64_t);
    in.read(reinterpret_cast<char *>(&sampling_rate_), sizeof(uint32_t));
    in_size += sizeof(uint32_t);

    in_size += SuccinctBase::DeserializeBitmap(&data_, in);

    return in_size;
  }

  virtual size_t MemoryMap(std::string filename) {
    uint8_t *data_buf, *data_beg;
    data_buf = data_beg = (uint8_t *) SuccinctUtils::MemoryMap(filename);

    data_size_ = *((uint64_t *) data_buf);
    data_buf += sizeof(uint64_t);
    data_bits_ = *((uint8_t *) data_buf);
    data_buf += sizeof(uint8_t);
    original_size_ = *((uint64_t *) data_buf);
    data_buf += sizeof(uint64_t);
    sampling_rate_ = *((uint32_t *) data_buf);
    data_buf += sizeof(uint32_t);

    data_buf += SuccinctBase::MemoryMapBitmap(&data_, data_buf);

    return data_buf - data_beg;
  }

  virtual size_t StorageSize() {
    return sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint64_t)
        + sizeof(uint32_t) + SuccinctBase::BitmapSize(data_);
  }

 protected:
  // Sample original array using the sampling scheme
  virtual void Sample(ArrayStream& original, uint64_t n) = 0;

  bitmap_t *data_;
  uint8_t data_bits_;
  uint64_t data_size_;
  uint64_t original_size_;
  uint32_t sampling_rate_;

  NPA *npa_;
  SuccinctAllocator succinct_allocator_;
};

#endif
