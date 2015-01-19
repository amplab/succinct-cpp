#ifndef SUCCINCT_H
#define SUCCINCT_H

#include <cstdint>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <assert.h>

class KVStoreShard {
private:
    std::string input_datafile;
    char *data;

    std::vector<int64_t> keys;
    std::vector<int64_t> value_offsets;

    uint32_t id;

    size_t input_size;

public:
    static const int64_t MAX_KEYS = 1L << 32;

    KVStoreShard(uint32_t id, std::string datafile, uint32_t num_keys);

    std::string name();

    size_t num_keys();

    size_t size();

    /*
     * Random access into the Succinct file with the specified offset
     * and length
     */
    void get(std::string& result, int64_t key);

private:
    int64_t get_value_offset_pos(const int64_t key);

};

#endif
