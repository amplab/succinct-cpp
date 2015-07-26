#ifndef WAVELET_TREE_ENCODED_NPA_H
#define WAVELET_TREE_ENCODED_NPA_H

#include "npa/NPA.hpp"

class WaveletTreeEncodedNPA : public NPA {

public:
    typedef SuccinctBase::Dictionary dictionary_t;

    typedef struct _wletnode {
        dictionary_t D;
        char id;
        struct _wletnode *lt, *rt;
    } WaveletNode;

protected:
    // Create wavelet tree
    void create_wavelettree(WaveletNode **w, uint64_t s, uint64_t e,
            std::vector<uint64_t> &v, std::vector<uint64_t> &v_c);

    // Lookup wavelet tree
    uint64_t lookup_wavelettree(WaveletNode *tree, uint64_t c_pos,
            uint64_t sl_pos, uint64_t s, uint64_t e);

public:
    // Constructor
    WaveletTreeEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
            uint32_t context_len, uint32_t sampling_rate, bitmap_t *data_bitmap,
            bitmap_t *compactSA, bitmap_t *compactISA,
            SuccinctAllocator &s_allocator, SuccinctBase *s_base);

    WaveletTreeEncodedNPA(uint32_t context_len, uint32_t sampling_rate,
            SuccinctAllocator &s_allocator, SuccinctBase *s_base);

    // Virtual destructor
    ~WaveletTreeEncodedNPA() { }

    // Encode EliasDeltaEncodedNPA
    virtual void encode(bitmap_t *data_bitmap, bitmap_t *compactSA,
                            bitmap_t *compactISA);

    // Access element at index i
    virtual uint64_t operator[](uint64_t i);

    // Serialize the wavelet tree enoded NPA
    virtual size_t serialize(std::ostream& out);

    // Deserialize the wavelet tree encoded NPA
    virtual size_t deserialize(std::istream& in);

    // Memory map the wavelet tree encoded NPA
    virtual size_t memorymap(std::string filename);

    virtual size_t storage_size();

private:
    SuccinctBase *s_base;
    uint64_t *csizes;
    WaveletNode **wtree;

};

#endif
