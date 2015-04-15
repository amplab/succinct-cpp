#include "succinct/SuccinctShard.hpp"

SuccinctShard::SuccinctShard(uint32_t id, std::string filename, SuccinctMode s_mode, uint32_t sa_sampling_rate,
        uint32_t isa_sampling_rate, uint32_t npa_sampling_rate, SamplingScheme sa_sampling_scheme,
        SamplingScheme isa_sampling_scheme, NPA::NPAEncodingScheme npa_encoding_scheme, uint32_t context_len,
        uint32_t sampling_range)
    : SuccinctCore(filename.c_str(),
            s_mode,
            sa_sampling_rate,
            isa_sampling_rate,
            npa_sampling_rate,
            context_len,
            sa_sampling_scheme,
            isa_sampling_scheme,
            npa_encoding_scheme,
            sampling_range) {

    this->id = id;
    this->input_datafile = filename;
    this->succinct_datafile = filename + ".succinct";

    // Read keys and value offsets
    std::ifstream input(filename);
    int64_t value_offset = 0;
    for(uint32_t i = 0; !input.eof(); i++) {
        keys.push_back(i);
        value_offsets.push_back(value_offset);
        std::string line;
        std::getline(input, line, '\n');
        value_offset += line.length() + 1;
    }
    input.close();
    invalid_offsets = new BitMap;
    init_bitmap(&invalid_offsets, keys.size(), s_allocator);
}

uint64_t SuccinctShard::compute_context_value(const char *p, uint64_t i) {
    uint64_t val = 0;
    uint64_t max = MIN((i + npa->get_context_len()), input_size);
    for (uint64_t t = i; t < max; t++) {
        val = val * alphabet_size + alphabet_map[p[t]].second;
    }

    if (max < i + npa->get_context_len()) {
        for (uint64_t t = 0; t < (i + npa->get_context_len()) % input_size; t++) {
            val = val * alphabet_size + alphabet_map[p[t]].second;
        }
    }

    return val;
}

std::pair<int64_t, int64_t> SuccinctShard::get_range_slow(const char *p,
                                                        uint64_t len) {
    std::pair<int64_t, int64_t> range(0, -1);
    uint64_t m = strlen(p);
    int64_t sp, ep, c1, c2;

    if (alphabet_map.find(p[m - 1]) != alphabet_map.end()) {
        sp = (alphabet_map[p[m - 1]]).first;
        ep = alphabet_map[alphabet[alphabet_map[p[m - 1]].second + 1]].first - 1;
    } else return range;

    for (int64_t i = m - 2; i >= 0; i--) {
        if (alphabet_map.find(p[m - 1]) != alphabet_map.end()) {
            c1 = alphabet_map[p[i]].first;
            c2 = alphabet_map[alphabet[alphabet_map[p[i]].second + 1]].first - 1;
        } else return range;

        if(c2 < c1) return range;

        sp = npa->binary_search_npa(sp, c1, c2, false);
        ep = npa->binary_search_npa(ep, c1, c2, true);

        if (sp > ep) return range;
    }

    range.first = sp;
    range.second = ep;

    return range;
}

std::pair<int64_t, int64_t> SuccinctShard::get_range(const char *p,
                                                    uint64_t len) {
    uint64_t m = strlen(p);
    if (m <= npa->get_context_len()) {
       return get_range_slow(p, len);
    }
    std::pair<int64_t, int64_t> range(0, -1);
    uint32_t sigma_id;
    int64_t sp, ep, c1, c2;
    uint64_t start_off;
    uint64_t context_val, context_id;

    sigma_id = alphabet_map[p[m - npa->get_context_len() - 1]].second;
    context_val = compute_context_value(p, m - npa->get_context_len());
    context_id = npa->contexts[context_val];
    start_off = get_rank1(&(npa->col_nec[sigma_id]), context_id) - 1;
    sp = npa->col_offsets[sigma_id] + npa->cell_offsets[sigma_id][start_off];
    if(start_off + 1 < npa->cell_offsets[sigma_id].size()) {
       ep = npa->col_offsets[sigma_id] + npa->cell_offsets[sigma_id][start_off + 1] - 1;
    } else if((sigma_id + 1) < alphabet_size){
       ep = npa->col_offsets[sigma_id + 1] - 1;
    } else {
       ep = input_size - 1;
    }

    if(sp > ep) return range;

    for (int64_t i = m - npa->get_context_len() - 2; i >= 0; i--) {
    	if (alphabet_map.find(p[i]) != alphabet_map.end()) {
           sigma_id = alphabet_map[p[i]].second;
           context_val = compute_context_value(p, i + 1);
           if(npa->contexts.find(context_val) == npa->contexts.end()) {
        	   return range;
           }
           context_id = npa->contexts[context_val];
           start_off = get_rank1(&(npa->col_nec[sigma_id]), context_id) - 1;
           c1 = npa->col_offsets[sigma_id] + npa->cell_offsets[sigma_id][start_off];
           if(start_off + 1 < npa->cell_offsets[sigma_id].size()) {
               c2 = npa->col_offsets[sigma_id] + npa->cell_offsets[sigma_id][start_off + 1] - 1;
           } else if((sigma_id + 1) < alphabet_size){
               c2 = npa->col_offsets[sigma_id + 1] - 1;
           } else {
               c2 = input_size - 1;
           }
           if(c2 < c1) return range;
    	} else return range;

		sp = npa->binary_search_npa(sp, c1, c2, false);
		ep = npa->binary_search_npa(ep, c1, c2, true);

		if (sp > ep) return range;
    }

    range.first = sp;
    range.second = ep;

    return range;
}

std::string SuccinctShard::name() {
    return this->succinct_datafile;
}

size_t SuccinctShard::num_keys() {
    return keys.size();
}

uint32_t SuccinctShard::sa_sampling_rate() {
    return SA->get_sampling_rate();
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

void SuccinctShard::access(std::string& result, int64_t key, int32_t offset, int32_t len) {
    result = "";
    int64_t pos = get_value_offset_pos(key);
    if(pos < 0)
        return;
    int64_t start = value_offsets[pos] + offset;
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
        if(ISA->is_sampled(next_pos)) {
            idx = lookupISA(next_pos);
        } else {
            idx = lookupNPA(idx);
        }
    }
}

int64_t SuccinctShard::get_key_pos(const int64_t value_offset) {
    int64_t pos = std::prev(std::upper_bound(value_offsets.begin(),
                                            value_offsets.end(),
                                            value_offset)) - value_offsets.begin();
    return (pos >= (int64_t)value_offsets.size() || ACCESSBIT(invalid_offsets, pos) == 1) ? -1 : pos;
}

void SuccinctShard::search(std::set<int64_t> &result, std::string str) {
    std::pair<int64_t, int64_t> range = get_range(str.c_str(), str.length());
    if(range.first > range.second) return;
    for(int64_t i = range.first; i <= range.second; i++) {
        int64_t key_pos = get_key_pos((int64_t)lookupSA(i));
        if(key_pos >= 0) {
           result.insert(keys[key_pos]);
        }
    }
}

int64_t SuccinctShard::count(std::string str) {
    std::set<int64_t> result;
    search(result, str);
    return result.size();
}

size_t SuccinctShard::storage_size() {
    // TODO: Add size of keys, offsets, invalid_offsets bitmap
    return SuccinctCore::storage_size();
}
