#ifndef NPA_H
#define NPA_H

#include "../succinct_base.h"

class NPA {
 public:
  typedef enum npa_encoding_scheme {
    WAVELET_TREE_ENCODED = 0,
    ELIAS_DELTA_ENCODED = 1,
    ELIAS_GAMMA_ENCODED = 2
  } NPAEncodingScheme;

  typedef SuccinctBase::BitMap bitmap_t;

 public:
  // Constructor
  NPA(uint64_t npa_size, uint64_t sigma_size, uint32_t context_len,
      uint32_t sampling_rate, NPAEncodingScheme encoding_scheme,
      SuccinctAllocator &s_allocator) {

    npa_size_ = npa_size;
    sigma_size_ = sigma_size;
    context_len_ = context_len;
    sampling_rate_ = sampling_rate;
    col_nec_ = NULL;
    row_nec_ = NULL;
    cell_offsets_ = NULL;
    encoding_scheme_ = encoding_scheme;
    s_allocator_ = s_allocator;
  }
  // Virtual destructor
  virtual ~NPA() {
  }

  // Returns the encoding scheme for the NPA
  NPAEncodingScheme encoding_scheme() {
    return encoding_scheme_;
  }

  // Get size of NPA
  uint64_t npa_size() {
    return npa_size_;
  }

  uint64_t get_context_len() {
    return context_len_;
  }

  uint32_t get_sampling_rate() {
    return sampling_rate_;
  }

  // Encode NPA based on the encoding scheme
  virtual void Encode(bitmap_t *data_bitmap, bitmap_t *compactSA,
                      bitmap_t *compactISA) = 0;

  // Access element at index i
  virtual uint64_t operator[](uint64_t i) = 0;

  // Access element at index i
  virtual uint64_t at(uint64_t i) {
    return operator[](i);
  }

  virtual size_t Serialize(std::ostream& out) = 0;

  virtual size_t Deserialize(std::istream& in) = 0;

  virtual size_t MemoryMap(std::string filename) = 0;

  virtual size_t StorageSize() = 0;

  int64_t BinarySearch(uint64_t val, uint64_t s, uint64_t e, bool flag) {
    int64_t sp = s;
    int64_t ep = e;
    uint64_t m;
    while (sp <= ep) {
      m = (sp + ep) / 2;
      uint64_t npa_val = operator[](m);
      if (npa_val == val)
        return m;
      else if (val < npa_val)
        ep = m - 1;
      else
        sp = m + 1;
    }

    return flag ? ep : sp;
  }

 protected:

  bool CompareDataBitmap(bitmap_t *data_bitmap, uint64_t i, uint64_t j,
                           uint64_t k) {
    uint32_t sigma_bits = SuccinctUtils::int_log_2(sigma_size_);
    for (uint64_t p = i; p < i + k; p++)
      if (SuccinctBase::lookup_bitmap_array(data_bitmap, p, sigma_bits)
          != SuccinctBase::lookup_bitmap_array(data_bitmap, j++, sigma_bits))
        return false;

    return true;
  }

  uint64_t ComputeContextValue(bitmap_t *data_bitmap, uint32_t i) {
    uint32_t sigma_bits = SuccinctUtils::int_log_2(sigma_size_ + 1);
    uint64_t val = 0;
    uint64_t max = MIN(i + context_len_, npa_size_);
    for (uint64_t p = i; p < max; p++) {
      val = val * sigma_size_
          + SuccinctBase::lookup_bitmap_array(data_bitmap, p, sigma_bits);
    }

    if (max < i + context_len_) {
      for (uint64_t p = 0; p < (i + context_len_) % npa_size_; p++) {
        val = val * sigma_size_
            + SuccinctBase::lookup_bitmap_array(data_bitmap, p, sigma_bits);
      }
    }

    return val;
  }

  // Protected data structures
  NPAEncodingScheme encoding_scheme_;
  uint64_t npa_size_;
  uint64_t sigma_size_;
  uint32_t context_len_;
  uint32_t sampling_rate_;
  SuccinctAllocator s_allocator_;

 public:
  // Public data structures
  std::map<uint64_t, uint64_t> contexts_;
  std::vector<uint64_t> row_offsets_;
  std::vector<uint64_t> col_offsets_;
  std::vector<uint64_t> *row_nec_;
  std::vector<uint64_t> *col_nec_;
  std::vector<uint64_t> *cell_offsets_;

};

#endif
