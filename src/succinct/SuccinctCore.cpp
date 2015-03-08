#include "succinct/SuccinctCore.hpp"

SuccinctCore::SuccinctCore(const char *filename,
                            bool construct_succinct,
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

    if(construct_succinct) {
        construct(filename, sa_sampling_rate, isa_sampling_rate, npa_sampling_rate,
            context_len, sa_sampling_scheme, isa_sampling_scheme,
            npa_encoding_scheme, sampling_range);
    } else {
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

        std::string s_path = std::string(filename) + ".succinct";
        std::ifstream s_file(s_path);
        deserialize(s_file);
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

size_t SuccinctCore::serialize(std::ostream& out) {
    size_t out_size = 0;
    typedef std::map<char, std::pair<uint64_t, uint32_t> >::iterator iterator_t;

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

    out_size += SA->serialize(out);
    out_size += ISA->serialize(out);

    if(SA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
        assert(ISA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
        out_size += serialize_dictionary(((SampledByValueSA *)SA)->get_d_bpos(), out);
    }

    out_size += npa->serialize(out);

    return out_size;
}

size_t SuccinctCore::deserialize(std::istream& in) {
    size_t in_size = 0;

    // Read size of input file
    in.read(reinterpret_cast<char *>(&(input_size)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);

    // Read cmap size
    uint64_t cmap_size;
    in.read(reinterpret_cast<char *>(&(cmap_size)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);
    for(uint64_t i = 0; i < cmap_size; i++) {
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

    in.read(reinterpret_cast<char *>(&alphabet_size), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    alphabet = new char[alphabet_size + 1];
    for(uint32_t i = 0; i < alphabet_size + 1; i++) {
        in.read(reinterpret_cast<char *>(&alphabet[i]), sizeof(char));
    }

    in_size += SA->deserialize(in);
    in_size += ISA->deserialize(in);

    if(SA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
    	Dictionary *d_bpos = new Dictionary;
        assert(ISA->get_sampling_scheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
        in_size += deserialize_dictionary(&d_bpos, in);
        ((SampledByValueSA *)SA)->set_d_bpos(d_bpos);
        ((SampledByValueISA *)ISA)->set_d_bpos(d_bpos);
    }

    switch(npa->get_encoding_scheme()) {
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
        in_size += ((EliasDeltaEncodedNPA *)npa)->deserialize(in);
        break;
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
        in_size += ((EliasGammaEncodedNPA *)npa)->deserialize(in);
        break;
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
        in_size += ((WaveletTreeEncodedNPA *)npa)->deserialize(in);
        break;
    default:
        assert(0);
    }

    // TODO: Fix
    Cinv_idx.insert(Cinv_idx.end(), npa->col_offsets.begin() + 1, npa->col_offsets.end());

    return in_size;
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

SampledArray *SuccinctCore::getSA() {
    return SA;
}

SampledArray *SuccinctCore::getISA() {
    return ISA;
}
