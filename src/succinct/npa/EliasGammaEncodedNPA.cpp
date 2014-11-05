#include "succinct/npa/EliasGammaEncodedNPA.hpp"

EliasGammaEncodedNPA::EliasGammaEncodedNPA(uint64_t npa_size,
        uint64_t sigma_size, uint32_t context_len, uint32_t sampling_rate,
        bitmap_t *data_bitmap, bitmap_t *compactSA, bitmap_t *compactISA,
        SuccinctAllocator &s_allocator) :
        DeltaEncodedNPA(npa_size, sigma_size, context_len, sampling_rate,
                        NPAEncodingScheme::ELIAS_GAMMA_ENCODED, s_allocator) {
    init_prefixsum();
    encode(data_bitmap, compactSA, compactISA);
}

EliasGammaEncodedNPA::EliasGammaEncodedNPA(uint32_t context_len,
        uint32_t sampling_rate, SuccinctAllocator &s_allocator) :
        DeltaEncodedNPA(0, 0, context_len, sampling_rate,
                        NPAEncodingScheme::ELIAS_GAMMA_ENCODED, s_allocator) {
    init_prefixsum();
}

uint16_t EliasGammaEncodedNPA::access_data_pos16(uint16_t data, uint32_t pos,
                                                    uint32_t b) {
    assert(b <= 16 && pos >= 0);
    if(b == 0) return 0;
    uint16_t val = data << pos;
    return val >> (16 - b);

}

void EliasGammaEncodedNPA::init_prefixsum() {
    for(uint64_t i = 0; i < 65536; i++) {
        uint16_t val = (uint16_t) i;
        uint16_t count = 0, offset = 0, sum = 0;
        while(val && offset <= 16) {
            int N = 0;
            while(!GETBIT16(val, offset)) {
                N++;
                offset++;
            }
            if(offset + (N + 1) <= 16) {
                sum += access_data_pos16(val, offset, N + 1);
                offset += (N + 1);
                count++;
            } else {
                offset -= N;
                break;
            }
        }
        prefixsum[i] = (offset << 24) | (count << 16) | sum;
    }
}

// Compute the elias gamma encoding size in bits for a 64 bit integer
uint32_t EliasGammaEncodedNPA::elias_gamma_encoding_size(uint64_t n) {
    uint32_t N = lower_log_2(n);
    return 2 * N + 1;
}

// Encode a sorted vector using elias gamma encoding
void EliasGammaEncodedNPA::elias_gamma_encode(bitmap_t **B,
                                                std::vector<uint64_t> &deltas,
                                                uint64_t size) {
    if(size == 0) {
        delete *B;
        *B = NULL;
        return;
    }
    uint64_t pos = 0;
    SuccinctBase::init_bitmap(B, size, s_allocator);
    for(size_t i = 0; i < deltas.size(); i++) {
        int bits = elias_gamma_encoding_size(deltas[i]);
        SuccinctBase::set_bitmap_pos(B, pos, deltas[i], bits);
        pos += bits;
    }
}

// Decode a particular elias-gamma encoded delta value at a provided offset
// in the deltas bitmap
uint64_t EliasGammaEncodedNPA::elias_gamma_decode(bitmap_t *B,
                                                    uint64_t *offset) {
    uint32_t N = 0;
#if USE_BSR
    uint64_t data = B->bitmap[(*offset) / 64] << ((*offset) % 64);
    N = __builtin_clz(data);
    (*offset) += N;
#else
    while(!ACCESSBIT(B, (*offset))) {
        N++;
        (*offset)++;
    }
#endif
    long val = SuccinctBase::lookup_bitmap_pos(B, *offset, N + 1);
    *offset += (N + 1);
    return val;
}

// Compute the prefix sum of the elias-gamma encoded deltas
uint64_t EliasGammaEncodedNPA::elias_gamma_prefix_sum(bitmap_t *B,
                                                        uint64_t offset,
                                                        uint64_t i) {
    uint64_t delta_sum = 0;

#if USE_PREFIXSUM_TABLE
    uint64_t delta_idx = 0;
    uint64_t delta_off = offset;
    while(delta_idx != i) {
        uint16_t block =
                (uint16_t)SuccinctBase::lookup_bitmap_pos(B, delta_off, 16);
        uint16_t cnt = PREFIX_CNT(block);
        if(cnt == 0) {
            // If the prefixsum table for the block returns count == 0
            // this must mean the value spans more than 16 bits
            // read this manually
            uint32_t N = 0;
#if USE_BSR
            uint64_t data = lookup_bitmap_pos(B, delta_off, 64);
            N = __builtin_clzl(data);
            delta_off += N;
#else
            while(!ACCESSBIT(B, delta_off)) {
                N++;
                delta_off++;
            }
#endif
            delta_sum += SuccinctBase::lookup_bitmap_pos(B, delta_off, N + 1);
            delta_off += (N + 1);
            delta_idx += 1;
        } else if(delta_idx + cnt <= i) {
            // If sum can be computed from the prefixsum table
            delta_sum += PREFIX_SUM(block);
            delta_off += PREFIX_OFF(block);
            delta_idx += cnt;
        } else {
            // Last few values, decode them without looking up table
            while(delta_idx != i) {
                uint32_t N = 0;
#if USE_BSR
                uint64_t data = lookup_bitmap_pos(B, delta_off, 64);
                N = __builtin_clzl(data);
                delta_off += N;
#else
                while(!ACCESSBIT(B, delta_off)) {
                    N++;
                    delta_off++;
                }
#endif
                delta_sum += SuccinctBase::lookup_bitmap_pos(B, delta_off, N + 1);
                delta_off += (N + 1);
                delta_idx += 1;
            }
        }
    }
#else
    for(uint64_t j = 0; j < i; j++) {
        uint32_t N = 0;
#if USE_BSR
        uint64_t data = lookup_bitmap_pos(B, offset, 64);
        N = __builtin_clzl(data);
        offset += N;
#else
        while(!ACCESSBIT(B, offset)) {
            N++;
            offset++;
        }
#endif
        delta_sum += lookup_bitmap_pos(B, offset, N + 1);
        offset += (N + 1);
    }
#endif
    return delta_sum;
}

void EliasGammaEncodedNPA::createDEV(DeltaEncodedVector *dv,
                                        std::vector<uint64_t> &data) {
    if(data.size() == 0) {
        return;
    }
    assert(dv != NULL);

    uint64_t max_sample = 0;
    std::vector<uint64_t> _samples, _deltas, _delta_offsets;
    uint64_t tot_delta_count = 0, delta_count = 0;
    uint64_t delta_enc_size = 0;
    uint64_t cum_delta_size = 0, max_offset = 0;
    uint64_t last_val = 0;

    for(size_t i = 0; i < data.size(); i++) {
        if(i % sampling_rate == 0) {
            _samples.push_back(data[i]);
            if(data[i] > max_sample) {
                max_sample = data[i];
            }
            if(cum_delta_size > max_offset)
                max_offset = cum_delta_size;
            _delta_offsets.push_back(cum_delta_size);
            if(i != 0) {
                assert(delta_count == sampling_rate - 1);
                tot_delta_count += delta_count;
                delta_count = 0;
            }
        } else {
            long delta = data[i] - last_val;
            assert(delta > 0);
            _deltas.push_back(delta);

            delta_enc_size = elias_gamma_encoding_size(delta);
            cum_delta_size += delta_enc_size;
            delta_count++;
        }
        last_val = data[i];
    }
    tot_delta_count += delta_count;

    assert(tot_delta_count == _deltas.size());
    assert(_samples.size() + _deltas.size() == data.size());
    assert(_delta_offsets.size() == _samples.size());

    // Can occur at most once per context;
    // only occurs if 0 is the only value in the cell
    if(max_sample == 0) dv->sample_bits = 1;
    else dv->sample_bits =
            (uint8_t) SuccinctUtils::int_log_2(max_sample + 1);

    if(max_offset == 0) dv->delta_offset_bits = 1;
    else dv->delta_offset_bits =
            (uint8_t) SuccinctUtils::int_log_2(max_offset + 1);

    dv->samples = new bitmap_t;
    dv->deltas = new bitmap_t;
    SuccinctBase::create_bitmap_array(&(dv->samples), (_samples.size() == 0) ?
            NULL : &_samples[0], _samples.size(), dv->sample_bits, s_allocator);

    elias_gamma_encode(&(dv->deltas), _deltas, cum_delta_size);
    dv->delta_offsets = new bitmap_t;
    SuccinctBase::create_bitmap_array(&(dv->delta_offsets),
                        (_delta_offsets.size() == 0) ? NULL :
                        &_delta_offsets[0], _delta_offsets.size(),
                        dv->delta_offset_bits, s_allocator);

}

uint64_t EliasGammaEncodedNPA::lookupDEV(DeltaEncodedVector *dv, uint64_t i) {
    uint64_t sample_offset = i / sampling_rate;
    uint64_t delta_offset_idx = i % sampling_rate;
    uint64_t val = SuccinctBase::lookup_bitmap_array(dv->samples, sample_offset,
                                    dv->sample_bits);
    if(delta_offset_idx == 0) return val;
    uint64_t delta_offset = SuccinctBase::lookup_bitmap_array(dv->delta_offsets,
                                    sample_offset, dv->delta_offset_bits);
    val += elias_gamma_prefix_sum(dv->deltas, delta_offset, delta_offset_idx);
    return val;
}
