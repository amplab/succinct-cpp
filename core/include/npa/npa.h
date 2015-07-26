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

 protected:
  NPAEncodingScheme npa_scheme;
  uint64_t npa_size;
  uint64_t sigma_size;
  uint32_t context_len;
  uint32_t sampling_rate;

  SuccinctAllocator s_allocator;

 public:
  // Constructor
  NPA(uint64_t npa_size, uint64_t sigma_size, uint32_t context_len,
      uint32_t sampling_rate, NPAEncodingScheme npa_scheme,
      SuccinctAllocator &s_allocator) {

    this->npa_size = npa_size;
    this->sigma_size = sigma_size;
    this->context_len = context_len;
    this->sampling_rate = sampling_rate;

    this->col_nec = NULL;
    this->row_nec = NULL;
    this->cell_offsets = NULL;

    this->npa_scheme = npa_scheme;

    this->s_allocator = s_allocator;
  }
  // Virtual destructor
  virtual ~NPA() {
  }
  ;

  // Returns the encoding scheme for the NPA
  NPAEncodingScheme get_encoding_scheme() {
    return npa_scheme;
  }

  // Get size of NPA
  uint64_t get_npa_size() {
    return npa_size;
  }

  uint64_t get_context_len() {
    return context_len;
  }

  uint32_t get_sampling_rate() {
    return sampling_rate;
  }

  // Encode NPA based on the encoding scheme
  virtual void encode(bitmap_t *data_bitmap, bitmap_t *compactSA,
                      bitmap_t *compactISA) = 0;

  // Access element at index i
  virtual uint64_t operator[](uint64_t i) = 0;

  // Access element at index i
  virtual uint64_t at(uint64_t i) {
    return operator[](i);
  }

  virtual size_t serialize(std::ostream& out) = 0;

  virtual size_t deserialize(std::istream& in) = 0;

  virtual size_t memorymap(std::string filename) = 0;

  virtual size_t storage_size() = 0;

  int64_t binary_search_npa(uint64_t val, uint64_t s, uint64_t e, bool flag) {
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

  bool compare_data_bitmap(bitmap_t *data_bitmap, uint64_t i, uint64_t j,
                           uint64_t k) {
    uint32_t sigma_bits = SuccinctUtils::int_log_2(sigma_size);
    for (uint64_t p = i; p < i + k; p++)
      if (SuccinctBase::lookup_bitmap_array(data_bitmap, p, sigma_bits)
          != SuccinctBase::lookup_bitmap_array(data_bitmap, j++, sigma_bits))
        return false;

    return true;
  }

  uint64_t compute_context_val(bitmap_t *data_bitmap, uint32_t i) {

    uint32_t sigma_bits = SuccinctUtils::int_log_2(sigma_size + 1);
    uint64_t val = 0;
    uint64_t max = MIN(i + context_len, npa_size);
    for (uint64_t p = i; p < max; p++) {
      val = val * sigma_size
          + SuccinctBase::lookup_bitmap_array(data_bitmap, p, sigma_bits);
    }

    if (max < i + context_len) {
      for (uint64_t p = 0; p < (i + context_len) % npa_size; p++) {
        val = val * sigma_size
            + SuccinctBase::lookup_bitmap_array(data_bitmap, p, sigma_bits);
      }
    }

    return val;
  }

 public:
  // Public data structures
  std::map<uint64_t, uint64_t> contexts;
  std::vector<uint64_t> row_offsets;
  std::vector<uint64_t> col_offsets;
  std::vector<uint64_t> *row_nec;
  std::vector<uint64_t> *col_nec;
  std::vector<uint64_t> *cell_offsets;

};

#endif
