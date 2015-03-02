#ifndef DELTA_ENCODED_NPA_H
#define DELTA_ENCODED_NPA_H

#include "succinct/npa/NPA.hpp"

#include "succinct/utils/SuccinctUtils.hpp"

class DeltaEncodedNPA : public NPA {
public:

    typedef struct _dev {
        bitmap_t *samples;
        bitmap_t *deltas;
        bitmap_t *delta_offsets;

        uint8_t sample_bits;
        uint8_t delta_offset_bits;
    } DeltaEncodedVector;

    DeltaEncodedVector *del_npa;

    virtual size_t serialize_delta_encoded_vector(DeltaEncodedVector *dv,
                                                    std::ostream& out) {

        size_t out_size = 0;

        out.write(reinterpret_cast<const char *>(&(dv->sample_bits)), sizeof(uint8_t));
        out_size += sizeof(uint8_t);
        out.write(reinterpret_cast<const char *>(&(dv->delta_offset_bits)), sizeof(uint8_t));
        out_size += sizeof(uint8_t);

        out_size += SuccinctBase::serialize_bitmap(dv->samples, out);
        out_size += SuccinctBase::serialize_bitmap(dv->deltas, out);
        out_size += SuccinctBase::serialize_bitmap(dv->delta_offsets, out);

        return out_size;
    }

    virtual size_t deserialize_delta_encoded_vector(DeltaEncodedVector *dv,
                                                    std::istream& in) {
        size_t in_size = 0;

        in.read(reinterpret_cast<char *>(&(dv->sample_bits)), sizeof(uint8_t));
        in_size += sizeof(uint8_t);
        in.read(reinterpret_cast<char *>(&(dv->delta_offset_bits)), sizeof(uint8_t));
        in_size += sizeof(uint8_t);

        in_size += SuccinctBase::deserialize_bitmap(&(dv->samples), in);
        in_size += SuccinctBase::deserialize_bitmap(&(dv->deltas), in);
        in_size += SuccinctBase::deserialize_bitmap(&(dv->delta_offsets), in);

        return in_size;
    }

    virtual size_t dev_size(DeltaEncodedVector *dv) {
        if(dv == NULL) return 0;
        return sizeof(uint8_t) + sizeof(uint8_t) +
                SuccinctBase::bitmap_size(dv->samples) +
                SuccinctBase::bitmap_size(dv->deltas) +
                SuccinctBase::bitmap_size(dv->delta_offsets);
    }

protected:

    // Return lower bound integer log (base 2) for n
    static uint32_t lower_log_2(uint64_t n) {
        return SuccinctUtils::int_log_2(n + 1) - 1;
    }

    // Create delta encoded vector
    virtual void createDEV(DeltaEncodedVector *dv,
            std::vector<uint64_t> &data) = 0;

    // Lookup delta encoded vector
    virtual uint64_t lookupDEV(DeltaEncodedVector *dv, uint64_t i) = 0;

public:
    // Constructor
    DeltaEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
            uint32_t context_len, uint32_t sampling_rate,
            NPAEncodingScheme delta_encoding_scheme,
            SuccinctAllocator &s_allocator) :
            NPA(npa_size, sigma_size, context_len, sampling_rate,
                    delta_encoding_scheme, s_allocator) {
        this->del_npa = NULL;
    }

    // Virtual destructor
    virtual ~DeltaEncodedNPA() {};

    // Encode DeltaEncodedNPA based on the delta encoding scheme
    virtual void encode(bitmap_t *data_bitmap, bitmap_t *compactSA,
                            bitmap_t *compactISA) {
        uint32_t logn, q;
        uint64_t k1, k2, k = 0, l_off = 0, c_id, c_val, npa_val, p = 0;

        std::map<uint64_t, uint64_t> context_size;
        std::vector<uint64_t> **table;
        std::vector<uint64_t> sigma_list;

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

        for (std::map<uint64_t, uint64_t>::iterator it = contexts.begin(); it != contexts.end(); it++) {
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
        del_npa = new DeltaEncodedVector[sigma_size]();

        for (uint64_t i = 0; i < k; i++) {
            table[i] = new std::vector<uint64_t>[sigma_size];
        }

        SuccinctBase::init_bitmap(&(E), k * sigma_size, s_allocator);

        k1 = SuccinctBase::lookup_bitmap_array(compactSA, 0, logn);
        c_id = compute_context_val(data_bitmap, (k1 + 1) % npa_size);
        c_val = contexts[c_id];
        npa_val = SuccinctBase::lookup_bitmap_array(compactISA,
                    (SuccinctBase::lookup_bitmap_array(compactSA, 0, logn) + 1)
                                % npa_size, logn);
        table[c_val][l_off / k].push_back(npa_val);
        starts[c_val] = MIN(starts[c_val], npa_val);

        sigma_list.push_back(npa_val);

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
                createDEV(&(del_npa[l_off/k]), sigma_list);
                assert(sigma_list.size() > 0);
                sigma_list.clear();

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
                    (SuccinctBase::lookup_bitmap_array(compactSA, i, logn) + 1)
                            % npa_size, logn);
            sigma_list.push_back(npa_val);

            assert(l_off / k < sigma_size);
            // Push npa value to npa table: note indices. indexed by context value, and l_offs / k
            table[c_val][l_off / k].push_back(npa_val);
            assert(table[c_val][l_off / k].back() == npa_val);
            starts[c_val] = MIN(starts[c_val], npa_val);
        }

        createDEV(&(del_npa[l_off/k]), sigma_list);
        assert(sigma_list.size() > 0);
        sigma_list.clear();

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
                    row_nec[i].push_back(j);
                    q++;
                }
            }
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

    // Access element at index i
    virtual uint64_t operator[](uint64_t i) {
        // Get column id
        uint64_t column_id = SuccinctBase::get_rank1(&col_offsets, i) - 1;
        assert(column_id < sigma_size);
        uint64_t npa_val = lookupDEV(&(del_npa[column_id]), i - col_offsets[column_id]);
        return npa_val;
    }

    virtual size_t storage_size() {
        size_t tot_size = 3 * sizeof(uint64_t) + 2 * sizeof(uint32_t);
        tot_size += sizeof(contexts.size()) + contexts.size() * (sizeof(uint64_t) * 2);
        tot_size += SuccinctBase::vector_size(row_offsets);
        tot_size += SuccinctBase::vector_size(col_offsets);

        for(uint64_t i = 0; i < sigma_size; i++) {
            tot_size += SuccinctBase::vector_size(col_nec[i]);
        }

        for(uint64_t i = 0; i < contexts.size(); i++) {
            tot_size += SuccinctBase::vector_size(row_nec[i]);
        }

        for(uint64_t i = 0; i < sigma_size; i++) {
            tot_size += SuccinctBase::vector_size(cell_offsets[i]);
        }

        for(uint64_t i = 0; i < sigma_size; i++) {
            tot_size += dev_size(&del_npa[i]);
        }

        return tot_size;
    }

    virtual size_t serialize(std::ostream& out) {
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

        // Output row_offsets
        out_size += SuccinctBase::serialize_vector(row_offsets, out);

        // Output col_offsets
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


        // Output delta encoded vectors
        for(uint64_t i = 0; i < sigma_size; i++) {
            out_size += serialize_delta_encoded_vector(&del_npa[i], out);
        }

        return out_size;
    }

    virtual size_t deserialize(std::istream& in) {
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
        col_nec = new std::vector<uint64_t>[sigma_size];
        for(uint64_t i = 0; i < sigma_size; i++) {
            in_size += SuccinctBase::deserialize_vector(col_nec[i], in);
        }

        // Read necrow
        row_nec = new std::vector<uint64_t>[contexts.size()];
        for(uint64_t i = 0; i < contexts.size(); i++) {
            in_size += SuccinctBase::deserialize_vector(row_nec[i], in);
        }

        // Read cell offsets
        cell_offsets = new std::vector<uint64_t>[sigma_size];
        for(uint64_t i = 0; i < sigma_size; i++) {
            in_size += SuccinctBase::deserialize_vector(cell_offsets[i], in);
        }

        // Read delta encoded vectors
        del_npa = new DeltaEncodedVector[sigma_size];
        for(uint64_t i = 0; i < sigma_size; i++) {
            in_size += deserialize_delta_encoded_vector(&del_npa[i], in);
        }

        return in_size;
    }

};

#endif
