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

    out_size += SuccinctBase::serialize_bitmap(dv->samples, out);
    out_size += SuccinctBase::serialize_bitmap(dv->deltas, out);
    out_size += SuccinctBase::serialize_bitmap(dv->delta_offsets, out);

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

    in_size += SuccinctBase::deserialize_bitmap(&(dv->samples), in);
    in_size += SuccinctBase::deserialize_bitmap(&(dv->deltas), in);
    in_size += SuccinctBase::deserialize_bitmap(&(dv->delta_offsets), in);

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

    data += SuccinctBase::memorymap_bitmap(&(dv->samples), data);
    data += SuccinctBase::memorymap_bitmap(&(dv->deltas), data);
    data += SuccinctBase::memorymap_bitmap(&(dv->delta_offsets), data);

    return data - data_beg;
  }

  virtual size_t DeltaEncodedVectorSize(DeltaEncodedVector *dv) {
    if (dv == NULL)
      return 0;
    return sizeof(uint8_t) + sizeof(uint8_t)
        + SuccinctBase::bitmap_size(dv->samples)
        + SuccinctBase::bitmap_size(dv->deltas)
        + SuccinctBase::bitmap_size(dv->delta_offsets);
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
  virtual uint64_t LookupDeltaEncodedVector(DeltaEncodedVector *dv, uint64_t i) = 0;

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
    uint64_t k1, k2, k = 0, l_off = 0, c_id, c_val, npa_val, p = 0;

    std::map<uint64_t, uint64_t> context_size;
    std::vector<uint64_t> **table;
    std::vector<uint64_t> sigma_list;

    bool flag;
    uint64_t *sizes, *starts;
    uint64_t last_i = 0;

    logn = SuccinctUtils::IntegerLog2(npa_size_ + 1);

    for (uint64_t i = 0; i < npa_size_; i++) {
      uint64_t c_val = ComputeContextValue(data_bitmap, i);
      contexts_.insert(std::pair<uint64_t, uint64_t>(c_val, 0));
      context_size[c_val]++;
    }

    sizes = new uint64_t[contexts_.size()];
    starts = new uint64_t[contexts_.size()];

    for (std::map<uint64_t, uint64_t>::iterator it = contexts_.begin();
        it != contexts_.end(); it++) {
      sizes[k] = context_size[it->first];
      starts[k] = npa_size_;
      contexts_[it->first] = k++;
    }

    assert(k == contexts_.size());
    context_size.clear();

    Bitmap *E = new Bitmap;
    table = new std::vector<uint64_t>*[k];
    cell_offsets_ = new std::vector<uint64_t>[sigma_size_];
    col_nec_ = new std::vector<uint64_t>[sigma_size_];
    row_nec_ = new std::vector<uint64_t>[k];
    del_npa_ = new DeltaEncodedVector[sigma_size_]();

    for (uint64_t i = 0; i < k; i++) {
      table[i] = new std::vector<uint64_t>[sigma_size_];
    }

    SuccinctBase::init_bitmap(&(E), k * sigma_size_, s_allocator_);

    k1 = SuccinctBase::lookup_bitmap_array(compact_sa, 0, logn);
    c_id = ComputeContextValue(data_bitmap, (k1 + 1) % npa_size_);
    c_val = contexts_[c_id];
    npa_val = SuccinctBase::lookup_bitmap_array(
        compact_isa,
        (SuccinctBase::lookup_bitmap_array(compact_sa, 0, logn) + 1) % npa_size_,
        logn);
    table[c_val][l_off / k].push_back(npa_val);
    starts[c_val] = MIN(starts[c_val], npa_val);

    sigma_list.push_back(npa_val);

    SETBITVAL(E, c_val);
    col_nec_[0].push_back(c_val);
    col_offsets_.push_back(0);
    cell_offsets_[0].push_back(0);

    for (uint64_t i = 1; i < npa_size_; i++) {
      k1 = SuccinctBase::lookup_bitmap_array(compact_sa, i, logn);
      k2 = SuccinctBase::lookup_bitmap_array(compact_sa, i - 1, logn);

      c_id = ComputeContextValue(data_bitmap, (k1 + 1) % npa_size_);
      c_val = contexts_[c_id];

      assert(c_val < k);

      if (!CompareDataBitmap(data_bitmap, k1, k2, 1)) {
        col_offsets_.push_back(i);
        CreateDeltaEncodedVector(&(del_npa_[l_off / k]), sigma_list);
        assert(sigma_list.size() > 0);
        sigma_list.clear();

        l_off += k;
        last_i = i;
        cell_offsets_[l_off / k].push_back(i - last_i);
      } else if (!CompareDataBitmap(data_bitmap, (k1 + 1) % npa_size_,
                                      (k2 + 1) % npa_size_, context_len_)) {
        // Context has changed; mark in L
        cell_offsets_[l_off / k].push_back(i - last_i);
      }

      // If we haven't marked it already, mark current cell as non empty
      if (!ACCESSBIT(E, (l_off + c_val))) {
        SETBITVAL(E, (l_off + c_val));
        col_nec_[l_off / k].push_back(c_val);
      }

      // Obtain actual npa value
      npa_val = SuccinctBase::lookup_bitmap_array(
          compact_isa,
          (SuccinctBase::lookup_bitmap_array(compact_sa, i, logn) + 1)
              % npa_size_,
          logn);
      sigma_list.push_back(npa_val);

      assert(l_off / k < sigma_size_);
      // Push npa value to npa table: note indices. indexed by context value, and l_offs / k
      table[c_val][l_off / k].push_back(npa_val);
      assert(table[c_val][l_off / k].back() == npa_val);
      starts[c_val] = MIN(starts[c_val], npa_val);
    }

    CreateDeltaEncodedVector(&(del_npa_[l_off / k]), sigma_list);
    assert(sigma_list.size() > 0);
    sigma_list.clear();

    for (uint64_t i = 0; i < k; i++) {
      q = 0;
      flag = false;
      for (uint64_t j = 0; j < sigma_size_; j++) {
        if (!flag && ACCESSBIT(E, (i + j * k))) {
          row_offsets_.push_back(p);
          p += table[i][j].size();
          flag = true;
        } else if (ACCESSBIT(E, (i + j * k))) {
          p += table[i][j].size();
        }

        if (ACCESSBIT(E, (i + j * k))) {
          row_nec_[i].push_back(j);
          q++;
        }
      }
    }

    // Clean up
    for (uint64_t i = 0; i < k; i++) {
      for (uint64_t j = 0; j < sigma_size_; j++) {
        if (table[i][j].size()) {
          table[i][j].clear();
        }
      }
      delete[] table[i];
    }
    delete[] table;
    delete[] starts;
    delete[] sizes;
    delete[] E->bitmap;
    delete E;

    context_size.clear();
  }

  // Access element at index i
  virtual uint64_t operator[](uint64_t i) {
    // Get column id
    uint64_t column_id = SuccinctBase::get_rank1(&col_offsets_, i) - 1;
    assert(column_id < sigma_size_);
    uint64_t npa_val = LookupDeltaEncodedVector(&(del_npa_[column_id]),
                                 i - col_offsets_[column_id]);
    return npa_val;
  }

  virtual size_t StorageSize() {
    size_t tot_size = 3 * sizeof(uint64_t) + 2 * sizeof(uint32_t);
    tot_size += sizeof(contexts_.size())
        + contexts_.size() * (sizeof(uint64_t) * 2);
    tot_size += SuccinctBase::vector_size(row_offsets_);
    tot_size += SuccinctBase::vector_size(col_offsets_);

    for (uint64_t i = 0; i < sigma_size_; i++) {
      tot_size += SuccinctBase::vector_size(col_nec_[i]);
    }

    for (uint64_t i = 0; i < contexts_.size(); i++) {
      tot_size += SuccinctBase::vector_size(row_nec_[i]);
    }

    for (uint64_t i = 0; i < sigma_size_; i++) {
      tot_size += SuccinctBase::vector_size(cell_offsets_[i]);
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
    out.write(reinterpret_cast<const char *>(&(encoding_scheme_)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output NPA size
    out.write(reinterpret_cast<const char *>(&(npa_size_)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output sigma_size
    out.write(reinterpret_cast<const char *>(&(sigma_size_)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output context_len
    out.write(reinterpret_cast<const char *>(&(context_len_)), sizeof(uint32_t));
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

    // Output row_offsets
    out_size += SuccinctBase::serialize_vector(row_offsets_, out);

    // Output col_offsets
    out_size += SuccinctBase::serialize_vector(col_offsets_, out);

    // Output neccol
    for (uint64_t i = 0; i < sigma_size_; i++) {
      out_size += SuccinctBase::serialize_vector(col_nec_[i], out);
    }

    // Output necrow
    for (uint64_t i = 0; i < contexts_.size(); i++) {
      out_size += SuccinctBase::serialize_vector(row_nec_[i], out);
    }

    // Output cell offsets
    for (uint64_t i = 0; i < sigma_size_; i++) {
      out_size += SuccinctBase::serialize_vector(cell_offsets_[i], out);
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

    // Read rowoffsets
    in_size += SuccinctBase::deserialize_vector(row_offsets_, in);

    // Read coloffsets
    in_size += SuccinctBase::deserialize_vector(col_offsets_, in);

    // Read neccol
    col_nec_ = new std::vector<uint64_t>[sigma_size_];
    for (uint64_t i = 0; i < sigma_size_; i++) {
      in_size += SuccinctBase::deserialize_vector(col_nec_[i], in);
    }

    // Read necrow
    row_nec_ = new std::vector<uint64_t>[contexts_.size()];
    for (uint64_t i = 0; i < contexts_.size(); i++) {
      in_size += SuccinctBase::deserialize_vector(row_nec_[i], in);
    }

    // Read cell offsets
    cell_offsets_ = new std::vector<uint64_t>[sigma_size_];
    for (uint64_t i = 0; i < sigma_size_; i++) {
      in_size += SuccinctBase::deserialize_vector(cell_offsets_[i], in);
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

    // Read rowoffsets
    data += SuccinctBase::memorymap_vector(row_offsets_, data);

    // Read coloffsets
    data += SuccinctBase::memorymap_vector(col_offsets_, data);

    // Read neccol
    col_nec_ = new std::vector<uint64_t>[sigma_size_];
    for (uint64_t i = 0; i < sigma_size_; i++) {
      data += SuccinctBase::memorymap_vector(col_nec_[i], data);
    }

    // Read necrow
    row_nec_ = new std::vector<uint64_t>[contexts_.size()];
    for (uint64_t i = 0; i < contexts_.size(); i++) {
      data += SuccinctBase::memorymap_vector(row_nec_[i], data);
    }

    // Read cell offsets
    cell_offsets_ = new std::vector<uint64_t>[sigma_size_];
    for (uint64_t i = 0; i < sigma_size_; i++) {
      data += SuccinctBase::memorymap_vector(cell_offsets_[i], data);
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
