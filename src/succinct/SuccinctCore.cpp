#include "succinct/SuccinctCore.hpp"

SuccinctCore::SuccinctCore(const char *filename,
                            SuccinctMode s_mode,
                            uint32_t sa_sampling_rate,
                            uint32_t isa_sampling_rate,
                            uint32_t npa_sampling_rate,
                            uint32_t context_len,
                            SamplingScheme sa_sampling_scheme,
                            SamplingScheme isa_sampling_scheme,
                            NPA::NPAEncodingScheme npa_encoding_scheme,
                            uint32_t sampling_range) :
                            SuccinctBase() {

    this->alphabet = NULL;
    this->SA = NULL;
    this->ISA = NULL;
    this->npa = NULL;
    this->alphabet_size = 0;
    this->input_size = 0;
    this->filename = std::string(filename);
    this->succinct_path = this->filename + ".succinct";
    switch(s_mode) {
    case SuccinctMode::CONSTRUCT_IN_MEMORY:
    {
        construct(filename, sa_sampling_rate, isa_sampling_rate, npa_sampling_rate,
                    context_len, sa_sampling_scheme, isa_sampling_scheme,
                    npa_encoding_scheme, sampling_range);
        break;
    }
    case SuccinctMode::CONSTRUCT_MEMORY_MAPPED:
    {
        fprintf(stderr, "Unsupported mode.\n");
        assert(0);
        break;
    }
    case SuccinctMode::LOAD_IN_MEMORY:
    {
        switch(npa_encoding_scheme) {
        case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
            npa = new EliasGammaEncodedNPA(context_len, npa_sampling_rate,
                    s_allocator);
            break;
        case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
            npa = new EliasDeltaEncodedNPA(context_len, npa_sampling_rate,
                    s_allocator);
            return;
        case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
            npa = new WaveletTreeEncodedNPA(context_len, npa_sampling_rate,
                    s_allocator, this);
            break;
        default:
            npa = NULL;
        }

        assert(npa != NULL);

        switch(sa_sampling_scheme) {
        case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
            SA = new SampledByIndexSA(sa_sampling_rate, npa, s_allocator);
            break;
        case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
            SA = new SampledByValueSA(sa_sampling_rate, npa, s_allocator, this);
            break;
        case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
            SA = new LayeredSampledSA(sa_sampling_rate, sa_sampling_rate * sampling_range,
                    npa, s_allocator);
            break;
        case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
            SA = new OpportunisticLayeredSampledSA(sa_sampling_rate, sa_sampling_rate * sampling_range,
                                npa, s_allocator);
            break;
        default:
            SA = NULL;
        }

        assert(SA != NULL);

        switch(isa_sampling_scheme) {
        case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
            ISA = new SampledByIndexISA(isa_sampling_rate, npa, s_allocator);
            break;
        case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
            assert(SA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
            ISA = new SampledByValueISA(sa_sampling_rate, npa, s_allocator, this);
            break;
        case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
            ISA = new LayeredSampledISA(isa_sampling_rate, isa_sampling_rate * sampling_range,
                    npa, s_allocator);
            break;
        case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
            ISA = new OpportunisticLayeredSampledISA(isa_sampling_rate, isa_sampling_rate * sampling_range,
                    npa, s_allocator);
            break;
        default:
            ISA = NULL;
        }

        assert(ISA != NULL);

        deserialize();
        break;
    }
    case SuccinctMode::LOAD_MEMORY_MAPPED:
    {
        switch(npa_encoding_scheme) {
        case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
            npa = new EliasGammaEncodedNPA(context_len, npa_sampling_rate,
                    s_allocator);
            break;
        case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
            npa = new EliasDeltaEncodedNPA(context_len, npa_sampling_rate,
                    s_allocator);
            return;
        case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
            npa = new WaveletTreeEncodedNPA(context_len, npa_sampling_rate,
                    s_allocator, this);
            break;
        default:
            npa = NULL;
        }

        assert(npa != NULL);

        switch(sa_sampling_scheme) {
        case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
            SA = new SampledByIndexSA(sa_sampling_rate, npa, s_allocator);
            break;
        case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
            SA = new SampledByValueSA(sa_sampling_rate, npa, s_allocator, this);
            break;
        case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
            SA = new LayeredSampledSA(sa_sampling_rate, sa_sampling_rate * sampling_range,
                    npa, s_allocator);
            break;
        case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
            SA = new OpportunisticLayeredSampledSA(sa_sampling_rate, sa_sampling_rate * sampling_range,
                                npa, s_allocator);
            break;
        default:
            SA = NULL;
        }

        assert(SA != NULL);

        switch(isa_sampling_scheme) {
        case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
            ISA = new SampledByIndexISA(isa_sampling_rate, npa, s_allocator);
            break;
        case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
            assert(SA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
            ISA = new SampledByValueISA(sa_sampling_rate, npa, s_allocator, this);
            break;
        case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
            ISA = new LayeredSampledISA(isa_sampling_rate, isa_sampling_rate * sampling_range,
                    npa, s_allocator);
            break;
        case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
            ISA = new OpportunisticLayeredSampledISA(isa_sampling_rate, isa_sampling_rate * sampling_range,
                    npa, s_allocator);
            break;
        default:
            ISA = NULL;
        }

        assert(ISA != NULL);

        memorymap();
        break;
    }
    }

}

/* Primary Construct function */
void SuccinctCore::construct(const char* filename,
        uint32_t sa_sampling_rate,
        uint32_t isa_sampling_rate,
        uint32_t npa_sampling_rate,
        uint32_t context_len,
        SamplingScheme sa_sampling_scheme,
        SamplingScheme isa_sampling_scheme,
        NPA::NPAEncodingScheme npa_encoding_scheme,
        uint32_t sampling_range) {

    // Read input from file
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    uint64_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = (char *) s_allocator.s_malloc(fsize + 1);
    size_t felems = fread(data, fsize, 1, f);
    fclose(f);

    assert(felems == 1);

    data[fsize] = (char)1;

    input_size = fsize + 1;
    uint32_t bits = SuccinctUtils::int_log_2(input_size + 1);

    // Construct Suffix Array
    // TODO: Remove dependency from divsufsortxx library
    int64_t *lSA = (int64_t *) s_allocator.s_calloc(sizeof(int64_t), input_size);
    divsufsortxx::constructSA((uint8_t *)data, (uint8_t *)(data + input_size), lSA,
                                lSA + input_size, 256);

    // Compact SA
    BitMap *compactSA = new BitMap;
    create_bitmap_array(&compactSA, (uint64_t *)lSA, input_size, bits, s_allocator);
    s_allocator.s_free(lSA);

    construct_aux(compactSA, data);

    // Compact input data
    BitMap *data_bitmap = new BitMap;
    int sigma_bits = SuccinctUtils::int_log_2(alphabet_size + 1);
    init_bitmap(&data_bitmap, input_size * sigma_bits, s_allocator);
    for(uint64_t i = 0; i < input_size; i++) {
        set_bitmap_array(&data_bitmap, i, alphabet_map[data[i]].second, sigma_bits);
    }

    s_allocator.s_free(data);

    BitMap *compactISA = new BitMap;
    init_bitmap(&compactISA, input_size * bits, s_allocator);

    for(uint64_t i = 0; i < input_size; i++) {
        uint64_t sa_val = lookup_bitmap_array(compactSA, i, bits);
        set_bitmap_array(&compactISA, sa_val, i, bits);
    }

    switch(npa_encoding_scheme) {
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
        npa = new EliasGammaEncodedNPA(input_size, alphabet_size, context_len, npa_sampling_rate,
                            data_bitmap, compactSA, compactISA, s_allocator);
        break;
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
        npa = new EliasDeltaEncodedNPA(input_size, alphabet_size, context_len, npa_sampling_rate,
                data_bitmap, compactSA, compactISA, s_allocator);
        return;
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
        npa = new WaveletTreeEncodedNPA(input_size, alphabet_size, context_len, npa_sampling_rate,
                data_bitmap, compactSA, compactISA, s_allocator, this);
        break;
    default:
        npa = NULL;
    }

    assert(npa != NULL);

    destroy_bitmap(&compactISA, s_allocator);
    destroy_bitmap(&data_bitmap, s_allocator);

    switch(sa_sampling_scheme) {
    case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
        SA = new SampledByIndexSA(sa_sampling_rate, npa, compactSA, input_size, s_allocator);
        break;
    case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
        SA = new SampledByValueSA(sa_sampling_rate, npa, compactSA, input_size, s_allocator, this);
        break;
    case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
        SA = new LayeredSampledSA(sa_sampling_rate, sa_sampling_rate * sampling_range,
                npa, compactSA, input_size, s_allocator);
        break;
    case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
        SA = new OpportunisticLayeredSampledSA(sa_sampling_rate, sa_sampling_rate * sampling_range,
                npa, compactSA, input_size, s_allocator);
        break;
    default:
        SA = NULL;
    }

    assert(SA != NULL);

    switch(isa_sampling_scheme) {
    case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
        ISA = new SampledByIndexISA(isa_sampling_rate, npa, compactSA, input_size, s_allocator);
        break;
    case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
        assert(SA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
        ISA = new SampledByValueISA(sa_sampling_rate, npa, compactSA, input_size,
                ((SampledByValueSA *)SA)->get_d_bpos(), s_allocator, this);
        break;
    case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
        ISA = new LayeredSampledISA(isa_sampling_rate, isa_sampling_rate * sampling_range,
                npa, compactSA, input_size, s_allocator);
        break;
    case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
        ISA = new OpportunisticLayeredSampledISA(isa_sampling_rate, isa_sampling_rate * sampling_range,
                npa, compactSA, input_size, s_allocator);
        break;
    default:
        ISA = NULL;
    }

    assert(ISA != NULL);

    destroy_bitmap(&compactSA, s_allocator);
}

void SuccinctCore::construct_aux(BitMap *compactSA, const char *data) {
    uint32_t bits = SuccinctUtils::int_log_2(input_size + 1);
    alphabet_size = 1;

    for (uint64_t i = 1; i < input_size; ++i) {
        if (data[lookup_bitmap_array(compactSA, i, bits)] !=
                data[lookup_bitmap_array(compactSA, i - 1, bits)]) {
            Cinv_idx.push_back(i);
            alphabet_size++;
        }
    }

    alphabet = new char[alphabet_size + 1];

    alphabet[0] = data[lookup_bitmap_array(compactSA, 0, bits)];
    alphabet_map[alphabet[0]] = std::pair<uint64_t, uint32_t>(0, 0);
    uint64_t i;

    for (i = 1; i < alphabet_size; i++) {
        uint64_t sel = Cinv_idx[i - 1];
        alphabet[i] = data[lookup_bitmap_array(compactSA, sel, bits)];
        alphabet_map[alphabet[i]] = std::pair<uint64_t, uint32_t>(sel, i);
    }
    alphabet_map[(char)0] = std::pair<uint64_t, uint32_t> (input_size, i);
    alphabet[i] = (char)0;
}

/* Lookup functions for each of the core data structures */
// Lookup NPA at index i
uint64_t SuccinctCore::lookupNPA(uint64_t i) {
    return (*npa)[i];
}

// Lookup SA at index i
uint64_t SuccinctCore::lookupSA(uint64_t i) {
    return (*SA)[i];
}

// Lookup ISA at index i
uint64_t SuccinctCore::lookupISA(uint64_t i) {
    return (*ISA)[i];
}

// Lookup C at index i
uint64_t SuccinctCore::lookupC(uint64_t i) {
    return get_rank1(&Cinv_idx, i);
}

size_t SuccinctCore::serialize() {
    size_t out_size = 0;
    typedef std::map<char, std::pair<uint64_t, uint32_t> >::iterator iterator_t;
    struct stat st;
    if (stat(succinct_path.c_str(), &st) != 0) {
        if (mkdir(succinct_path.c_str(), (mode_t)(S_IRWXU | S_IRGRP |  S_IXGRP | S_IROTH | S_IXOTH)) != 0) {
            fprintf(stderr,
                "succinct dir '%s' does not exist, and failed to mkdir (no space or permission?)\n",
                this->succinct_path.c_str());
            fprintf(stderr, "terminating the serialization process.\n");
            return 1;
        }
    }
    std::ofstream out(succinct_path + "/metadata");
    std::ofstream sa_out(succinct_path + "/sa");
    std::ofstream isa_out(succinct_path + "/isa");
    std::ofstream npa_out(succinct_path + "/npa");

    // Output size of input file
    out.write(reinterpret_cast<const char *>(&(input_size)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);

    // Output cmap size
    uint64_t cmap_size = alphabet_map.size();
    out.write(reinterpret_cast<const char *>(&(cmap_size)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
    for(iterator_t it = alphabet_map.begin(); it != alphabet_map.end(); ++it) {
        out.write(reinterpret_cast<const char *>(&(it->first)), sizeof(char));
        out_size += sizeof(char);
        out.write(reinterpret_cast<const char *>(&(it->second.first)), sizeof(uint64_t));
        out_size += sizeof(uint64_t);
        out.write(reinterpret_cast<const char *>(&(it->second.second)), sizeof(uint32_t));
        out_size += sizeof(uint32_t);
    }

    out.write(reinterpret_cast<const char *>(&alphabet_size), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
    for(uint32_t i = 0; i < alphabet_size + 1; i++) {
        out.write(reinterpret_cast<const char *>(&alphabet[i]), sizeof(char));
    }

    out_size += SA->serialize(sa_out);
    out_size += ISA->serialize(isa_out);

    if(SA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
        assert(ISA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
        out_size += serialize_dictionary(((SampledByValueSA *)SA)->get_d_bpos(), out);
    }

    out_size += npa->serialize(npa_out);

    out.close();
    sa_out.close();
    isa_out.close();
    npa_out.close();

    return out_size;
}

size_t SuccinctCore::deserialize() {
    // Check if directory exists
    struct stat st;
    assert(stat(succinct_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

    std::ifstream in(succinct_path + "/metadata");
    std::ifstream sa_in(succinct_path + "/sa");
    std::ifstream isa_in(succinct_path + "/isa");
    std::ifstream npa_in(succinct_path + "/npa");

    size_t in_size = 0;

    // Read size of input file
    in.read(reinterpret_cast<char *>(&(input_size)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    // Read alphabet map size
    uint64_t alphabet_map_size;
    in.read(reinterpret_cast<char *>(&(alphabet_map_size)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    // Deserialize alphabet map
    for(uint64_t i = 0; i < alphabet_map_size; i++) {
    	char first;
    	uint64_t second_first;
    	uint32_t second_second;
        in.read(reinterpret_cast<char *>(&(first)), sizeof(char));
        in_size += sizeof(char);
        in.read(reinterpret_cast<char *>(&(second_first)), sizeof(uint64_t));
        in_size += sizeof(uint64_t);
        in.read(reinterpret_cast<char *>(&(second_second)), sizeof(uint32_t));
        in_size += sizeof(uint32_t);
        alphabet_map[first] = std::pair<uint64_t, uint32_t>(second_first, second_second);
    }

    // Read alphabet size
    in.read(reinterpret_cast<char *>(&alphabet_size), sizeof(uint32_t));
    in_size += sizeof(uint32_t);

    // Deserialize alphabet
    alphabet = new char[alphabet_size + 1];
    for(uint32_t i = 0; i < alphabet_size + 1; i++) {
        in.read(reinterpret_cast<char *>(&alphabet[i]), sizeof(char));
    }

    // Deserialize SA, ISA
    in_size += SA->deserialize(sa_in);
    in_size += ISA->deserialize(isa_in);

    // Deserialize bitmap marking positions of sampled values if the sampling scheme
    // is sample by value.
    if(SA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
    	Dictionary *d_bpos = new Dictionary;
        assert(ISA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
        in_size += deserialize_dictionary(&d_bpos, in);
        ((SampledByValueSA *)SA)->set_d_bpos(d_bpos);
        ((SampledByValueISA *)ISA)->set_d_bpos(d_bpos);
    }

    // Deserialize NPA based on the NPA encoding scheme.
    switch(npa->get_encoding_scheme()) {
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
        in_size += ((EliasDeltaEncodedNPA *)npa)->deserialize(npa_in);
        break;
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
        in_size += ((EliasGammaEncodedNPA *)npa)->deserialize(npa_in);
        break;
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
        in_size += ((WaveletTreeEncodedNPA *)npa)->deserialize(npa_in);
        break;
    default:
        assert(0);
    }

    // TODO: Fix
    Cinv_idx.insert(Cinv_idx.end(), npa->col_offsets.begin() + 1, npa->col_offsets.end());

    in.close();
    sa_in.close();
    isa_in.close();
    npa_in.close();

    return in_size;
}

size_t SuccinctCore::memorymap() {
    // Check if directory exists
    struct stat st;
    assert(stat(succinct_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

    uint8_t *data_beg, *data;
    data = data_beg = (uint8_t *)SuccinctUtils::memory_map(succinct_path + "/metadata");

    input_size = *((uint64_t *)data);
    data += sizeof(uint64_t);

    // Read alphabet_map size
    uint64_t alphabet_map_size = *((uint64_t *)data);
    data += sizeof(uint64_t);

    // Deserialize map, because we don't know how to have serialized,
    // memory mapped maps yet. This is fine, since the map size is
    // pretty small.
    for(uint64_t i = 0; i < alphabet_map_size; i++) {
        char first = *((char *)data);
        data += sizeof(char);
        uint64_t second_first = *((uint64_t *)data);
        data += sizeof(uint64_t);
        uint32_t second_second = *((uint32_t *)data);
        data += sizeof(uint32_t);
        alphabet_map[first] = std::pair<uint64_t, uint32_t>(second_first, second_second);
    }

    // Read alphabet size
    alphabet_size = *((uint32_t *)data);
    data += sizeof(uint32_t);

    // Read alphabet
    alphabet = (char *)data;
    data += (sizeof(char) * (alphabet_size + 1));

    // Memory map SA and ISA
    data += SA->memorymap(succinct_path + "/sa");
    data += ISA->memorymap(succinct_path + "/isa");

    // Memory map bitmap marking positions of sampled values if the sampling scheme
    // is sample by value.
    if(SA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
        Dictionary *d_bpos;
        assert(ISA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
        data += memorymap_dictionary(&d_bpos, data);
        ((SampledByValueSA *)SA)->set_d_bpos(d_bpos);
        ((SampledByValueISA *)ISA)->set_d_bpos(d_bpos);
    }

    // Memory map NPA based on the NPA encoding scheme.
    switch(npa->get_encoding_scheme()) {
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
        data += ((EliasDeltaEncodedNPA *)npa)->memorymap(succinct_path + "/npa");
        break;
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
        data += ((EliasGammaEncodedNPA *)npa)->memorymap(succinct_path + "/npa");
        break;
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
        data += ((WaveletTreeEncodedNPA *)npa)->memorymap(succinct_path + "/npa");
        break;
    default:
        assert(0);
    }

    // TODO: Fix
    Cinv_idx.insert(Cinv_idx.end(), npa->col_offsets.begin() + 1, npa->col_offsets.end());

    return data - data_beg;
}

uint64_t SuccinctCore::original_size() {
    return input_size;
}

size_t SuccinctCore::storage_size() {
    size_t tot_size = SuccinctBase::storage_size();
    tot_size += sizeof(uint64_t);
    tot_size += sizeof(alphabet_map.size()) +
            alphabet_map.size() * (sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t));
    tot_size += sizeof(alphabet_size) + alphabet_size * sizeof(char);
    tot_size += SA->storage_size();
    tot_size += ISA->storage_size();
    tot_size += npa->storage_size();
    return tot_size;
}

void SuccinctCore::print_storage_breakdown() {
    size_t metadata_size = SuccinctBase::storage_size();
    metadata_size += sizeof(uint64_t);
    metadata_size += sizeof(alphabet_map.size()) +
            alphabet_map.size() * (sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t));
    metadata_size += sizeof(alphabet_size) + alphabet_size * sizeof(char);
    fprintf(stderr, "Metadata size = %zu\n", metadata_size);
    fprintf(stderr, "SA size = %zu\n", SA->storage_size());
    fprintf(stderr, "ISA size = %zu\n", ISA->storage_size());
    fprintf(stderr, "NPA size = %zu\n", npa->storage_size());
}

std::pair<int64_t, int64_t> SuccinctCore::bw_search(std::string mgram) {
    std::pair<int64_t, int64_t> range(0, -1), col_range;
    uint64_t m_len = mgram.length();

    if (alphabet_map.find(mgram[m_len - 1]) != alphabet_map.end()) {
        range.first = (alphabet_map[mgram[m_len - 1]]).first;
        range.second = alphabet_map[alphabet[alphabet_map[mgram[m_len - 1]].second + 1]].first - 1;
    } else return std::pair<int64_t, int64_t>(0, -1);

    for (int64_t i = m_len - 2; i >= 0; i--) {
        if (alphabet_map.find(mgram[i]) != alphabet_map.end()) {
            col_range.first = alphabet_map[mgram[i]].first;
            col_range.second = alphabet_map[alphabet[alphabet_map[mgram[i]].second + 1]].first - 1;
        } else return std::pair<int64_t, int64_t>(0, -1);

        if(col_range.first > col_range.second) return std::pair<int64_t, int64_t>(0, -1);

        range.first = npa->binary_search_npa(range.first, col_range.first, col_range.second, false);
        range.second = npa->binary_search_npa(range.second, col_range.first, col_range.second, true);

        if(range.first > range.second) return range;
    }

    return range;
}

std::pair<int64_t, int64_t> SuccinctCore::continue_bw_search(std::string mgram, std::pair<int64_t, int64_t> range) {
    std::pair<int64_t, int64_t> col_range;
    uint64_t m_len = mgram.length();

    for (int64_t i = m_len - 1; i >= 0; i--) {
        if (alphabet_map.find(mgram[i]) != alphabet_map.end()) {
            col_range.first = alphabet_map[mgram[i]].first;
            col_range.second = alphabet_map[alphabet[alphabet_map[mgram[i]].second + 1]].first - 1;
        } else return std::pair<int64_t, int64_t>(0, -1);

        if(col_range.first > col_range.second) return std::pair<int64_t, int64_t>(0, -1);

        range.first = npa->binary_search_npa(range.first, col_range.first, col_range.second, false);
        range.second = npa->binary_search_npa(range.second, col_range.first, col_range.second, true);

        if(range.first > range.second) return range;
    }

    return range;
}

int SuccinctCore::compare(std::string p, int64_t i) {
    long j = 0;
    do {
        char c = alphabet[lookupC(i)];
        if (p[j] < c) {
            return -1;
        } else if(p[j] > c) {
            return 1;
        }
        i = lookupNPA(i);
        j++;
    } while (j < p.length());
    return 0;
}

int SuccinctCore::compare(std::string p, int64_t i, size_t offset) {

    long j = 0;

    // Skip first offset chars
    while(offset) {
        i = lookupNPA(i);
        offset--;
    }

    do {
        char c = alphabet[lookupC(i)];
        if (p[j] < c) {
            return -1;
        } else if(p[j] > c) {
            return 1;
        }
        i = lookupNPA(i);
        j++;
    } while (j < p.length());

    return 0;
}

std::pair<int64_t, int64_t> SuccinctCore::fw_search(std::string mgram) {

    int64_t st = original_size() - 1;
    int64_t sp = 0;
    int64_t s;
    while(sp < st) {
        s = (sp + st) / 2;
        if (compare(mgram, s) > 0) sp = s + 1;
        else st = s;
    }

    int64_t et = original_size() - 1;
    int64_t ep = sp - 1;
    int64_t e;

     while (ep < et) {
        e = ceil((double)(ep + et) / 2);
        if (compare(mgram, e) == 0) ep = e;
        else et = e - 1;
    }

    return std::pair<int64_t, int64_t>(sp, ep);
}

std::pair<int64_t, int64_t> SuccinctCore::continue_fw_search(std::string mgram,
        std::pair<int64_t, int64_t> range, size_t len) {

    if(mgram == "") return range;

    int64_t st = range.second;
    int64_t sp = range.first;
    int64_t s;
    while(sp < st) {
        s = (sp + st) / 2;
        if (compare(mgram, s, len) > 0) sp = s + 1;
        else st = s;
    }

    int64_t et = range.second;
    int64_t ep = sp - 1;
    int64_t e;

    while (ep < et) {
        e = ceil((double)(ep + et) / 2);
        if (compare(mgram, e, len) == 0) ep = e;
        else et = e - 1;
    }

    return std::pair<int64_t, int64_t>(sp, ep);
}

SampledArray *SuccinctCore::getSA() {
    return SA;
}

SampledArray *SuccinctCore::getISA() {
    return ISA;
}

NPA *SuccinctCore::getNPA() {
    return npa;
}

char *SuccinctCore::getAlphabet() {
    return alphabet;
}
