#ifndef SUCCINCT_H
#define SUCCINCT_H

#include <cstdint>
#include <string>
#include <cstring>
#include <vector>

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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

    SuccinctShard(uint32_t id, std::string datafile, uint32_t num_keys, bool construct = true,
            uint32_t isa_sampling_rate = 32, uint32_t npa_sampling_rate = 128);

    std::string name();

    size_t num_keys();

    void get(std::string& result, int64_t key);

    void access(std::string& result, int64_t key, int32_t len);

    void fetch(std::string& result);

private:
    int64_t get_value_offset_pos(const int64_t key);

};

#endif
