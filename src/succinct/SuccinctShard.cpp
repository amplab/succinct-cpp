#include "../../include/succinct/SuccinctShard.hpp"

SuccinctShard::SuccinctShard(uint32_t id, std::string filename, uint32_t num_keys, bool construct,
        uint32_t isa_sampling_rate, uint32_t npa_sampling_rate)
    : SuccinctCore(filename.c_str(),
            construct,
            isa_sampling_rate,
            npa_sampling_rate) {
    this->id = id;
    this->input_datafile = filename;
    this->succinct_datafile = filename + ".succinct";

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
    input.close();
    invalid_offsets = new BitMap;
    init_bitmap(&invalid_offsets, num_keys, s_allocator);
}

std::string SuccinctShard::name() {
    return this->succinct_datafile;
}

size_t SuccinctShard::num_keys() {
    return keys.size();
}

uint32_t SuccinctShard::isa_sampling_rate() {
    return ISA->get_sampling_rate();
}

uint32_t SuccinctShard::npa_sampling_rate() {
    return npa->get_sampling_rate();
}

int64_t SuccinctShard::get_value_offset_pos(const int64_t key) {
    size_t pos = std::lower_bound(keys.begin(), keys.end(), key) - keys.begin();
    return (keys[pos] != key || pos >= keys.size() || ACCESSBIT(invalid_offsets, pos) == 1) ? -1 : pos;
}

void SuccinctShard::access(std::string& result, int64_t key, int32_t len) {
    result = "";
    int64_t pos = get_value_offset_pos(key);
    if(pos < 0)
        return;
    int64_t start = value_offsets[pos];
    result.resize(len);
    uint64_t idx = lookupISA(start);
    for(int64_t i = 0; i < len; i++) {
        result[i] = alphabet[lookupC(idx)];
        uint64_t next_pos = start + i + 1;
        if((next_pos % ISA->get_sampling_rate()) == 0) {
            idx = lookupISA(next_pos);
        } else {
            idx = lookupNPA(idx);
        }
    }
}

void SuccinctShard::get(std::string& result, int64_t key) {
    result = "";
    int64_t pos = get_value_offset_pos(key);
    if(pos < 0)
        return;
    int64_t start = value_offsets[pos];
    int64_t end = ((size_t)(pos + 1) < value_offsets.size()) ? value_offsets[pos + 1] : input_size;
    int64_t len = end - start - 1;
    result.resize(len);
    uint64_t idx = lookupISA(start);
    for(int64_t i = 0; i < len; i++) {
        result[i] = alphabet[lookupC(idx)];
        uint64_t next_pos = start + i + 1;
        if((next_pos % ISA->get_sampling_rate()) == 0) {
            idx = lookupISA(next_pos);
        } else {
            idx = lookupNPA(idx);
        }
    }
}

void SuccinctShard::fetch(std::string &result, int64_t offset, int32_t len) {
    // Memory map file
    struct stat st;
    stat(succinct_datafile.c_str(), &st);
    size_t succinct_size = st.st_size;
    if((size_t)(offset + len) > succinct_size) {
        len = succinct_size - offset;
    }
    result.resize(len);

    std::ifstream in(this->succinct_datafile, std::ios::in | std::ios::binary);
    if (in) {
        in.seekg(offset);
        in.read(&result[0], result.size());
        in.close();
        return;
    }
    fprintf(stderr, "Could not read file %s!\n", this->succinct_datafile.c_str());
}
