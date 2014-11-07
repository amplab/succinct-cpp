#ifndef SUCCINCT_CORE_H
#define SUCCINCT_CORE_H

#include <vector>
#include <fstream>

#include "succinct/npa/NPA.hpp"
#include "succinct/npa/EliasGammaEncodedNPA.hpp"
#include "succinct/npa/EliasDeltaEncodedNPA.hpp"
#include "succinct/npa/WaveletTreeEncodedNPA.hpp"

#include "succinct/sampledarray/SampledArray.hpp"
#include "succinct/sampledarray/SampledByIndexSA.hpp"
#include "succinct/sampledarray/SampledByIndexISA.hpp"
#include "succinct/sampledarray/SampledByValueSA.hpp"
#include "succinct/sampledarray/SampledByValueISA.hpp"

#include "succinct/utils/divsufsortxx.hpp"
#include "succinct/utils/divsufsortxx_utility.hpp"

#include "succinct/SuccinctBase.hpp"

class SuccinctCore : public SuccinctBase {
private:
    typedef std::map<char, std::pair<uint64_t, uint32_t>> alphabet_map_t;
protected:

    uint64_t input_size;                // Size of input

    /* Primary data structures */
    SampledArray *SA;                   // Suffix Array
    SampledArray *ISA;                  // Inverse Suffix Array
    NPA *npa;                           // Next Pointer Array
    std::vector<uint64_t> Cinv_idx;     // Indexes into Cinv;

    /* Auxiliary data structures */
    char *alphabet;
    alphabet_map_t alphabet_map;
    uint32_t alphabet_size;             // Size of the input alphabet

public:
    /* Constructors */
    SuccinctCore(const char *filename,
                bool construct_succinct = true,
                uint32_t sa_sampling_rate = 32,
                uint32_t isa_sampling_rate = 32,
                uint32_t npa_sampling_rate = 128,
                uint32_t context_len = 3,
                SamplingScheme sa_sampling_scheme =
                        SamplingScheme::SAMPLE_BY_INDEX,
                SamplingScheme isa_sampling_scheme =
                        SamplingScheme::SAMPLE_BY_INDEX,
                NPA::NPAEncodingScheme npa_encoding_scheme =
                        NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED);

    /* Lookup functions for each of the core data structures */
    // Lookup NPA at index i
    uint64_t lookupNPA(uint64_t i);

    // Lookup SA at index i
    uint64_t lookupSA(uint64_t i);

    // Lookup ISA at index i
    uint64_t lookupISA(uint64_t i);

    // Get index of value v in C
    uint64_t lookupC(uint64_t val);

    // Serialize succinct data structures
    size_t serialize(std::ostream& out);

    // Deserialize succinct data structures
    size_t deserialize(std::istream& in);

    // Get size of original input
    uint64_t original_size();

    // Get succinct size
    uint64_t size();

private:
    /* Construct functions */
    // Create all auxiliary data structures
    void construct_aux(BitMap *compactSA, const char *data);

    // Parent construct function
    void construct(const char* filename,
            uint32_t sa_sampling_rate,
            uint32_t isa_sampling_rate,
            uint32_t npa_sampling_rate,
            uint32_t context_len,
            SamplingScheme sa_sampling_scheme,
            SamplingScheme isa_sampling_scheme,
            NPA::NPAEncodingScheme npa_encoding_scheme);

    // Helper functions
    bool compare_data_bitmap(BitMap *T, uint64_t i, uint64_t j, uint64_t k);
    uint64_t get_context_val(BitMap *T, uint32_t i);

    bool is_sampled(uint64_t i);
};
#endif
