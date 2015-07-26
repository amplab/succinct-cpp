#include "npa/WaveletTreeEncodedNPA.hpp"

WaveletTreeEncodedNPA::WaveletTreeEncodedNPA(uint64_t npa_size,
        uint64_t sigma_size, uint32_t context_len, uint32_t sampling_rate,
        bitmap_t *data_bitmap, bitmap_t *compactSA, bitmap_t *compactISA,
        SuccinctAllocator &s_allocator, SuccinctBase *s_base) :
        NPA(npa_size, sigma_size, context_len, sampling_rate,
                NPAEncodingScheme::WAVELET_TREE_ENCODED, s_allocator) {
    this->wtree = NULL;
    this->s_base = s_base;
    this->csizes = NULL;
    encode(data_bitmap, compactSA, compactISA);
}

WaveletTreeEncodedNPA::WaveletTreeEncodedNPA(uint32_t context_len,
        uint32_t sampling_rate, SuccinctAllocator &s_allocator,
        SuccinctBase *s_base) :
        NPA(0, 0, context_len, sampling_rate,
                NPAEncodingScheme::WAVELET_TREE_ENCODED, s_allocator) {
    this->wtree = NULL;
    this->s_base = s_base;
    this->csizes = NULL;
}

void WaveletTreeEncodedNPA::create_wavelettree(WaveletNode **w, uint64_t s,
        uint64_t e, std::vector<uint64_t> &v, std::vector<uint64_t> &v_c) {
    if (s == e) {
        (*w) = NULL;
        return;
    }

    unsigned char m = v_c.at((v.size() - 1) / 2);
    m = MIN(m, e - 1);

    uint64_t r;
    std::vector<uint64_t> v1, v1_c, v0, v0_c;

    (*w) = new WaveletNode;
    bitmap_t *B = new bitmap_t;
    SuccinctBase::init_bitmap(&(B), v.size(), s_allocator);
    (*w)->id = m;

    for (uint64_t i = 0; i < v.size(); i++) {
        if (v_c.at(i) > m && v_c.at(i) <= e) {
            SETBITVAL((B), v.at(i));
        }
    }

    s_base->create_dictionary(B, &((*w)->D));
    SuccinctBase::destroy_bitmap(&B, s_allocator);

    for (uint64_t i = 0; i < v.size(); i++) {
        if (v_c.at(i) > m && v_c.at(i) <= e) {
            r = s_base->get_rank1(&((*w)->D), v.at(i)) - 1;
            assert(r >= 0);
            v1.push_back(r);
            v1_c.push_back(v_c.at(i));
        } else {
            r = s_base->get_rank0(&((*w)->D), v.at(i)) - 1;
            assert(r >= 0);
            v0.push_back(r);
            v0_c.push_back(v_c.at(i));
        }
    }

    create_wavelettree(&((*w)->lt), s, m, v0, v0_c);
    v0.clear();
    v0_c.clear();

    create_wavelettree(&((*w)->rt), m + 1, e, v1, v1_c);
    v1.clear();
    v1_c.clear();
}

uint64_t WaveletTreeEncodedNPA::lookup_wavelettree(WaveletNode *tree,
        uint64_t context_off, uint64_t cell_off, uint64_t s, uint64_t e) {

    if (tree == NULL)
        return cell_off;

    unsigned char m = tree->id;
    uint64_t p, v;
    if (context_off > m && context_off <= e) {
        p = lookup_wavelettree(tree->rt, context_off, cell_off, m + 1, e);
        v = s_base->get_select1(&(tree->D), p);
    } else {
        p = lookup_wavelettree(tree->lt, context_off, cell_off, s, m);
        v = s_base->get_select0(&(tree->D), p);
    }

    return v;

}

void WaveletTreeEncodedNPA::encode(bitmap_t *data_bitmap, bitmap_t *compactSA,
        bitmap_t *compactISA) {
    uint32_t logn, q;
    uint64_t k1, k2, k = 0, l_off = 0, c_id, c_val, npa_val, p = 0;

    std::map<uint64_t, uint64_t> context_size;
    std::vector<uint64_t> **table;
    std::vector<std::pair<std::vector<uint64_t>, unsigned char> > context;
    std::vector<uint64_t> cell;
    std::vector<uint64_t> v, v_c;

    bool flag;
    uint64_t *sizes, *starts;
    uint64_t last_i = 0;

    logn = SuccinctUtils::int_log_2(npa_size + 1);

    for (uint64_t i = 0; i < npa_size; i++) {
        uint64_t c_val = compute_context_val(data_bitmap, i);
        contexts.insert(std::pair<uint64_t, uint64_t>(c_val, 0));
        context_size[c_val]++;
    }

    sizes = new uint64_t[contexts.size()];
    starts = new uint64_t[contexts.size()];

    for (std::map<uint64_t, uint64_t>::iterator it = contexts.begin();
            it != contexts.end(); it++) {
        sizes[k] = context_size[it->first];
        starts[k] = npa_size;
        contexts[it->first] = k++;
    }

    assert(k == contexts.size());
    context_size.clear();

    bitmap_t *E = new bitmap_t;
    table = new std::vector<uint64_t>*[k];
    cell_offsets = new std::vector<uint64_t>[sigma_size];
    col_nec = new std::vector<uint64_t>[sigma_size];
    row_nec = new std::vector<uint64_t>[k];
    wtree = new WaveletNode*[k];
    csizes = new uint64_t[k]();

    for (uint64_t i = 0; i < k; i++) {
        table[i] = new std::vector<uint64_t>[sigma_size];
    }

    SuccinctBase::init_bitmap(&(E), k * sigma_size, s_allocator);

    k1 = SuccinctBase::lookup_bitmap_array(compactSA, 0, logn);
    c_id = compute_context_val(data_bitmap, (k1 + 1) % npa_size);
    c_val = contexts[c_id];
    npa_val = SuccinctBase::lookup_bitmap_array(compactISA,
                (SuccinctBase::lookup_bitmap_array(compactSA, 0, logn) + 1) % npa_size, logn);
    table[c_val][l_off / k].push_back(npa_val);
    starts[c_val] = MIN(starts[c_val], npa_val);

    SETBITVAL(E, c_val);
    col_nec[0].push_back(c_val);
    col_offsets.push_back(0);
    cell_offsets[0].push_back(0);

    for (uint64_t i = 1; i < npa_size; i++) {
        k1 = SuccinctBase::lookup_bitmap_array(compactSA, i, logn);
        k2 = SuccinctBase::lookup_bitmap_array(compactSA, i - 1, logn);

        c_id = compute_context_val(data_bitmap, (k1 + 1) % npa_size);
        c_val = contexts[c_id];

        assert(c_val < k);

        if (!compare_data_bitmap(data_bitmap, k1, k2, 1)) {
            col_offsets.push_back(i);
            l_off += k;
            last_i = i;
            cell_offsets[l_off / k].push_back(i - last_i);
        } else if (!compare_data_bitmap(data_bitmap, (k1 + 1) % npa_size,
                                            (k2 + 1) % npa_size, context_len)) {
            // Context has changed; mark in L
            cell_offsets[l_off / k].push_back(i - last_i);
        }

        // If we haven't marked it already, mark current cell as non empty
        if (!ACCESSBIT(E, (l_off + c_val))) {
            SETBITVAL(E, (l_off + c_val));
            col_nec[l_off / k].push_back(c_val);
        }

        // Obtain actual npa value
        npa_val = SuccinctBase::lookup_bitmap_array(compactISA,
                (SuccinctBase::lookup_bitmap_array(compactSA, i, logn) + 1) % npa_size, logn);

        assert(l_off / k < sigma_size);
        // Push npa value to npa table: note indices. indexed by context value, and l_offs / k
        table[c_val][l_off / k].push_back(npa_val);
        assert(table[c_val][l_off / k].back() == npa_val);
        starts[c_val] = MIN(starts[c_val], npa_val);
    }

    for (uint64_t i = 0; i < k; i++) {
        q = 0;
        flag = false;
        for (uint64_t j = 0; j < sigma_size; j++) {
            if(!flag && ACCESSBIT(E, (i + j * k))) {
                row_offsets.push_back(p);
                p += table[i][j].size();
                flag = true;
            } else if(ACCESSBIT(E, (i + j * k))) {
                p += table[i][j].size();
            }

            if (ACCESSBIT(E, (i + j * k))) {
                for (uint64_t t = 0; t < table[i][j].size(); t++) {
                    cell.push_back(table[i][j][t] - starts[i]);
                }
                context.push_back(std::pair<std::vector<uint64_t>, unsigned char> (cell, j));
                cell.clear();
                row_nec[i].push_back(j);
                csizes[i]++;
                q++;
            }
        }

        if (i > 0) {
            csizes[i] += csizes[i - 1];
        }

        for (size_t j = 0; j < context.size(); j++) {
            for (size_t t = 0; t < context[j].first.size(); t++) {
                v.push_back(context[j].first[t]);
                v_c.push_back(j);
            }
        }
        create_wavelettree(&wtree[i], 0, context.size() - 1, v, v_c);
        v.clear();
        v_c.clear();
        context.clear();
    }

    // Clean up
    for (uint64_t i = 0; i < k; i++) {
        for (uint64_t j = 0; j < sigma_size; j++) {
            if(table[i][j].size()) {
                table[i][j].clear();
            }
        }
        delete [] table[i];
    }
    delete [] table;
    delete [] starts;
    delete [] sizes;
    delete [] E->bitmap;
    delete E;

    context_size.clear();
}

uint64_t WaveletTreeEncodedNPA::operator[](uint64_t i) {

    // Get column id
    uint64_t column_id = SuccinctBase::get_rank1(&(col_offsets), i) - 1;

    // Get column offset
    uint64_t column_off = col_offsets[column_id];

    // Get pseudo row id
    uint64_t pseudo_row_id = SuccinctBase::get_rank1(&(cell_offsets[column_id]),
                                                        i - column_off) - 1;

    // Get cell offset
    uint64_t cell_off = i - column_off - cell_offsets[column_id][pseudo_row_id];

    // Get row id
    uint64_t row_id = col_nec[column_id][pseudo_row_id];

    // Get row offset
    uint64_t row_off = row_offsets[row_id];

    // Get cell size
    uint64_t num_contexts = (row_id > 0) ? csizes[row_id] - csizes[row_id - 1]
                                                                   : csizes[0];

    // Get context offset
    uint64_t context_off = SuccinctBase::get_rank1(&(row_nec[row_id]), column_id) - 1;

    // Get row value
    uint64_t row_val = lookup_wavelettree(wtree[row_id], context_off, cell_off, 0,
                                            num_contexts - 1);

    // Get npa value
    uint64_t npa_val = row_off + row_val;
    return npa_val;
}

size_t WaveletTreeEncodedNPA::serialize(std::ostream& out) {
    size_t out_size = 0;
    typedef std::map<uint64_t, uint64_t>::iterator iterator_t;

    // Output NPA scheme
    out.write(reinterpret_cast<const char *>(&(npa_scheme)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output NPA size
    out.write(reinterpret_cast<const char *>(&(npa_size)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output sigma_size
    out.write(reinterpret_cast<const char *>(&(sigma_size)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output context_len
    out.write(reinterpret_cast<const char *>(&(context_len)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);

    // Output sampling rate
    out.write(reinterpret_cast<const char *>(&(sampling_rate)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);

    // Output contexts
    uint64_t context_size = contexts.size();
    out.write(reinterpret_cast<const char *>(&(context_size)), sizeof(uint64_t));
    for (iterator_t it = contexts.begin(); it != contexts.end(); ++it) {
        out.write(reinterpret_cast<const char *>(&(it->first)), sizeof(uint64_t));
        out.write(reinterpret_cast<const char *>(&(it->second)), sizeof(uint64_t));
    }

    // Output rowoffsets
    out_size += SuccinctBase::serialize_vector(row_offsets, out);

    // Output coloffsets
    out_size += SuccinctBase::serialize_vector(col_offsets, out);

    // Output neccol
    for(uint64_t i = 0; i < sigma_size; i++) {
        out_size += SuccinctBase::serialize_vector(col_nec[i], out);
    }

    // Output necrow
    for(uint64_t i = 0; i < contexts.size(); i++) {
        out_size += SuccinctBase::serialize_vector(row_nec[i], out);
    }

    // Output cell offsets
    for(uint64_t i = 0; i < sigma_size; i++) {
        out_size += SuccinctBase::serialize_vector(cell_offsets[i], out);
    }

    // TODO: Add serialization for wavelet tree

    return out_size;
}

size_t WaveletTreeEncodedNPA::deserialize(std::istream& in) {
    size_t in_size = 0;

    // Read NPA scheme
    in.read(reinterpret_cast<char *>(&(npa_scheme)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    // Read NPA size
    in.read(reinterpret_cast<char *>(&(npa_size)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    // Read sigma_size
    in.read(reinterpret_cast<char *>(&(sigma_size)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    // Read context_len
    in.read(reinterpret_cast<char *>(&(context_len)), sizeof(uint32_t));
    in_size += sizeof(uint32_t);

    // Read sampling rate
    in.read(reinterpret_cast<char *>(&(sampling_rate)), sizeof(uint32_t));
    in_size += sizeof(uint32_t);

    // Read contexts
    uint64_t context_size = contexts.size();
    in.read(reinterpret_cast<char *>(&(context_size)), sizeof(uint64_t));
    for (uint64_t i = 0; i < context_size; i++) {
        uint64_t first, second;
        in.read(reinterpret_cast<char *>(&(first)), sizeof(uint64_t));
        in.read(reinterpret_cast<char *>(&(second)), sizeof(uint64_t));
        contexts[first] = second;
    }

    // Read rowoffsets
    in_size += SuccinctBase::deserialize_vector(row_offsets, in);

    // Read coloffsets
    in_size += SuccinctBase::deserialize_vector(col_offsets, in);

    // Read neccol
    for(uint64_t i = 0; i < sigma_size; i++) {
        in_size += SuccinctBase::deserialize_vector(col_nec[i], in);
    }

    // Read necrow
    for(uint64_t i = 0; i < contexts.size(); i++) {
        in_size += SuccinctBase::deserialize_vector(row_nec[i], in);
    }

    // Read cell offsets
    for(uint64_t i = 0; i < sigma_size; i++) {
        in_size += SuccinctBase::deserialize_vector(cell_offsets[i], in);
    }

    // TODO: Add deserialization for wavelet trees

    return in_size;
}

size_t WaveletTreeEncodedNPA::memorymap(std::string filename) {
    uint8_t *data, *data_beg;
    data = data_beg = (uint8_t *)SuccinctUtils::memory_map(filename);

    npa_scheme = (NPAEncodingScheme)(*((uint64_t *)data));
    data += sizeof(uint64_t);
    npa_size = *((uint64_t *)data);
    data += sizeof(uint64_t);
    sigma_size = *((uint64_t *)data);
    data += sizeof(uint64_t);
    context_len = *((uint32_t *)data);
    data += sizeof(uint32_t);
    sampling_rate = *((uint32_t *)data);
    data += sizeof(uint32_t);

    // Read contexts
    uint64_t context_size = *((uint64_t *)data);
    data += sizeof(uint64_t);
    for (uint64_t i = 0; i < context_size; i++) {
        uint64_t first = *((uint64_t *)data);
        data += sizeof(uint64_t);
        uint64_t second = *((uint64_t *)data);
        data += sizeof(uint64_t);
        contexts[first] = second;
    }

    // Read rowoffsets
    data += SuccinctBase::memorymap_vector(row_offsets, data);

    // Read coloffsets
    data += SuccinctBase::memorymap_vector(col_offsets, data);

    // Read neccol
    col_nec = new std::vector<uint64_t>[sigma_size];
    for(uint64_t i = 0; i < sigma_size; i++) {
        data += SuccinctBase::memorymap_vector(col_nec[i], data);
    }

    // Read necrow
    row_nec = new std::vector<uint64_t>[contexts.size()];
    for(uint64_t i = 0; i < contexts.size(); i++) {
        data += SuccinctBase::memorymap_vector(row_nec[i], data);
    }

    // Read cell offsets
    cell_offsets = new std::vector<uint64_t>[sigma_size];
    for(uint64_t i = 0; i < sigma_size; i++) {
        data += SuccinctBase::memorymap_vector(cell_offsets[i], data);
    }

    // TODO: Memory map WaveletTrees

    return data - data_beg;
}

size_t WaveletTreeEncodedNPA::storage_size() {
    // TODO: fix
    return 0;
}
