#ifndef SUCCINCT_H
#define SUCCINCT_H

#include <cstdint>
#include <string>
#include <cstring>
#include <vector>

#include "succinct_core.h"

class SuccinctFile : public SuccinctCore {
 public:
  SuccinctFile(std::string filename, SuccinctMode s_mode =
                   SuccinctMode::CONSTRUCT_IN_MEMORY);

  std::string Name();
  /*
   * Random access into the Succinct file with the specified offset
   * and length
   */
  void Extract(std::string& result, uint64_t offset, uint64_t len);

  /*
   * Get the count of a string in the Succinct file
   */
  uint64_t Count(std::string str);

  /*
   * Get the offsets of all the occurrences
   * of a string in the Succinct file
   */
  void Search(std::vector<int64_t>& result, std::string str);

  /*
   * Get the offsets of all the occurrences of a
   * wild-card string in the Succinct file
   */
  void WildCardSearch(std::vector<int64_t>& result, std::string pattern,
                      uint64_t max_sep);

 private:
  std::pair<int64_t, int64_t> GetRangeSlow(const char *str, uint64_t len);
  std::pair<int64_t, int64_t> GetRange(const char *str, uint64_t len);

  uint64_t ComputeContextValue(const char *str, uint64_t pos);

  void ReadChunk(std::string& out, const int64_t out_pos,
                 const int64_t chunk_id, const int64_t chunk_start, int64_t chunk_end);

  std::string input_filename_;
  std::string succinct_filename_;

};

#endif
