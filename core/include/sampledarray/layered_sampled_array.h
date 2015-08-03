#ifndef LAYERED_SAMPLED_ARRAY_HPP
#define LAYERED_SAMPLED_ARRAY_HPP

#include <cstdio>
#include <map>
#include <unistd.h>
#include <atomic>

#include "npa/npa.h"
#include "succinct_base.h"
#include "utils/assertions.h"
#include "utils/definitions.h"
#include "utils/succinct_allocator.h"
#include "utils/succinct_utils.h"
#include "sampled_array.h"

#define CREATE_LAYER(i)    SETBIT((this->layer_map_), (i))
#define DELETE_LAYER(i)    CLEARBIT((this->layer_map_), (i))
#define EXISTS_LAYER(i)    GETBIT((this->layer_map_), (i))

class LayeredSampledArray : public SampledArray {
 public:
  typedef SuccinctBase::BitMap bitmap_t;
  typedef SuccinctBase::Dictionary dictionary_t;
  typedef struct {
    uint32_t layer_id;
    uint64_t layer_idx;
  } Layer;

  LayeredSampledArray(uint32_t target_sampling_rate,
                      uint32_t base_sampling_rate, bitmap_t *sa, uint64_t n,
                      SuccinctAllocator &succinct_allocator)
      : SampledArray(SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {

    assert(target_sampling_rate < base_sampling_rate);
    assert(ISPOWOF2(target_sampling_rate));
    assert(ISPOWOF2(base_sampling_rate));
    this->target_sampling_rate_ = target_sampling_rate;
    this->base_sampling_rate_ = base_sampling_rate;
    this->sampling_range_ = base_sampling_rate / target_sampling_rate;
    this->num_layers_ = SuccinctUtils::IntegerLog2(sampling_range_) + 1;
    this->layer_ = new uint32_t[sampling_range_];
    this->layer_index_ = new uint64_t[sampling_range_];

    for (uint32_t i = 0; i < sampling_range_; i++) {
      layer_[i] = SuccinctUtils::IntegerLog2(GCD(i, sampling_range_));
      layer_index_[i] = count_[layer_[i]];
      count_[layer_[i]]++;
    }

    this->layer_map_ = 0;
    this->layer_data_ = new bitmap_t*[this->num_layers_];
    this->original_size_ = n;
    this->data_bits_ = SuccinctUtils::IntegerLog2(n + 1);

    for (uint32_t i = 0; i < this->num_layers_; i++) {
      CREATE_LAYER(i);
      layer_data_[i] = new bitmap_t;
      uint32_t layer_sampling_rate = (1 << i) * target_sampling_rate;
      layer_sampling_rate =
          (i == (num_layers_ - 1)) ?
              layer_sampling_rate : layer_sampling_rate * 2;
      uint64_t num_entries = (n / layer_sampling_rate) + 1;
      SuccinctBase::init_bitmap(&layer_data_[i], num_entries * data_bits_,
                                succinct_allocator);
    }

    this->succinct_allocator_ = succinct_allocator;
  }

  LayeredSampledArray(uint32_t target_sampling_rate,
                      uint32_t base_sampling_rate,
                      SuccinctAllocator &succinct_allocator)
      : SampledArray(SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {
    assert(target_sampling_rate < base_sampling_rate);
    assert(ISPOWOF2(target_sampling_rate));
    assert(ISPOWOF2(base_sampling_rate));
    target_sampling_rate_ = target_sampling_rate;
    base_sampling_rate_ = base_sampling_rate;
    sampling_range_ = base_sampling_rate / target_sampling_rate;
    num_layers_ = SuccinctUtils::IntegerLog2(sampling_range_) + 1;
    layer_ = new uint32_t[sampling_range_];
    layer_index_ = new uint64_t[sampling_range_];

    for (uint32_t i = 0; i < sampling_range_; i++) {
      layer_[i] = SuccinctUtils::IntegerLog2(GCD(i, sampling_range_));
      layer_index_[i] = count_[layer_[i]];
      count_[layer_[i]]++;
    }

    layer_map_ = 0;
    layer_data_ = NULL;
    original_size_ = 0;
    data_bits_ = 0;
    succinct_allocator_ = succinct_allocator;
  }

  inline void GetLayer(Layer *l, uint64_t i) {
    uint32_t layer_offset = (i / target_sampling_rate_) % sampling_range_;
    l->layer_id = layer_[layer_offset];
    l->layer_idx = (i / base_sampling_rate_) * count_[l->layer_id]
        + layer_index_[layer_offset];
  }

  virtual inline uint64_t GetLayerLeq(Layer *l, int64_t i) {
    int32_t layer_offset = (i / target_sampling_rate_) % sampling_range_;
    uint64_t n_hops = (i % target_sampling_rate_);
    i -= n_hops;
    while (!EXISTS_LAYER(layer_[layer_offset])) {
      layer_offset--;
      i -= target_sampling_rate_;
      n_hops += target_sampling_rate_;
      if (layer_offset < 0)
        layer_offset += num_layers_;
      if (i < 0)
        i += original_size_;
    }
    l->layer_id = layer_[layer_offset];
    l->layer_idx = (i / base_sampling_rate_) * count_[l->layer_id]
        + layer_index_[layer_offset];
    return n_hops;
  }

  inline uint32_t GetBaseSamplingRate() {
    return base_sampling_rate_;
  }

  inline uint32_t GetTargetSamplingRate() {
    return target_sampling_rate_;
  }

  inline uint32_t GetSamplingRate() {
    return GetTargetSamplingRate();
  }

  inline uint32_t GetSamplingRange() {
    return sampling_range_;
  }

  inline uint64_t GetOriginalSize() {
    return original_size_;
  }

  inline uint8_t GetDataBits() {
    return data_bits_;
  }

  virtual bool IsSampled(uint64_t i) {
    return (i % target_sampling_rate_ == 0)
        && EXISTS_LAYER(layer_[(i / target_sampling_rate_) % sampling_range_]);
  }

  uint64_t GetSampleAt(uint64_t i) {
    i *= target_sampling_rate_;

    Layer l;
    GetLayer(&l, i);
    return SuccinctBase::lookup_bitmap_array(layer_data_[l.layer_id],
                                             l.layer_idx, data_bits_);
  }

  virtual size_t DestroyLayer(uint32_t layer_id) {
    size_t size = 0;
    if (EXISTS_LAYER(layer_id)) {
      DELETE_LAYER(layer_id);
      size = layer_data_[layer_id]->size;
      usleep(10000);                      // TODO: This is very hacky
      SuccinctBase::destroy_bitmap(&layer_data_[layer_id], succinct_allocator_);
      layer_data_[layer_id] = NULL;
    }
    return size;
  }

  virtual size_t ReconstructLayer(uint32_t layer_id) {
    size_t size = 0;
    if (!EXISTS_LAYER(layer_id)) {
      layer_data_[layer_id] = new bitmap_t;
      uint32_t layer_sampling_rate = (1 << layer_id) * target_sampling_rate_;
      layer_sampling_rate =
          (layer_id == (num_layers_ - 1)) ?
              layer_sampling_rate : layer_sampling_rate * 2;
      uint64_t num_entries = (original_size_ / layer_sampling_rate) + 1;
      SuccinctBase::init_bitmap(&layer_data_[layer_id],
                                num_entries * data_bits_, succinct_allocator_);
      uint64_t idx;
      if (layer_id == this->num_layers_ - 1) {
        for (uint64_t i = 0; i < num_entries; i++) {
          idx = i * layer_sampling_rate;
          if (idx > original_size_)
            break;
          SuccinctBase::set_bitmap_array(&layer_data_[layer_id], i, at(idx),
                                         data_bits_);
        }
      } else {
        for (uint64_t i = 0; i < num_entries; i++) {
          idx = i * layer_sampling_rate + (layer_sampling_rate / 2);
          if (idx > original_size_)
            break;
          SuccinctBase::set_bitmap_array(&layer_data_[layer_id], i, at(idx),
                                         data_bits_);
        }
      }
      size = layer_data_[layer_id]->size;
      CREATE_LAYER(layer_id);
    }
    return size;
  }

  virtual uint64_t operator[](uint64_t i) = 0;

  virtual size_t Serialize(std::ostream& out) {
    size_t out_size = 0;

    out.write(reinterpret_cast<const char *>(&layer_map_), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
    out.write(reinterpret_cast<const char *>(&data_bits_), sizeof(uint8_t));
    out_size += sizeof(uint8_t);
    out.write(reinterpret_cast<const char *>(&original_size_),
              sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    for (uint32_t i = 0; i < this->num_layers_; i++) {
      out_size += SuccinctBase::serialize_bitmap(layer_data_[i], out);
    }

    return out_size;
  }

  virtual size_t Deserialize(std::istream& in) {
    size_t in_size = 0;

    in.read(reinterpret_cast<char *>(&layer_map_), sizeof(uint64_t));
    in_size += sizeof(uint64_t);
    in.read(reinterpret_cast<char *>(&data_bits_), sizeof(uint8_t));
    in_size += sizeof(uint8_t);
    in.read(reinterpret_cast<char *>(&original_size_), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    layer_data_ = new bitmap_t*[this->num_layers_];
    for (uint32_t i = 0; i < this->num_layers_; i++) {
      in_size += SuccinctBase::deserialize_bitmap(&layer_data_[i], in);
    }

    return in_size;
  }

  virtual size_t MemoryMap(std::string filename) {
    uint8_t *data, *data_beg;
    data = data_beg = (uint8_t *) SuccinctUtils::MemoryMap(filename);

    layer_map_ = *((uint64_t *) data);
    data += sizeof(uint64_t);
    data_bits_ = *data;
    data += sizeof(uint8_t);
    original_size_ = *((uint64_t *) data);
    data += sizeof(uint64_t);

    layer_data_ = new bitmap_t*[this->num_layers_];
    for (uint32_t i = 0; i < this->num_layers_; i++) {
      data += SuccinctBase::memorymap_bitmap(&layer_data_[i], data);
    }

    return data - data_beg;
  }

  virtual size_t StorageSize() {
    size_t tot_size = sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint64_t);
    tot_size += 4 * sizeof(uint32_t)
        + this->num_layers_ * (sizeof(uint32_t) + sizeof(uint64_t));
    tot_size += this->sampling_range_ * (sizeof(uint32_t) + sizeof(uint64_t));
    tot_size += sizeof(bitmap_t*) * this->num_layers_;
    for (uint32_t i = 0; i < this->num_layers_; i++) {
      tot_size += SuccinctBase::bitmap_size(layer_data_[i]);
    }
    return tot_size;
  }

 protected:
  uint32_t num_layers_;                   // Number of layers
  std::atomic<uint64_t> layer_map_;  // Bitmap indicating which layers are stored
  std::map<uint32_t, uint64_t> count_;  // Count of each layer in the sampling_ranges
  uint32_t *layer_;                       // Sample offset to layer mapping
  uint64_t *layer_index_;           // Sample offset to index into layer mapping
  uint32_t sampling_range_;         // base_sampling_rate / target_sampling_rate
  uint32_t target_sampling_rate_;  // The target sampling rate (i.e., top most layer)
  uint32_t base_sampling_rate_;  // The base sampling rate (i.e, lower most layer)

  bitmap_t **layer_data_;                 // All of the layered data content
  uint8_t data_bits_;                     // Width of each data entry in bits
  uint64_t original_size_;               // Number of elements in original array
  SuccinctAllocator succinct_allocator_;  // Allocator for allocating memory

 private:
  uint32_t GCD(uint32_t first, uint32_t second) {
    return second == 0 ? first : GCD(second, first % second);
  }

};

#endif /* LAYERED_SAMPLED_ARRAY_HPP */
