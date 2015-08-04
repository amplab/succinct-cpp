#ifndef OPPORTUNISTIC_LAYERED_SAMPLED_ARRAY_HPP
#define OPPORTUNISTIC_LAYERED_SAMPLED_ARRAY_HPP

#include "layered_sampled_array.h"

class OpportunisticLayeredSampledArray : public LayeredSampledArray {
#define IS_LAYER_VAL_SAMPLED(l, i)  ACCESSBIT(is_layer_value_sampled_[(l)], (i))
#define SET_LAYER_VAL_SAMPLED(l, i) SETBITVAL(is_layer_value_sampled_[(l)], (i))
#define MARK_FOR_CREATION(i)        SETBIT((layer_to_create_), (i))
#define UNMARK_FOR_CREATION(i)      CLEARBIT((layer_to_create_), (i))
#define IS_MARKED_FOR_CREATION(i)   GETBIT((layer_to_create_), (i))

 public:
  OpportunisticLayeredSampledArray(uint32_t target_sampling_rate,
                                   uint32_t base_sampling_rate, bitmap_t *sa,
                                   uint64_t sa_size,
                                   SuccinctAllocator &succinct_allocator)
      : LayeredSampledArray(target_sampling_rate, base_sampling_rate, sa,
                            sa_size, succinct_allocator) {
    this->is_layer_value_sampled_ = new bitmap_t*[this->num_layers_];
    this->layer_to_create_ = 0;
    this->num_sampled_values_ = 0;
    for (uint32_t i = 0; i < this->num_layers_; i++) {
      this->is_layer_value_sampled_[i] = new bitmap_t;
      uint32_t layer_sampling_rate = (1 << i) * target_sampling_rate;
      layer_sampling_rate =
          (i == (this->num_layers_ - 1)) ?
              layer_sampling_rate : layer_sampling_rate * 2;
      uint64_t num_entries = (sa_size / layer_sampling_rate) + 1;
      this->num_sampled_values_ += num_entries;
      SuccinctBase::InitBitmapSet(&is_layer_value_sampled_[i], num_entries,
                                    succinct_allocator);
    }
  }

  OpportunisticLayeredSampledArray(uint32_t target_sampling_rate,
                                   uint32_t base_sampling_rate,
                                   SuccinctAllocator &succinct_allocator)
      : LayeredSampledArray(target_sampling_rate, base_sampling_rate,
                            succinct_allocator) {
    this->is_layer_value_sampled_ = NULL;
    this->layer_to_create_ = 0;
    this->num_sampled_values_ = 0;
  }

  virtual inline uint64_t GetLayerLeq(Layer *l, int64_t i) {
    int32_t layer_offset = (i / target_sampling_rate_) % sampling_range_;
    uint64_t n_hops = (i % target_sampling_rate_);
    i -= n_hops;
    int64_t l_idx = (i / base_sampling_rate_) * count_[layer_[layer_offset]]
        + layer_index_[layer_offset];
    while (!EXISTS_LAYER(layer_[layer_offset])
        || !IS_LAYER_VAL_SAMPLED(layer_[layer_offset], l_idx)) {
      layer_offset--;
      i -= target_sampling_rate_;
      n_hops += target_sampling_rate_;
      if (layer_offset < 0)
        layer_offset += num_layers_;
      if (i < 0)
        i += original_size_;
      l_idx = (i / base_sampling_rate_) * count_[layer_[layer_offset]]
          + layer_index_[layer_offset];
    }
    l->layer_id = layer_[layer_offset];
    l->layer_idx = l_idx;
    return n_hops;
  }

  virtual bool IsSampled(uint64_t i) {
    uint32_t layer_offset = (i / target_sampling_rate_) % sampling_range_;
    uint32_t layer_id = layer_[layer_offset];
    uint64_t l_idx = (i / base_sampling_rate_) * count_[layer_id]
        + layer_index_[layer_offset];
    return (i % target_sampling_rate_ == 0) && EXISTS_LAYER(layer_id)
        && IS_LAYER_VAL_SAMPLED(layer_id, l_idx);
  }

  bool Store(uint64_t i, uint64_t val) {
    if (i % target_sampling_rate_ == 0) {
      Layer l;
      GetLayer(&l, i);
      if (IS_MARKED_FOR_CREATION(l.layer_id) &&
      !IS_LAYER_VAL_SAMPLED(l.layer_id, l.layer_idx)) {
        SuccinctBase::SetBitmapArray(&layer_data_[l.layer_id], l.layer_idx,
                                       val, data_bits_);
        SET_LAYER_VAL_SAMPLED(l.layer_id, l.layer_idx);
        this->num_sampled_values_++;
        return true;
      }
    }
    return false;
  }

  bool IsIndexMarkedForCreation(uint64_t i) {
    if (i % target_sampling_rate_ != 0)
      return false;
    Layer l;
    GetLayer(&l, i);
    return IS_MARKED_FOR_CREATION(l.layer_id)
        && !IS_LAYER_VAL_SAMPLED(l.layer_id, l.layer_idx);
  }

  void StoreWithoutCheck(uint64_t i, uint64_t val) {
    Layer l;
    GetLayer(&l, i);
    SuccinctBase::SetBitmapArray(&layer_data_[l.layer_id], l.layer_idx, val,
                                   data_bits_);
    SET_LAYER_VAL_SAMPLED(l.layer_id, l.layer_idx);
    this->num_sampled_values_++;
  }

  virtual size_t DestroyLayer(uint32_t layer_id) {
    size_t size = 0;
    if (EXISTS_LAYER(layer_id)) {
      if (IS_MARKED_FOR_CREATION(layer_id)) {
        UNMARK_FOR_CREATION(layer_id);
      }
      DELETE_LAYER(layer_id);
      size = layer_data_[layer_id]->size;
      for (uint64_t i = 0; i < is_layer_value_sampled_[layer_id]->size; i++) {
        this->num_sampled_values_ -= IS_LAYER_VAL_SAMPLED(layer_id, i);
      }
      SuccinctBase::ClearBitmap(&is_layer_value_sampled_[layer_id],
                                 succinct_allocator_);
      for (uint64_t i = 0; i < is_layer_value_sampled_[layer_id]->size; i++) {
        assert(!IS_LAYER_VAL_SAMPLED(layer_id, i));
      }
      usleep(10000);                      // TODO: I don't like this!
      SuccinctBase::DestroyBitmap(&layer_data_[layer_id], succinct_allocator_);
      layer_data_[layer_id] = NULL;
    }
    return size;
  }

  virtual size_t ReconstructLayer(uint32_t layer_id) {
    size_t add_size = 0;
    if (!EXISTS_LAYER(layer_id)) {
      layer_data_[layer_id] = new bitmap_t;
      uint32_t layer_sampling_rate = (1 << layer_id) * target_sampling_rate_;
      layer_sampling_rate =
          (layer_id == (num_layers_ - 1)) ?
              layer_sampling_rate : layer_sampling_rate * 2;
      uint64_t num_entries = (original_size_ / layer_sampling_rate) + 1;
      SuccinctBase::InitBitmap(&layer_data_[layer_id],
                                num_entries * data_bits_, succinct_allocator_);
      add_size = layer_data_[layer_id]->size;
      CREATE_LAYER(layer_id);
      if (!IS_MARKED_FOR_CREATION(layer_id)) {
        MARK_FOR_CREATION(layer_id);
      }
    }
    return add_size;
  }

  virtual size_t Deserialize(std::istream& in) {
    size_t in_size = LayeredSampledArray::Deserialize(in);
    this->is_layer_value_sampled_ = new bitmap_t*[this->num_layers_];
    for (uint32_t i = 0; i < this->num_layers_; i++) {
      this->is_layer_value_sampled_[i] = new bitmap_t;
      uint32_t layer_sampling_rate = (1 << i) * target_sampling_rate_;
      layer_sampling_rate =
          (i == (this->num_layers_ - 1)) ?
              layer_sampling_rate : layer_sampling_rate * 2;
      uint64_t num_entries = (original_size_ / layer_sampling_rate) + 1;
      SuccinctBase::InitBitmapSet(&is_layer_value_sampled_[i], num_entries,
                                    succinct_allocator_);
      this->num_sampled_values_ += num_entries;
    }
    return in_size;
  }

  uint64_t GetNumSampledValues() {
    return this->num_sampled_values_;
  }

 private:
  bitmap_t **is_layer_value_sampled_;
  uint64_t layer_to_create_;

  std::atomic<uint64_t> num_sampled_values_;
};
#endif
