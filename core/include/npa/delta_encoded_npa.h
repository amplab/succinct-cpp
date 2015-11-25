#ifndef DELTA_ENCODED_NPA_H
#define DELTA_ENCODED_NPA_H

#include <thread>

#include "utils/succinct_utils.h"
#include "utils/definitions.h"
#include "utils/array_stream.h"
#include "utils/thread_pool.h"
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
  void Encode(std::string& isa_file, std::vector<uint64_t>& col_offsets,
              std::string npa_file) {

    // Initialize Auxiliary NPA structures
    col_offsets_ = col_offsets;

    // Get all NPA values
    int64_t *lNPA = new int64_t[npa_size_]();
    uint64_t first_idx, cur_idx, nxt_idx, num_elements_per_chunk;
    std::thread constructor_thread[8];

    ArrayStream isa_stream(isa_file);
    first_idx = isa_stream.Get();
    num_elements_per_chunk = SuccinctUtils::NumBlocks(npa_size_, 8);
    for (uint8_t i = 0; i < 8; i++) {
      uint64_t remaining_elements =
          (i * num_elements_per_chunk >= npa_size_) ?
              0 : npa_size_ - i * num_elements_per_chunk;
      uint64_t num_elements = SuccinctUtils::Min(remaining_elements,
                                                 num_elements_per_chunk);
      constructor_thread[i] = std::thread(&DeltaEncodedNPA::ConstructNPAChunk, lNPA,
                                    isa_file, i * num_elements_per_chunk,
                                    num_elements, (i == 7) ? first_idx : -1ULL);
    }

    for (uint8_t i = 0; i < 8; i++) {
      constructor_thread[i].join();
    }

    isa_stream.CloseAndRemove();

    SuccinctUtils::WriteToFile(lNPA, npa_size_, npa_file);
    delete[] lNPA;

    del_npa_ = new DeltaEncodedVector[sigma_size_];
    ThreadPool pool(8);
    for (uint64_t i = 0; i < col_offsets_.size(); i++) {
      uint64_t start_offset = col_offsets_[i];
      uint64_t end_offset = (i < col_offsets_.size() - 1) ? col_offsets_[i + 1] : npa_size_;
      pool.Enqueue([&, start_offset, end_offset, i] {
        EncodeNPAChunk(&(this->del_npa_[i]), npa_file, start_offset, end_offset);
      });
    }
    pool.ShutDown();
    remove(npa_file.c_str());
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
    tot_size += SuccinctBase::VectorSize(col_offsets_);

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

    // Output col_offsets
    out_size += SuccinctBase::SerializeVector(col_offsets_, out);

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

    // Read coloffsets
    in_size += SuccinctBase::DeserializeVector(col_offsets_, in);

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

    // Read coloffsets
    data += SuccinctBase::MemoryMapVector(col_offsets_, data);

    // Read delta encoded vectors
    del_npa_ = new DeltaEncodedVector[sigma_size_];
    for (uint64_t i = 0; i < sigma_size_; i++) {
      data += MemoryMapDeltaEncodedVector(&del_npa_[i], data);
    }

    return data - data_beg;
  }

 protected:
  DeltaEncodedVector *del_npa_;

 private:
  static void ConstructNPAChunk(int64_t *lNPA, std::string isa_file,
                                uint64_t start_pos, uint64_t n_elems,
                                int64_t first_idx) {
    // ISA Stream is configured to start reading from correct position
    uint64_t cur_idx, nxt_idx;
    ArrayStream isa_stream(isa_file, start_pos);
    cur_idx = isa_stream.Get();

    for (uint64_t i = 0; i < n_elems; i++) {
      nxt_idx = isa_stream.Get();
      lNPA[cur_idx] = nxt_idx;
      cur_idx = nxt_idx;
    }

    if (first_idx > 0) {
      lNPA[cur_idx] = first_idx;
    }
    isa_stream.Close();
  }

  void EncodeNPAChunk(DeltaEncodedVector *dv, std::string npa_file, uint64_t start_offset,
                             uint64_t end_offset) {
    ArrayStream npa_stream(npa_file, start_offset);
    std::vector<uint64_t> column;
    for (uint64_t j = start_offset; j < end_offset; j++) {
      column.push_back(npa_stream.Get());
    }
    assert(column.size() > 0);
    CreateDeltaEncodedVector(dv, column);
    npa_stream.Close();
  }

};

#endif
