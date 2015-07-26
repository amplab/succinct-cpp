#ifndef ELIAS_GAMMA_ENCODED_NPA_H
#define ELIAS_GAMMA_ENCODED_NPA_H

#include "npa/DeltaEncodedNPA.hpp"

#define USE_BSR             0
#define USE_PREFIXSUM_TABLE 1

/* Gamma prefix sum table functions */
#define PREFIX_OFF(i)   ((prefixsum[(i)] >> 24) & 0xFF)
#define PREFIX_CNT(i)   ((prefixsum[i] >> 16) & 0xFF)
#define PREFIX_SUM(i)   (prefixsum[i] & 0xFFFF)

class EliasGammaEncodedNPA : public DeltaEncodedNPA {
private:
    // Pre-computed prefix sums for elias gamma encoded NPA
    uint32_t prefixsum[65536];

protected:
    // Create elias-gamma delta encoded vector
    virtual void createDEV(DeltaEncodedVector *dv,
            std::vector<uint64_t> &data);

    // Lookup elias-gamma delta encoded vector at index i
    virtual uint64_t lookupDEV(DeltaEncodedVector *dv, uint64_t i);

public:
    EliasGammaEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
            uint32_t context_len, uint32_t sampling_rate, bitmap_t *data_bitmap,
            bitmap_t *compactSA, bitmap_t *compactISA,
            SuccinctAllocator &s_allocator);

    EliasGammaEncodedNPA(uint32_t context_len, uint32_t sampling_rate,
            SuccinctAllocator &s_allocator);

    // Virtual destructor
    ~EliasGammaEncodedNPA() { }

    // Compute the elias gamma encoding size in bits for a 64 bit integer
    static uint32_t elias_gamma_encoding_size(uint64_t n);

    /*
    virtual int64_t binary_search_npa(uint64_t val, uint64_t s, uint64_t e,
    									bool flag);
    */

private:
    // Accesses data from a 64 bit integer represented as a bit map
    // from a specified position and for a specified number of bits
    uint16_t access_data_pos16(uint16_t data, uint32_t pos, uint32_t b);

    int64_t binary_search_samples(DeltaEncodedVector *dv, uint64_t val, uint64_t s, uint64_t e);

    // Initialize pre-computed prefix sums
    void init_prefixsum();

    // Encode a sorted vector using elias-gamma encoding
    void elias_gamma_encode(bitmap_t **B, std::vector<uint64_t> &deltas,
                            uint64_t size);

    // Decode a particular elias-gamma encoded delta value at a provided offset
    // in the deltas bitmap
    uint64_t elias_gamma_decode(bitmap_t *B, uint64_t *offset);

    // Compute the prefix sum of elias-gamma encoded delta values
    uint64_t elias_gamma_prefix_sum(bitmap_t *B, uint64_t offset, uint64_t i);

};

#endif
