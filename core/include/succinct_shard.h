#ifndef SUCCINCT_SHARD_H
#define SUCCINCT_SHARD_H

#include <cstdint>
#include <string>
#include <cstring>
#include <vector>
#include <set>

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "regex/regex.h"
#include "succinct_core.h"

class SuccinctShard : public SuccinctCore {
 public:
  static const int64_t MAX_KEYS = 1L << 32;

  SuccinctShard(uint32_t id, std::string datafile, SuccinctMode s_mode = SuccinctMode::CONSTRUCT_IN_MEMORY,
                uint32_t sa_sampling_rate = 32, uint32_t isa_sampling_rate = 32, uint32_t npa_sampling_rate = 128,
                SamplingScheme sa_sampling_scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX,
                SamplingScheme isa_sampling_scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX,
                NPA::NPAEncodingScheme npa_encoding_scheme = NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED,
                uint32_t context_len = 3, uint32_t sampling_range = 1024);

  SuccinctShard()
      : SuccinctCore() {
    id_ = 0;
    invalid_offsets_ = nullptr;
  }

  ~SuccinctShard() override = default;

  uint32_t GetSASamplingRate();

  uint32_t GetISASamplingRate();

  uint32_t GetNPASamplingRate();

  size_t GetNumKeys();

  virtual void Get(std::string &result, int64_t key);

  void Access(std::string &result, int64_t key, int32_t offset, int32_t len);

  int64_t Count(const std::string &str);

  void Search(std::set<int64_t> &result, const std::string &str);

  int64_t FlatCount(const std::string &str);

  void FlatSearch(std::vector<int64_t> &result, const std::string &str);

  void FlatExtract(std::string &result, int64_t offset, int64_t len);

  void RegexSearch(std::set<std::pair<size_t, size_t>> &result,
                   const std::string &str, bool opt = true);

  void RegexCount(std::vector<size_t> &result, const std::string &str);

  // Serialize succinct data structures
  size_t Serialize(const std::string &path) override;

  // Deserialize succinct data structures
  size_t Deserialize(const std::string &path) override;

  // Memory map succinct data structures
  size_t MemoryMap(const std::string &path) override;

  // Get succinct shard size
  size_t StorageSize() override;

 protected:
  int64_t GetKeyPos(int64_t value_offset);
  int64_t GetValueOffsetPos(int64_t key);

  // std::pair<int64_t, int64_t> get_range_slow(const char *str, uint64_t len);
  std::pair<int64_t, int64_t> GetRange(const char *str, uint64_t len);

  uint64_t ComputeContextValue(const char *str, uint64_t pos);

  std::vector<int64_t> keys_;
  std::vector<int64_t> value_offsets_;
  Bitmap *invalid_offsets_;
  uint32_t id_;
};

#endif
