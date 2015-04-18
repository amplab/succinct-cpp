#ifndef SUCCINCT_CORE_H
#define SUCCINCT_CORE_H

#include <vector>
#include <fstream>

#include "succinct/npa/NPA.hpp"
#include "succinct/npa/EliasGammaEncodedNPA.hpp"
#include "succinct/npa/EliasDeltaEncodedNPA.hpp"
#include "succinct/npa/WaveletTreeEncodedNPA.hpp"

#include "succinct/sampledarray/FlatSampledArray.hpp"
#include "succinct/sampledarray/LayeredSampledArray.hpp"
#include "succinct/sampledarray/OpportunisticLayeredSampledArray.hpp"
#include "succinct/sampledarray/SampledByIndexSA.hpp"
#include "succinct/sampledarray/SampledByIndexISA.hpp"
#include "succinct/sampledarray/SampledByValueSA.hpp"
#include "succinct/sampledarray/SampledByValueISA.hpp"
#include "succinct/sampledarray/LayeredSampledSA.hpp"
#include "succinct/sampledarray/LayeredSampledISA.hpp"
#include "succinct/sampledarray/OpportunisticLayeredSampledSA.hpp"
#include "succinct/sampledarray/OpportunisticLayeredSampledISA.hpp"

#include "succinct/utils/divsufsortxx.hpp"
#include "succinct/utils/divsufsortxx_utility.hpp"

#include "succinct/SuccinctBase.hpp"

typedef enum {
    CONSTRUCT_IN_MEMORY = 0,
    CONSTRUCT_MEMORY_MAPPED = 1,
    LOAD_IN_MEMORY = 2,
    LOAD_MEMORY_MAPPED = 3
} SuccinctMode;

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
                SuccinctMode s_mode = SuccinctMode::CONSTRUCT_IN_MEMORY,
                uint32_t sa_sampling_rate = 32,
                uint32_t isa_sampling_rate = 32,
                uint32_t npa_sampling_rate = 128,
                uint32_t context_len = 3,
                SamplingScheme sa_sampling_scheme =
                        SamplingScheme::FLAT_SAMPLE_BY_INDEX,
                SamplingScheme isa_sampling_scheme =
                        SamplingScheme::FLAT_SAMPLE_BY_INDEX,
                NPA::NPAEncodingScheme npa_encoding_scheme =
                        NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED,
                uint32_t sampling_range = 1024);

    virtual ~SuccinctCore() {}

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

    // Memory map succinct data structures
    size_t memorymap(std::string filename);

    // Get size of original input
    uint64_t original_size();

    // Get succinct size
    virtual size_t storage_size();

    virtual void print_storage_breakdown();

    // Get SA
    SampledArray *getSA();

    // Get ISA
    SampledArray *getISA();

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
            NPA::NPAEncodingScheme npa_encoding_scheme,
            uint32_t sampling_range);

    // Helper functions
    bool compare_data_bitmap(BitMap *T, uint64_t i, uint64_t j, uint64_t k);
    uint64_t get_context_val(BitMap *T, uint32_t i);

    bool is_sampled(uint64_t i);
};
#endif
