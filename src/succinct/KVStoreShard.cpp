#include "../../include/succinct/KVStoreShard.hpp"

KVStoreShard::KVStoreShard(uint32_t id, std::string filename, uint32_t num_keys) {
    this->id = id;
    this->input_datafile = filename;

    // Read keys and value offsets
    std::ifstream input(filename);
    int64_t value_offset = 0;
    for(uint32_t i = 0; i < num_keys; i++) {
        keys.push_back(i);
        value_offsets.push_back(value_offset);
        std::string line;
        std::getline(input, line, '\n');
        value_offset += line.length() + 1;
    }

    // Memory map file
    struct stat st;
    stat(filename.c_str(), &st);
    this->input_size = st.st_size;

    int fd = open(filename.c_str(), O_RDONLY, 0);
    assert(fd != -1);

    data = (char *)mmap(NULL, this->input_size, PROT_READ, MAP_PRIVATE, fd, 0);

    input.close();
}

std::string KVStoreShard::name() {
    return this->input_datafile;
}

size_t KVStoreShard::num_keys() {
    return keys.size();
}

int64_t KVStoreShard::get_value_offset_pos(const int64_t key) {
    size_t pos = std::lower_bound(keys.begin(), keys.end(), key) - keys.begin();
    return (keys[pos] != key || pos >= keys.size()) ? -1 : pos;
}

size_t KVStoreShard::size() {
    return this->input_size;
}

void KVStoreShard::get(std::string& result, int64_t key) {
    result = "";
    int64_t pos = get_value_offset_pos(key);
    if(pos < 0)
        return;
    int64_t start = value_offsets[pos];
    int64_t end = (pos + 1 < value_offsets.size()) ? value_offsets[pos + 1] : input_size;
    int64_t len = end - start - 1;
    result.resize(len);
    for(uint64_t i = start; i < end; i++) {
        result[i - start] = data[i];
    }
}

void KVStoreShard::access(std::string& result, int64_t key, int32_t len) {
    result = "";
    int64_t pos = get_value_offset_pos(key);
    if(pos < 0)
        return;
    int64_t start = value_offsets[pos];
    result.resize(len);
    for(uint64_t i = start; i < start + len; i++) {
        result[i - start] = data[i];
    }
}
