#ifndef DELTA_ENCODED_NPA_H
#define DELTA_ENCODED_NPA_H

#include "utils/succinct_utils.h"
#include "utils/definitions.h"
#include "npa.h"

class DeltaEncodedNPA : public NPA {
 public:
  typedef struct {
    Bitmap *samples;
    Bitmap *deltas;
    Bitmap *delta_offsets;

    uint8_t sample_bits;
    uint8_t delta_offset_bits;
  } DeltaEncodedVector;

  virtual size_t SerializeDeltaEncodedVector(DeltaEncodedVector *dv,
                                             std::ostream& out) {

    size_t out_size = 0;

    out.write(reinterpret_cast<const char *>(&(dv->sample_bits)),
              sizeof(uint8_t));
    out_size += sizeof(uint8_t);
    out.write(reinterpret_cast<const char *>(&(dv->delta_offset_bits)),
              sizeof(uint8_t));
    out_size += sizeof(uint8_t);

    out_size += SuccinctBase::SerializeBitmap(dv->samples, out);
    out_size += SuccinctBase::SerializeBitmap(dv->deltas, out);
    out_size += SuccinctBase::SerializeBitmap(dv->delta_offsets, out);

    return out_size;
  }

  virtual size_t DeserializeDeltaEncodedVector(DeltaEncodedVector *dv,
                                               std::istream& in) {
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(&(dv->sample_bits)), sizeof(uint8_t));
    in_size += sizeof(uint8_t);
    in.read(reinterpret_cast<char *>(&(dv->delta_offset_bits)),
            sizeof(uint8_t));
    in_size += sizeof(uint8_t);

    in_size += SuccinctBase::DeserializeBitmap(&(dv->samples), in);
    in_size += SuccinctBase::DeserializeBitmap(&(dv->deltas), in);
    in_size += SuccinctBase::DeserializeBitmap(&(dv->delta_offsets), in);

    return in_size;
  }

  virtual size_t MemoryMapDeltaEncodedVector(DeltaEncodedVector *dv,
                                             uint8_t *buf) {
    uint8_t *data, *data_beg;
    data = data_beg = buf;

    dv->sample_bits = *data;
    data += sizeof(uint8_t);
    dv->delta_offset_bits = *data;
    data += sizeof(uint8_t);

    data += SuccinctBase::MemoryMapBitmap(&(dv->samples), data);
    data += SuccinctBase::MemoryMapBitmap(&(dv->deltas), data);
    data += SuccinctBase::MemoryMapBitmap(&(dv->delta_offsets), data);

    return data - data_beg;
  }

  virtual size_t DeltaEncodedVectorSize(DeltaEncodedVector *dv) {
    if (dv == NULL)
      return 0;
    return sizeof(uint8_t) + sizeof(uint8_t)
        + SuccinctBase::BitmapSize(dv->samples)
        + SuccinctBase::BitmapSize(dv->deltas)
        + SuccinctBase::BitmapSize(dv->delta_offsets);
  }

 protected:

  // Return lower bound integer log (base 2) for n
  static uint32_t LowerLog2(uint64_t n) {
    return SuccinctUtils::IntegerLog2(n + 1) - 1;
  }

  // Create delta encoded vector
  virtual void CreateDeltaEncodedVector(DeltaEncodedVector *dv,
                                        std::vector<uint64_t> &data) = 0;

  // Lookup delta encoded vector
  virtual uint64_t LookupDeltaEncodedVector(DeltaEncodedVector *dv,
                                            uint64_t i) = 0;

 public:
  // Constructor
  DeltaEncodedNPA(uint64_t npa_size, uint64_t sigma_size, uint32_t context_len,
                  uint32_t sampling_rate,
                  NPAEncodingScheme delta_encoding_scheme,
                  SuccinctAllocator &s_allocator)
      : NPA(npa_size, sigma_size, context_len, sampling_rate,
            delta_encoding_scheme, s_allocator) {
    this->del_npa_ = NULL;
  }

  // Virtual destructor
  virtual ~DeltaEncodedNPA() {
  }

  // Encode DeltaEncodedNPA based on the delta encoding scheme
  virtual void Encode(Bitmap *data_bitmap, Bitmap *compact_sa,
                      Bitmap *compact_isa) {
    uint32_t logn, q;
    uint64_t cur_sa, prv_sa, k = 0, l_off = 0, c_id, c_val, npa_val, p = 0;

    std::map<uint64_t, uint64_t> context_size;
    std::vector<uint64_t> sigma_list;

    bool flag;
    uint64_t last_i = 0;

    logn = SuccinctUtils::IntegerLog2(npa_size_ + 1);

    for (uint64_t i = 0; i < npa_size_; i++) {
      uint64_t c_val = ComputeContextValue(data_bitmap, i);
      contexts_.insert(std::pair<uint64_t, uint64_t>(c_val, 0));
    }
    for (std::map<uint64_t, uint64_t>::iterator it = contexts_.begin();
        it != contexts_.end(); it++) {
      contexts_[it->first] = k++;
    }

    assert(k == contexts_.size());

    cell_offsets_ = new std::vector<uint64_t>[sigma_size_];
    del_npa_ = new DeltaEncodedVector[sigma_size_]();

    cur_sa = SuccinctBase::LookupBitmapArray(compact_sa, 0, logn);
    c_id = ComputeContextValue(data_bitmap, (cur_sa + 1) % npa_size_);
    c_val = contexts_[c_id];
    npa_val = SuccinctBase::LookupBitmapArray(
        compact_isa,
        (SuccinctBase::LookupBitmapArray(compact_sa, 0, logn) + 1) % npa_size_,
        logn);

    sigma_list.push_back(npa_val);
    col_offsets_.push_back(0);
    cell_offsets_[0].push_back(0);

    for (uint64_t i = 1; i < npa_size_; i++) {
      cur_sa = SuccinctBase::LookupBitmapArray(compact_sa, i, logn);
      prv_sa = SuccinctBase::LookupBitmapArray(compact_sa, i - 1, logn);

      c_id = ComputeContextValue(data_bitmap, (cur_sa + 1) % npa_size_);
      c_val = contexts_[c_id];

      assert(c_val < k);

      if (!CompareDataBitmap(data_bitmap, cur_sa, prv_sa, 1)) {
        col_offsets_.push_back(i);
        CreateDeltaEncodedVector(&(del_npa_[l_off / k]), sigma_list);
        assert(sigma_list.size() > 0);
        sigma_list.clear();

        l_off += k;
        last_i = i;
        cell_offsets_[l_off / k].push_back(i - last_i);
      } else if (!CompareDataBitmap(data_bitmap, (cur_sa + 1) % npa_size_,
                                    (prv_sa + 1) % npa_size_, context_len_)) {
        // Context has changed; mark in L
        cell_offsets_[l_off / k].push_back(i - last_i);
      }

      // Obtain actual npa value
      npa_val = SuccinctBase::LookupBitmapArray(
          compact_isa,
          (SuccinctBase::LookupBitmapArray(compact_sa, i, logn) + 1)
              % npa_size_,
          logn);
      sigma_list.push_back(npa_val);

      assert(l_off / k < sigma_size_);
    }

    CreateDeltaEncodedVector(&(del_npa_[l_off / k]), sigma_list);
    assert(sigma_list.size() > 0);
    sigma_list.clear();
  }

  // Access element at index i
  virtual uint64_t operator[](uint64_t i) {
    // Get column id
    uint64_t column_id = SuccinctBase::GetRank1(&col_offsets_, i) - 1;
    assert(column_id < sigma_size_);
    uint64_t npa_val = LookupDeltaEncodedVector(&(del_npa_[column_id]),
                                                i - col_offsets_[column_id]);
    return npa_val;
  }

  virtual size_t StorageSize() {
    size_t tot_size = 3 * sizeof(uint64_t) + 2 * sizeof(uint32_t);
    tot_size += sizeof(contexts_.size())
        + contexts_.size() * (sizeof(uint64_t) * 2);
    tot_size += SuccinctBase::VectorSize(row_offsets_);
    tot_size += SuccinctBase::VectorSize(col_offsets_);

    for (uint64_t i = 0; i < sigma_size_; i++) {
      tot_size += SuccinctBase::VectorSize(col_nec_[i]);
    }

    for (uint64_t i = 0; i < contexts_.size(); i++) {
      tot_size += SuccinctBase::VectorSize(row_nec_[i]);
    }

    for (uint64_t i = 0; i < sigma_size_; i++) {
      tot_size += SuccinctBase::VectorSize(cell_offsets_[i]);
    }

    for (uint64_t i = 0; i < sigma_size_; i++) {
      tot_size += DeltaEncodedVectorSize(&del_npa_[i]);
    }

    return tot_size;
  }

  virtual size_t Serialize(std::ostream& out) {
    size_t out_size = 0;
    typedef std::map<uint64_t, uint64_t>::iterator iterator_t;

    // Output NPA scheme
    out.write(reinterpret_cast<const char *>(&(encoding_scheme_)),
              sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output NPA size
    out.write(reinterpret_cast<const char *>(&(npa_size_)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output sigma_size
    out.write(reinterpret_cast<const char *>(&(sigma_size_)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output context_len
    out.write(reinterpret_cast<const char *>(&(context_len_)),
              sizeof(uint32_t));
    out_size += sizeof(uint32_t);

    // Output sampling rate
    out.write(reinterpret_cast<const char *>(&(sampling_rate_)),
              sizeof(uint32_t));
    out_size += sizeof(uint32_t);

    // Output contexts
    uint64_t context_size = contexts_.size();
    out.write(reinterpret_cast<const char *>(&(context_size)),
              sizeof(uint64_t));
    for (iterator_t it = contexts_.begin(); it != contexts_.end(); ++it) {
      out.write(reinterpret_cast<const char *>(&(it->first)), sizeof(uint64_t));
      out.write(reinterpret_cast<const char *>(&(it->second)),
                sizeof(uint64_t));
    }

    // Output col_offsets
    out_size += SuccinctBase::SerializeVector(col_offsets_, out);

    // Output cell offsets
    for (uint64_t i = 0; i < sigma_size_; i++) {
      out_size += SuccinctBase::SerializeVector(cell_offsets_[i], out);
    }

    // Output delta encoded vectors
    for (uint64_t i = 0; i < sigma_size_; i++) {
      out_size += SerializeDeltaEncodedVector(&del_npa_[i], out);
    }

    return out_size;
  }

  virtual size_t Deserialize(std::istream& in) {
    size_t in_size = 0;

    // Read NPA scheme
    in.read(reinterpret_cast<char *>(&(encoding_scheme_)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    // Read NPA size
    in.read(reinterpret_cast<char *>(&(npa_size_)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    // Read sigma_size
    in.read(reinterpret_cast<char *>(&(sigma_size_)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    // Read context_len
    in.read(reinterpret_cast<char *>(&(context_len_)), sizeof(uint32_t));
    in_size += sizeof(uint32_t);

    // Read sampling rate
    in.read(reinterpret_cast<char *>(&(sampling_rate_)), sizeof(uint32_t));
    in_size += sizeof(uint32_t);

    // Read contexts
    uint64_t context_size;
    in.read(reinterpret_cast<char *>(&(context_size)), sizeof(uint64_t));
    for (uint64_t i = 0; i < context_size; i++) {
      uint64_t first, second;
      in.read(reinterpret_cast<char *>(&(first)), sizeof(uint64_t));
      in.read(reinterpret_cast<char *>(&(second)), sizeof(uint64_t));
      contexts_[first] = second;
    }

    // Read coloffsets
    in_size += SuccinctBase::DeserializeVector(col_offsets_, in);

    // Read cell offsets
    cell_offsets_ = new std::vector<uint64_t>[sigma_size_];
    for (uint64_t i = 0; i < sigma_size_; i++) {
      in_size += SuccinctBase::DeserializeVector(cell_offsets_[i], in);
    }

    // Read delta encoded vectors
    del_npa_ = new DeltaEncodedVector[sigma_size_];
    for (uint64_t i = 0; i < sigma_size_; i++) {
      in_size += DeserializeDeltaEncodedVector(&del_npa_[i], in);
    }

    return in_size;
  }

  virtual size_t MemoryMap(std::string filename) {
    uint8_t *data, *data_beg;
    data = data_beg = (uint8_t *) SuccinctUtils::MemoryMap(filename);

    encoding_scheme_ = (NPAEncodingScheme) (*((uint64_t *) data));
    data += sizeof(uint64_t);
    npa_size_ = *((uint64_t *) data);
    data += sizeof(uint64_t);
    sigma_size_ = *((uint64_t *) data);
    data += sizeof(uint64_t);
    context_len_ = *((uint32_t *) data);
    data += sizeof(uint32_t);
    sampling_rate_ = *((uint32_t *) data);
    data += sizeof(uint32_t);

    // Read contexts
    uint64_t context_size = *((uint64_t *) data);
    data += sizeof(uint64_t);
    for (uint64_t i = 0; i < context_size; i++) {
      uint64_t first = *((uint64_t *) data);
      data += sizeof(uint64_t);
      uint64_t second = *((uint64_t *) data);
      data += sizeof(uint64_t);
      contexts_[first] = second;
    }

    // Read column offsets
    data += SuccinctBase::MemoryMapVector(col_offsets_, data);

    // Read cell offsets
    cell_offsets_ = new std::vector<uint64_t>[sigma_size_];
    for (uint64_t i = 0; i < sigma_size_; i++) {
      data += SuccinctBase::MemoryMapVector(cell_offsets_[i], data);
    }

    // Read delta encoded vectors
    del_npa_ = new DeltaEncodedVector[sigma_size_];
    for (uint64_t i = 0; i < sigma_size_; i++) {
      data += MemoryMapDeltaEncodedVector(&del_npa_[i], data);
    }

    return data - data_beg;
  }

  DeltaEncodedVector *del_npa_;
};

#endif
