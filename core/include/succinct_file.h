#ifndef SUCCINCT_H
#define SUCCINCT_H

#include <cstdint>
#include <string>
#include <cstring>
#include <vector>
#include <set>

#include "succinct_core.h"
#include "regex/regex.h"

class SuccinctFile : public SuccinctCore {
 public:
  SuccinctFile(const std::string& filename, SuccinctMode s_mode =
                   SuccinctMode::CONSTRUCT_IN_MEMORY,
               uint32_t sa_sampling_rate = 32, uint32_t isa_sampling_rate = 32,
               uint32_t npa_sampling_rate = 128,
               SamplingScheme sa_sampling_scheme =
                   SamplingScheme::FLAT_SAMPLE_BY_INDEX,
               SamplingScheme isa_sampling_scheme =
                   SamplingScheme::FLAT_SAMPLE_BY_INDEX,
               NPA::NPAEncodingScheme npa_encoding_scheme =
                   NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED,
               uint32_t context_len = 3, uint32_t sampling_range = 1024);

  /*
   * Random access into the Succinct file with the specified offset
   * and length
   */
  void Extract(std::string& result, uint64_t offset, uint64_t len);

  /*
   * Get the count of a string in the Succinct file
   */
  uint64_t Count(const std::string& str);

  /*
   * Get the offsets of all the occurrences
   * of a string in the Succinct file
   */
  void Search(std::vector<int64_t>& result, const std::string& str);

  /*
   * Get the offsets corresponding to matches of regex.
   */
  void RegexSearch(std::set<std::pair<size_t, size_t>>& results, const std::string& query);

 private:
  // std::pair<int64_t, int64_t> GetRangeSlow(const char *str, uint64_t len);
  std::pair<int64_t, int64_t> GetRange(const char *str, uint64_t len);
  uint64_t ComputeContextValue(const char *str, uint64_t pos);

};

#endif
