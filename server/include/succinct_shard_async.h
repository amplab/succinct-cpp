#ifndef SUCCINCT_SHARD_ASYNC_H_
#define SUCCINCT_SHARD_ASYNC_H_

#include <future>

#include "succinct_shard.h"

class SuccinctShardAsync : public SuccinctShard {
 public:
  SuccinctShardAsync(uint32_t id, std::string datafile, SuccinctMode s_mode =
                         SuccinctMode::CONSTRUCT_IN_MEMORY,
                     uint32_t sa_sampling_rate = 32,
                     uint32_t isa_sampling_rate = 32,
                     uint32_t npa_sampling_rate = 128,
                     SamplingScheme sa_sampling_scheme =
                         SamplingScheme::FLAT_SAMPLE_BY_INDEX,
                     SamplingScheme isa_sampling_scheme =
                         SamplingScheme::FLAT_SAMPLE_BY_INDEX,
                     NPA::NPAEncodingScheme npa_encoding_scheme =
                         NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED,
                     uint32_t context_len = 3, uint32_t sampling_range = 1024)
      : SuccinctShard(id, datafile, s_mode, sa_sampling_rate, isa_sampling_rate,
                      npa_sampling_rate, sa_sampling_scheme,
                      isa_sampling_scheme, npa_encoding_scheme, context_len,
                      sampling_range) {
  }

  // Async get
  std::future<std::string> GetAsync(int64_t key) {
    return std::async(std::launch::async, Get, key);
  }

  // Async search
  std::future<std::set<int64_t>> SearchAsync(const std::string& str) {
    return std::async(std::launch::async, [&] {
      std::set<int64_t> results;
      Search(results, str);
      return results;
    });
  }

  // TODO: Make more API calls async
};
#endif
