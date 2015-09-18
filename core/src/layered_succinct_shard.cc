#include "layered_succinct_shard.h"

LayeredSuccinctShard::LayeredSuccinctShard(
    uint32_t id, std::string datafile, SuccinctMode s_mode,
    uint32_t sa_sampling_rate, uint32_t isa_sampling_rate,
    uint32_t sampling_range, bool opportunistic, uint32_t npa_sampling_rate,
    NPA::NPAEncodingScheme npa_encoding_scheme, uint32_t context_len)
    : SuccinctShard(
        id,
        datafile,
        s_mode,
        sa_sampling_rate,
        isa_sampling_rate,
        npa_sampling_rate,
        opportunistic ?
            SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX :
            SamplingScheme::LAYERED_SAMPLE_BY_INDEX,
        opportunistic ?
            SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX :
            SamplingScheme::LAYERED_SAMPLE_BY_INDEX,
        npa_encoding_scheme, context_len, sampling_range) {
  this->opportunistic = opportunistic;
}

size_t LayeredSuccinctShard::RemoveLayer(uint32_t layer_id) {
  size_t size = 0;
  if (!opportunistic) {
    size += ((LayeredSampledArray *) sa_)->DestroyLayer(layer_id);
    size += ((LayeredSampledArray *) isa_)->DestroyLayer(layer_id);
  } else {
    size += ((OpportunisticLayeredSampledArray *) sa_)->DestroyLayer(layer_id);
    size += ((OpportunisticLayeredSampledArray *) isa_)->DestroyLayer(layer_id);
  }
  return size;
}

size_t LayeredSuccinctShard::ReconstructLayer(uint32_t layer_id) {
  size_t size = 0;
  if (!opportunistic) {
    size += ((LayeredSampledSA *) sa_)->ReconstructLayer(layer_id);
    size += ((LayeredSampledISA *) isa_)->ReconstructLayer(layer_id);
  } else {
    size += ((OpportunisticLayeredSampledSA *) sa_)->ReconstructLayer(layer_id);
    size += ((OpportunisticLayeredSampledISA *) isa_)->ReconstructLayer(
        layer_id);
  }
  return size;
}

void LayeredSuccinctShard::Get(std::string& result, int64_t key) {

  if (!opportunistic) {
    LayeredSampledISA *ISA_lay = (LayeredSampledISA *) isa_;
    result = "";
    int64_t pos = GetValueOffsetPos(key);
    if (pos < 0)
      return;
    int64_t start = value_offsets_[pos];
    int64_t end =
        ((size_t) (pos + 1) < value_offsets_.size()) ?
            value_offsets_[pos + 1] : input_size_;
    int64_t len = end - start - 1;
    result.resize(len);
    uint64_t idx = LookupISA(start);
    for (int64_t i = 0; i < len; i++) {
      result[i] = alphabet_[LookupC(idx)];
      uint64_t next_pos = start + i + 1;
      if (ISA_lay->IsSampled(next_pos)) {
        idx = LookupISA(next_pos);
      } else {
        idx = LookupNPA(idx);
      }
    }
    return;
  }

  OpportunisticLayeredSampledISA *ISA_opp =
      (OpportunisticLayeredSampledISA *) isa_;

  result = "";
  int64_t pos = GetValueOffsetPos(key);
  if (pos < 0)
    return;
  int64_t start = value_offsets_[pos];
  int64_t end =
      ((size_t) (pos + 1) < value_offsets_.size()) ?
          value_offsets_[pos + 1] : input_size_;
  int64_t len = end - start - 1;
  result.resize(len);
  uint64_t idx = LookupISA(start);
  ISA_opp->Store(start, idx);
  for (int64_t i = 0; i < len; i++) {
    result[i] = alphabet_[LookupC(idx)];
    uint64_t next_pos = start + i + 1;
    if (ISA_opp->IsSampled(next_pos)) {
      idx = LookupISA(next_pos);
    } else {
      idx = LookupNPA(idx);
    }
    ISA_opp->Store(next_pos, idx);
  }
}

uint64_t LayeredSuccinctShard::NumSampledValues() {
  if (opportunistic) {
    return ((OpportunisticLayeredSampledISA *) isa_)->GetNumSampledValues();
  }
  return 0;
}

void LayeredSuccinctShard::Access(std::string& result, int64_t key,
                                  int32_t offset, int32_t len) {
  if (!opportunistic) {
    LayeredSampledISA *ISA_lay = (LayeredSampledISA *) isa_;
    result = "";
    int64_t pos = GetValueOffsetPos(key);
    if (pos < 0)
      return;
    int64_t start = value_offsets_[pos] + offset;
    result.resize(len);
    uint64_t idx = LookupISA(start);
    for (int64_t i = 0; i < len; i++) {
      result[i] = alphabet_[LookupC(idx)];
      uint64_t next_pos = (start + i + 1) % GetOriginalSize();
      if (ISA_lay->IsSampled(next_pos)) {
        idx = LookupISA(next_pos);
      } else {
        idx = LookupNPA(idx);
      }
    }
    return;
  }

  OpportunisticLayeredSampledISA *ISA_opp =
      (OpportunisticLayeredSampledISA *) isa_;
  result = "";
  int64_t pos = GetValueOffsetPos(key);
  if (pos < 0)
    return;
  int64_t start = value_offsets_[pos] + offset;
  result.resize(len);
  uint64_t idx = LookupISA(start);
  ISA_opp->Store(start, idx);
  for (int64_t i = 0; i < len; i++) {
    result[i] = alphabet_[LookupC(idx)];
    uint64_t next_pos = (start + i + 1) % GetOriginalSize();
    if (ISA_opp->IsSampled(next_pos)) {
      idx = LookupISA(next_pos);
    } else {
      idx = LookupNPA(idx);
    }
    ISA_opp->Store(next_pos, idx);
  }
}

void LayeredSuccinctShard::SerializeStatic(std::string& path,
                                           uint32_t sampling_rate) {

  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    mode_t mode = (mode_t) (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    int ret = mkdir(path.c_str(), mode);
    if (ret != 0) {
      fprintf(stderr, "Path creation failed [%s]\n", path.c_str());
      return;
    }
  }

  fprintf(stderr, "Output path = %s\n", path.c_str());

  SuccinctUtils::CopyFile(succinct_path_ + "/metadata", path + "/metadata");
  SuccinctUtils::CopyFile(succinct_path_ + "/keyval", path + "/keyval");
  SuccinctUtils::CopyFile(succinct_path_ + "/npa", path + "/npa");

  uint8_t data_bits = SuccinctUtils::IntegerLog2(input_size_ + 1);
  uint64_t data_size = (input_size_ / sampling_rate) + 1;

  fprintf(stderr, "data_bits = %u, data_size = %llu\n", data_bits, data_size);

  std::ofstream sa(path + "/sa");
  std::ofstream isa(path + "/isa");

  // Write metadata to SA
  sa.write(reinterpret_cast<const char *>(&data_size), sizeof(uint64_t));
  sa.write(reinterpret_cast<const char *>(&data_bits), sizeof(uint8_t));
  sa.write(reinterpret_cast<const char *>(&input_size_), sizeof(uint64_t));
  sa.write(reinterpret_cast<const char *>(&sampling_rate), sizeof(uint32_t));

  // Write metadata to ISAs
  isa.write(reinterpret_cast<const char *>(&data_size), sizeof(uint64_t));
  isa.write(reinterpret_cast<const char *>(&data_bits), sizeof(uint8_t));
  isa.write(reinterpret_cast<const char *>(&input_size_), sizeof(uint64_t));
  isa.write(reinterpret_cast<const char *>(&sampling_rate), sizeof(uint32_t));

  Bitmap *buf = new Bitmap;

  SuccinctBase::InitBitmap(&buf, data_size * data_bits, s_allocator);
  for (uint64_t i = 0; i < input_size_; i += sampling_rate) {
    SuccinctBase::SetBitmapArray(&buf, i / sampling_rate, sa_->at(i),
                                 data_bits);
  }
  SuccinctBase::SerializeBitmap(buf, sa);

  SuccinctBase::ClearBitmap(&buf, s_allocator);
  for (uint64_t i = 0; i < input_size_; i += sampling_rate) {
    SuccinctBase::SetBitmapArray(&buf, i / sampling_rate, isa_->at(i),
                                 data_bits);
  }
  SuccinctBase::SerializeBitmap(buf, isa);

  sa.close();
  isa.close();

}
