#ifndef LAYERED_SUCCINCT_SHARD_HPP
#define LAYERED_SUCCINCT_SHARD_HPP

#include "succinct_shard.h"

class LayeredSuccinctShard : public SuccinctShard {
 public:
  LayeredSuccinctShard(uint32_t id, std::string datafile, SuccinctMode s_mode =
                           SuccinctMode::CONSTRUCT_IN_MEMORY,
                       uint32_t sa_sampling_rate = 32,
                       uint32_t isa_sampling_rate = 32,
                       uint32_t sampling_range = 1024, bool opportunistic =
                           false,
                       uint32_t npa_sampling_rate = 128,
                       NPA::NPAEncodingScheme npa_encoding_scheme =
                           NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED,
                       uint32_t context_len = 3);

  virtual ~LayeredSuccinctShard() {
  }

  size_t RemoveLayer(uint32_t layer_id);

  size_t ReconstructLayer(uint32_t layer_id);

  size_t ReconstructLayerParallel(uint32_t layer_id, uint32_t num_threads);

  void Get(std::string &result, int64_t key);

  void Access(std::string& result, int64_t key, int32_t offset, int32_t len);

  uint64_t NumSampledValues();

  void SerializeStatic(std::string& path, uint32_t sampling_rate);

 private:
  bool opportunistic;
};

#endif
