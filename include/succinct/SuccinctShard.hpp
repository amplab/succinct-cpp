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

#include "succinct/SuccinctCore.hpp"

class SuccinctShard : public SuccinctCore {
private:
    std::string input_datafile;
    std::string succinct_datafile;

    std::vector<int64_t> keys;
    std::vector<int64_t> value_offsets;
    BitMap *invalid_offsets;

    uint32_t id;

public:
    static const int64_t MAX_KEYS = 1L << 32;

    SuccinctShard(uint32_t id, std::string datafile, bool construct = true, uint32_t sa_sampling_rate = 32,
            uint32_t isa_sampling_rate = 32, uint32_t npa_sampling_rate = 128,
            SamplingScheme sa_sampling_scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX,
            SamplingScheme isa_sampling_scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX,
            NPA::NPAEncodingScheme npa_encoding_scheme = NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED,
            uint32_t context_len = 3);

    uint32_t sa_sampling_rate();

    uint32_t isa_sampling_rate();

    uint32_t npa_sampling_rate();

    std::string name();

    size_t num_keys();

    void get(std::string& result, int64_t key);

    void access(std::string& result, int64_t key, int32_t offset, int32_t len);

    int64_t count(std::string str);

    void search(std::set<int64_t>& result, std::string str);

private:
    int64_t get_key_pos(const int64_t value_offset);
    int64_t get_value_offset_pos(const int64_t key);

    std::pair<int64_t, int64_t> get_range_slow(const char *str, uint64_t len);
    std::pair<int64_t, int64_t> get_range(const char *str, uint64_t len);

    uint64_t compute_context_value(const char *str, uint64_t pos);

};

#endif
