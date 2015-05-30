#include "succinct/SuccinctFile.hpp"

SuccinctFile::SuccinctFile(std::string filename, SuccinctMode s_mode)
    : SuccinctCore(filename.c_str(), s_mode) {
    this->input_filename = filename;
    this->succinct_filename = filename + ".succinct";
}

uint64_t SuccinctFile::compute_context_value(const char *p, uint64_t i) {
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

std::pair<int64_t, int64_t> SuccinctFile::get_range_slow(const char *p,
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

std::pair<int64_t, int64_t> SuccinctFile::get_range(const char *p,
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

std::string SuccinctFile::name() {
    return input_filename;
}

void SuccinctFile::extract(std::string& result, uint64_t offset, uint64_t len) {
    result = "";
    uint64_t idx = lookupISA(offset);
    for (uint64_t k = 0;  k < len; k++) {
        result += alphabet[lookupC(idx)];
        idx = lookupNPA(idx);
    }
}

char SuccinctFile::char_at(uint64_t pos) {
    uint64_t idx = lookupISA(pos);
    return alphabet[lookupC(idx)];
}

uint64_t SuccinctFile::count(std::string str) {
    std::pair<int64_t, int64_t> range = get_range(str.c_str(), str.length());
    return range.second - range.first + 1;
}

void SuccinctFile::search(std::vector<int64_t>& result, std::string str) {
    std::pair<int64_t, int64_t> range = get_range(str.c_str(), str.length());
    if(range.first > range.second) return;
    result.reserve((uint64_t)(range.second - range.first + 1));
    for(int64_t i = range.first; i <= range.second; i++) {
        result.push_back((int64_t)lookupSA(i));
    }
}

void SuccinctFile::wildcard_search(std::vector<int64_t>& result,
        std::string pattern, uint64_t max_sep) {
    // TODO: Implement wild card search
}
