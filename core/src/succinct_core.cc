#include "succinct_core.h"

SuccinctCore::SuccinctCore(const char *filename, SuccinctMode s_mode,
                           uint32_t sa_sampling_rate,
                           uint32_t isa_sampling_rate,
                           uint32_t npa_sampling_rate, uint32_t context_len,
                           SamplingScheme sa_sampling_scheme,
                           SamplingScheme isa_sampling_scheme,
                           NPA::NPAEncodingScheme npa_encoding_scheme,
                           uint32_t sampling_range)
    : SuccinctBase() {

  this->alphabet_ = NULL;
  this->sa_ = NULL;
  this->isa_ = NULL;
  this->npa_ = NULL;
  this->alphabet_size_ = 0;
  this->input_size_ = 0;
  this->filename_ = std::string(filename);
  this->succinct_path_ = this->filename_ + ".succinct";
  switch (s_mode) {
    case SuccinctMode::CONSTRUCT_IN_MEMORY: {
      Construct(filename, sa_sampling_rate, isa_sampling_rate,
                npa_sampling_rate, context_len, sa_sampling_scheme,
                isa_sampling_scheme, npa_encoding_scheme, sampling_range);
      break;
    }
    case SuccinctMode::CONSTRUCT_MEMORY_MAPPED: {
      fprintf(stderr, "Unsupported mode.\n");
      assert(0);
      break;
    }
    case SuccinctMode::LOAD_IN_MEMORY: {
      switch (npa_encoding_scheme) {
        case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
          npa_ = new EliasGammaEncodedNPA(context_len, npa_sampling_rate,
                                          s_allocator);
          break;
        case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
          npa_ = new EliasDeltaEncodedNPA(context_len, npa_sampling_rate,
                                          s_allocator);
          return;
        case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
          npa_ = new WaveletTreeEncodedNPA(context_len, npa_sampling_rate,
                                           s_allocator);
          break;
        default:
          npa_ = NULL;
      }

      assert(npa_ != NULL);

      switch (sa_sampling_scheme) {
        case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
          sa_ = new SampledByIndexSA(sa_sampling_rate, npa_, s_allocator);
          break;
        case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
          sa_ = new SampledByValueSA(sa_sampling_rate, npa_, s_allocator);
          break;
        case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
          sa_ = new LayeredSampledSA(sa_sampling_rate,
                                     sa_sampling_rate * sampling_range, npa_,
                                     s_allocator);
          break;
        case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
          sa_ = new OpportunisticLayeredSampledSA(
              sa_sampling_rate, sa_sampling_rate * sampling_range, npa_,
              s_allocator);
          break;
        default:
          sa_ = NULL;
      }

      assert(sa_ != NULL);

      switch (isa_sampling_scheme) {
        case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
          isa_ = new SampledByIndexISA(isa_sampling_rate, npa_, s_allocator);
          break;
        case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
          assert(
              sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
          isa_ = new SampledByValueISA(sa_sampling_rate, npa_, s_allocator);
          break;
        case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
          isa_ = new LayeredSampledISA(isa_sampling_rate,
                                       isa_sampling_rate * sampling_range, npa_,
                                       s_allocator);
          break;
        case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
          isa_ = new OpportunisticLayeredSampledISA(
              isa_sampling_rate, isa_sampling_rate * sampling_range, npa_,
              s_allocator);
          break;
        default:
          isa_ = NULL;
      }

      assert(isa_ != NULL);

      Deserialize();
      break;
    }
    case SuccinctMode::LOAD_MEMORY_MAPPED: {
      switch (npa_encoding_scheme) {
        case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
          npa_ = new EliasGammaEncodedNPA(context_len, npa_sampling_rate,
                                          s_allocator);
          break;
        case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
          npa_ = new EliasDeltaEncodedNPA(context_len, npa_sampling_rate,
                                          s_allocator);
          return;
        case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
          npa_ = new WaveletTreeEncodedNPA(context_len, npa_sampling_rate,
                                           s_allocator);
          break;
        default:
          npa_ = NULL;
      }

      assert(npa_ != NULL);

      switch (sa_sampling_scheme) {
        case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
          sa_ = new SampledByIndexSA(sa_sampling_rate, npa_, s_allocator);
          break;
        case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
          sa_ = new SampledByValueSA(sa_sampling_rate, npa_, s_allocator);
          break;
        case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
          sa_ = new LayeredSampledSA(sa_sampling_rate,
                                     sa_sampling_rate * sampling_range, npa_,
                                     s_allocator);
          break;
        case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
          sa_ = new OpportunisticLayeredSampledSA(
              sa_sampling_rate, sa_sampling_rate * sampling_range, npa_,
              s_allocator);
          break;
        default:
          sa_ = NULL;
      }

      assert(sa_ != NULL);

      switch (isa_sampling_scheme) {
        case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
          isa_ = new SampledByIndexISA(isa_sampling_rate, npa_, s_allocator);
          break;
        case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
          assert(
              sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
          isa_ = new SampledByValueISA(sa_sampling_rate, npa_, s_allocator);
          break;
        case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
          isa_ = new LayeredSampledISA(isa_sampling_rate,
                                       isa_sampling_rate * sampling_range, npa_,
                                       s_allocator);
          break;
        case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
          isa_ = new OpportunisticLayeredSampledISA(
              isa_sampling_rate, isa_sampling_rate * sampling_range, npa_,
              s_allocator);
          break;
        default:
          isa_ = NULL;
      }

      assert(isa_ != NULL);

      MemoryMap();
      break;
    }
  }

}

/* Primary Construct function */
void SuccinctCore::Construct(const char* filename, uint32_t sa_sampling_rate,
                             uint32_t isa_sampling_rate,
                             uint32_t npa_sampling_rate, uint32_t context_len,
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

  data[fsize] = (char) 1;

  input_size_ = fsize + 1;
  uint32_t bits = SuccinctUtils::IntegerLog2(input_size_ + 1);

  // Construct Suffix Array
  int64_t *lSA = (int64_t *) s_allocator.s_calloc(sizeof(int64_t), input_size_);
  divsufsortxx::constructSA((uint8_t *) data, (uint8_t *) (data + input_size_),
                            lSA, lSA + input_size_, 256);

  // Compact SA
  BitMap *compactSA = new BitMap;
  CreateBitmapArray(&compactSA, (uint64_t *) lSA, input_size_, bits,
                    s_allocator);
  s_allocator.s_free(lSA);

  BitMap *compactISA = new BitMap;
  InitBitmap(&compactISA, input_size_ * bits, s_allocator);

  uint64_t cur_sa, prv_sa;
  alphabet_size_ = 1;
  cur_sa = prv_sa = LookupBitmapArray(compactSA, 0, bits);
  SetBitmapArray(&compactISA, cur_sa, 0, bits);
  alphabet_map_[data[cur_sa]] = std::pair<uint64_t, uint32_t>(0, 0);
  for (uint64_t i = 1; i < input_size_; i++) {
    cur_sa = LookupBitmapArray(compactSA, i, bits);
    SetBitmapArray(&compactISA, cur_sa, i, bits);
    if (cur_sa != prv_sa) {
      Cinv_idx_.push_back(i);
      alphabet_map_[data[cur_sa]] = std::pair<uint64_t, uint32_t>(
          i, alphabet_size_);
      alphabet_size_++;
    }
    prv_sa = cur_sa;
  }

  alphabet_map_[(char) 0] = std::pair<uint64_t, uint32_t>(input_size_, alphabet_size_);
  alphabet_ = new char[alphabet_size_ + 1];
  for (auto alphabet_entry : alphabet_map_) {
    alphabet_[alphabet_entry.second.second] = alphabet_entry.first;
  }

  // Compact input data
  BitMap *data_bitmap = new BitMap;
  int sigma_bits = SuccinctUtils::IntegerLog2(alphabet_size_ + 1);
  InitBitmap(&data_bitmap, input_size_ * sigma_bits, s_allocator);
  for (uint64_t i = 0; i < input_size_; i++) {
    SetBitmapArray(&data_bitmap, i, alphabet_map_[data[i]].second, sigma_bits);
  }
  s_allocator.s_free(data);

  switch (npa_encoding_scheme) {
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
      npa_ = new EliasGammaEncodedNPA(input_size_, alphabet_size_, context_len,
                                      npa_sampling_rate, data_bitmap, compactSA,
                                      compactISA, s_allocator);
      break;
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
      npa_ = new EliasDeltaEncodedNPA(input_size_, alphabet_size_, context_len,
                                      npa_sampling_rate, data_bitmap, compactSA,
                                      compactISA, s_allocator);
      return;
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
      npa_ = new WaveletTreeEncodedNPA(input_size_, alphabet_size_, context_len,
                                       npa_sampling_rate, data_bitmap,
                                       compactSA, compactISA, s_allocator);
      break;
    default:
      npa_ = NULL;
  }

  assert(npa_ != NULL);

  DestroyBitmap(&compactISA, s_allocator);
  DestroyBitmap(&data_bitmap, s_allocator);

  switch (sa_sampling_scheme) {
    case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
      sa_ = new SampledByIndexSA(sa_sampling_rate, npa_, compactSA, input_size_,
                                 s_allocator);
      break;
    case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
      sa_ = new SampledByValueSA(sa_sampling_rate, npa_, compactSA, input_size_,
                                 s_allocator);
      break;
    case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
      sa_ = new LayeredSampledSA(sa_sampling_rate,
                                 sa_sampling_rate * sampling_range, npa_,
                                 compactSA, input_size_, s_allocator);
      break;
    case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
      sa_ = new OpportunisticLayeredSampledSA(sa_sampling_rate,
                                              sa_sampling_rate * sampling_range,
                                              npa_, compactSA, input_size_,
                                              s_allocator);
      break;
    default:
      sa_ = NULL;
  }

  assert(sa_ != NULL);

  switch (isa_sampling_scheme) {
    case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
      isa_ = new SampledByIndexISA(isa_sampling_rate, npa_, compactSA,
                                   input_size_, s_allocator);
      break;
    case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
      assert(sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
      isa_ = new SampledByValueISA(
          sa_sampling_rate, npa_, compactSA, input_size_,
          ((SampledByValueSA *) sa_)->GetSampledPositions(), s_allocator);
      break;
    case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
      isa_ = new LayeredSampledISA(isa_sampling_rate,
                                   isa_sampling_rate * sampling_range, npa_,
                                   compactSA, input_size_, s_allocator);
      break;
    case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
      isa_ = new OpportunisticLayeredSampledISA(
          isa_sampling_rate, isa_sampling_rate * sampling_range, npa_,
          compactSA, input_size_, s_allocator);
      break;
    default:
      isa_ = NULL;
  }

  assert(isa_ != NULL);

  DestroyBitmap(&compactSA, s_allocator);
}

void SuccinctCore::ConstructAuxiliary(BitMap *compactSA, const char *data) {
  uint32_t bits = SuccinctUtils::IntegerLog2(input_size_ + 1);
  alphabet_size_ = 1;

  for (uint64_t i = 1; i < input_size_; ++i) {
    if (data[LookupBitmapArray(compactSA, i, bits)]
        != data[LookupBitmapArray(compactSA, i - 1, bits)]) {
      Cinv_idx_.push_back(i);
      alphabet_size_++;
    }
  }

  alphabet_ = new char[alphabet_size_ + 1];

  alphabet_[0] = data[LookupBitmapArray(compactSA, 0, bits)];
  alphabet_map_[alphabet_[0]] = std::pair<uint64_t, uint32_t>(0, 0);
  uint64_t i;

  for (i = 1; i < alphabet_size_; i++) {
    uint64_t sel = Cinv_idx_[i - 1];
    alphabet_[i] = data[LookupBitmapArray(compactSA, sel, bits)];
    alphabet_map_[alphabet_[i]] = std::pair<uint64_t, uint32_t>(sel, i);
  }
  alphabet_map_[(char) 0] = std::pair<uint64_t, uint32_t>(input_size_, i);
  alphabet_[i] = (char) 0;
}

/* Lookup functions for each of the core data structures */
// Lookup NPA at index i
uint64_t SuccinctCore::LookupNPA(uint64_t i) {
  return (*npa_)[i];
}

// Lookup SA at index i
uint64_t SuccinctCore::LookupSA(uint64_t i) {
  return (*sa_)[i];
}

// Lookup ISA at index i
uint64_t SuccinctCore::LookupISA(uint64_t i) {
  return (*isa_)[i];
}

// Lookup C at index i
uint64_t SuccinctCore::LookupC(uint64_t i) {
  return GetRank1(&Cinv_idx_, i);
}

char SuccinctCore::CharAt(uint64_t i) {
  return alphabet_[LookupC(LookupISA(i))];
}

size_t SuccinctCore::Serialize() {
  size_t out_size = 0;
  typedef std::map<char, std::pair<uint64_t, uint32_t> >::iterator iterator_t;
  struct stat st;
  if (stat(succinct_path_.c_str(), &st) != 0) {
    if (mkdir(succinct_path_.c_str(),
              (mode_t) (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
        != 0) {
      fprintf(
          stderr,
          "succinct dir '%s' does not exist, and failed to mkdir (no space or permission?)\n",
          this->succinct_path_.c_str());
      fprintf(stderr, "terminating the serialization process.\n");
      return 1;
    }
  }
  std::ofstream out(succinct_path_ + "/metadata");
  std::ofstream sa_out(succinct_path_ + "/sa");
  std::ofstream isa_out(succinct_path_ + "/isa");
  std::ofstream npa_out(succinct_path_ + "/npa");

  // Output size of input file
  out.write(reinterpret_cast<const char *>(&(input_size_)), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  // Output cmap size
  uint64_t cmap_size = alphabet_map_.size();
  out.write(reinterpret_cast<const char *>(&(cmap_size)), sizeof(uint64_t));
  out_size += sizeof(uint64_t);
  for (iterator_t it = alphabet_map_.begin(); it != alphabet_map_.end(); ++it) {
    out.write(reinterpret_cast<const char *>(&(it->first)), sizeof(char));
    out_size += sizeof(char);
    out.write(reinterpret_cast<const char *>(&(it->second.first)),
              sizeof(uint64_t));
    out_size += sizeof(uint64_t);
    out.write(reinterpret_cast<const char *>(&(it->second.second)),
              sizeof(uint32_t));
    out_size += sizeof(uint32_t);
  }

  out.write(reinterpret_cast<const char *>(&alphabet_size_), sizeof(uint32_t));
  out_size += sizeof(uint32_t);
  for (uint32_t i = 0; i < alphabet_size_ + 1; i++) {
    out.write(reinterpret_cast<const char *>(&alphabet_[i]), sizeof(char));
  }

  out_size += sa_->Serialize(sa_out);
  out_size += isa_->Serialize(isa_out);

  if (sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
    assert(isa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
    out_size += SerializeDictionary(
        ((SampledByValueSA *) sa_)->GetSampledPositions(), out);
  }

  out_size += npa_->Serialize(npa_out);

  out.close();
  sa_out.close();
  isa_out.close();
  npa_out.close();

  return out_size;
}

size_t SuccinctCore::Deserialize() {
  // Check if directory exists
  struct stat st;
  assert(stat(succinct_path_.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

  std::ifstream in(succinct_path_ + "/metadata");
  std::ifstream sa_in(succinct_path_ + "/sa");
  std::ifstream isa_in(succinct_path_ + "/isa");
  std::ifstream npa_in(succinct_path_ + "/npa");

  size_t in_size = 0;

  // Read size of input file
  in.read(reinterpret_cast<char *>(&(input_size_)), sizeof(uint64_t));
  in_size += sizeof(uint64_t);

  // Read alphabet map size
  uint64_t alphabet_map_size;
  in.read(reinterpret_cast<char *>(&(alphabet_map_size)), sizeof(uint64_t));
  in_size += sizeof(uint64_t);

  // Deserialize alphabet map
  for (uint64_t i = 0; i < alphabet_map_size; i++) {
    char first;
    uint64_t second_first;
    uint32_t second_second;
    in.read(reinterpret_cast<char *>(&(first)), sizeof(char));
    in_size += sizeof(char);
    in.read(reinterpret_cast<char *>(&(second_first)), sizeof(uint64_t));
    in_size += sizeof(uint64_t);
    in.read(reinterpret_cast<char *>(&(second_second)), sizeof(uint32_t));
    in_size += sizeof(uint32_t);
    alphabet_map_[first] = std::pair<uint64_t, uint32_t>(second_first,
                                                         second_second);
  }

  // Read alphabet size
  in.read(reinterpret_cast<char *>(&alphabet_size_), sizeof(uint32_t));
  in_size += sizeof(uint32_t);

  // Deserialize alphabet
  alphabet_ = new char[alphabet_size_ + 1];
  for (uint32_t i = 0; i < alphabet_size_ + 1; i++) {
    in.read(reinterpret_cast<char *>(&alphabet_[i]), sizeof(char));
  }

  // Deserialize SA, ISA
  in_size += sa_->Deserialize(sa_in);
  in_size += isa_->Deserialize(isa_in);

  // Deserialize bitmap marking positions of sampled values if the sampling scheme
  // is sample by value.
  if (sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
    assert(isa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
    Dictionary *sampled_positions = new Dictionary;
    in_size += DeserializeDictionary(sampled_positions, in);
    ((SampledByValueSA *) sa_)->SetSampledPositions(sampled_positions);
    ((SampledByValueISA *) isa_)->SetSampledPositions(sampled_positions);
  }

  // Deserialize NPA based on the NPA encoding scheme.
  switch (npa_->GetEncodingScheme()) {
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
      in_size += ((EliasDeltaEncodedNPA *) npa_)->Deserialize(npa_in);
      break;
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
      in_size += ((EliasGammaEncodedNPA *) npa_)->Deserialize(npa_in);
      break;
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
      in_size += ((WaveletTreeEncodedNPA *) npa_)->Deserialize(npa_in);
      break;
    default:
      assert(0);
  }

  // TODO: Fix
  Cinv_idx_.insert(Cinv_idx_.end(), npa_->col_offsets_.begin() + 1,
                   npa_->col_offsets_.end());

  in.close();
  sa_in.close();
  isa_in.close();
  npa_in.close();

  return in_size;
}

size_t SuccinctCore::MemoryMap() {
  // Check if directory exists
  struct stat st;
  assert(stat(succinct_path_.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

  uint8_t *data_beg, *data;
  data = data_beg = (uint8_t *) SuccinctUtils::MemoryMap(
      succinct_path_ + "/metadata");

  input_size_ = *((uint64_t *) data);
  data += sizeof(uint64_t);

  // Read alphabet_map size
  uint64_t alphabet_map_size = *((uint64_t *) data);
  data += sizeof(uint64_t);

  // Deserialize map, because we don't know how to have serialized,
  // memory mapped maps yet. This is fine, since the map size is
  // pretty small.
  for (uint64_t i = 0; i < alphabet_map_size; i++) {
    char first = *((char *) data);
    data += sizeof(char);
    uint64_t second_first = *((uint64_t *) data);
    data += sizeof(uint64_t);
    uint32_t second_second = *((uint32_t *) data);
    data += sizeof(uint32_t);
    alphabet_map_[first] = std::pair<uint64_t, uint32_t>(second_first,
                                                         second_second);
  }

  // Read alphabet size
  alphabet_size_ = *((uint32_t *) data);
  data += sizeof(uint32_t);

  // Read alphabet
  alphabet_ = (char *) data;
  data += (sizeof(char) * (alphabet_size_ + 1));

  // Memory map SA and ISA
  data += sa_->MemoryMap(succinct_path_ + "/sa");
  data += isa_->MemoryMap(succinct_path_ + "/isa");

  // Memory map bitmap marking positions of sampled values if the sampling scheme
  // is sample by value.
  if (sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
    Dictionary *d_bpos;
    assert(isa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
    data += MemoryMapDictionary(&d_bpos, data);
    ((SampledByValueSA *) sa_)->SetSampledPositions(d_bpos);
    ((SampledByValueISA *) isa_)->SetSampledPositions(d_bpos);
  }

  // Memory map NPA based on the NPA encoding scheme.
  switch (npa_->GetEncodingScheme()) {
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:
      data += ((EliasDeltaEncodedNPA *) npa_)->MemoryMap(
          succinct_path_ + "/npa");
      break;
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:
      data += ((EliasGammaEncodedNPA *) npa_)->MemoryMap(
          succinct_path_ + "/npa");
      break;
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
      data += ((WaveletTreeEncodedNPA *) npa_)->MemoryMap(
          succinct_path_ + "/npa");
      break;
    default:
      assert(0);
  }

  // TODO: Fix
  Cinv_idx_.insert(Cinv_idx_.end(), npa_->col_offsets_.begin() + 1,
                   npa_->col_offsets_.end());

  return data - data_beg;
}

uint64_t SuccinctCore::GetOriginalSize() {
  return input_size_;
}

size_t SuccinctCore::StorageSize() {
  size_t tot_size = SuccinctBase::StorageSize();
  tot_size += sizeof(uint64_t);
  tot_size += sizeof(alphabet_map_.size())
      + alphabet_map_.size()
          * (sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t));
  tot_size += sizeof(alphabet_size_) + alphabet_size_ * sizeof(char);
  tot_size += sa_->StorageSize();
  tot_size += isa_->StorageSize();
  tot_size += npa_->StorageSize();
  return tot_size;
}

void SuccinctCore::PrintStorageBreakdown() {
  size_t metadata_size = SuccinctBase::StorageSize();
  metadata_size += sizeof(uint64_t);
  metadata_size += sizeof(alphabet_map_.size())
      + alphabet_map_.size()
          * (sizeof(char) + sizeof(uint64_t) + sizeof(uint32_t));
  metadata_size += sizeof(alphabet_size_) + alphabet_size_ * sizeof(char);
  fprintf(stderr, "Metadata size = %zu\n", metadata_size);
  fprintf(stderr, "SA size = %zu\n", sa_->StorageSize());
  fprintf(stderr, "ISA size = %zu\n", isa_->StorageSize());
  fprintf(stderr, "NPA size = %zu\n", npa_->StorageSize());
}

std::pair<int64_t, int64_t> SuccinctCore::BwdSearch(std::string mgram) {
  std::pair<int64_t, int64_t> range(0, -1), col_range;
  uint64_t m_len = mgram.length();

  if (alphabet_map_.find(mgram[m_len - 1]) != alphabet_map_.end()) {
    range.first = (alphabet_map_[mgram[m_len - 1]]).first;
    range.second = alphabet_map_[alphabet_[alphabet_map_[mgram[m_len - 1]]
        .second + 1]].first - 1;
  } else
    return std::pair<int64_t, int64_t>(0, -1);

  for (int64_t i = m_len - 2; i >= 0; i--) {
    if (alphabet_map_.find(mgram[i]) != alphabet_map_.end()) {
      col_range.first = alphabet_map_[mgram[i]].first;
      col_range.second = alphabet_map_[alphabet_[alphabet_map_[mgram[i]].second
          + 1]].first - 1;
    } else
      return std::pair<int64_t, int64_t>(0, -1);

    if (col_range.first > col_range.second)
      return std::pair<int64_t, int64_t>(0, -1);

    range.first = npa_->BinarySearch(range.first, col_range.first,
                                     col_range.second, false);
    range.second = npa_->BinarySearch(range.second, col_range.first,
                                      col_range.second, true);

    if (range.first > range.second)
      return range;
  }

  return range;
}

std::pair<int64_t, int64_t> SuccinctCore::ContinueBwdSearch(
    std::string mgram, std::pair<int64_t, int64_t> range) {
  std::pair<int64_t, int64_t> col_range;
  uint64_t m_len = mgram.length();

  for (int64_t i = m_len - 1; i >= 0; i--) {
    if (alphabet_map_.find(mgram[i]) != alphabet_map_.end()) {
      col_range.first = alphabet_map_[mgram[i]].first;
      col_range.second = alphabet_map_[alphabet_[alphabet_map_[mgram[i]].second
          + 1]].first - 1;
    } else
      return std::pair<int64_t, int64_t>(0, -1);

    if (col_range.first > col_range.second)
      return std::pair<int64_t, int64_t>(0, -1);

    range.first = npa_->BinarySearch(range.first, col_range.first,
                                     col_range.second, false);
    range.second = npa_->BinarySearch(range.second, col_range.first,
                                      col_range.second, true);

    if (range.first > range.second)
      return range;
  }

  return range;
}

int SuccinctCore::Compare(std::string p, int64_t i) {
  long j = 0;
  do {
    char c = alphabet_[LookupC(i)];
    if (p[j] < c) {
      return -1;
    } else if (p[j] > c) {
      return 1;
    }
    i = LookupNPA(i);
    j++;
  } while (j < p.length());
  return 0;
}

int SuccinctCore::Compare(std::string p, int64_t i, size_t offset) {

  uint64_t j = 0;

  // Skip first offset chars
  while (offset) {
    i = LookupNPA(i);
    offset--;
  }

  do {
    char c = alphabet_[LookupC(i)];
    if (p[j] < c) {
      return -1;
    } else if (p[j] > c) {
      return 1;
    }
    i = LookupNPA(i);
    j++;
  } while (j < p.length());

  return 0;
}

std::pair<int64_t, int64_t> SuccinctCore::FwdSearch(std::string mgram) {

  int64_t st = GetOriginalSize() - 1;
  int64_t sp = 0;
  int64_t s;
  while (sp < st) {
    s = (sp + st) / 2;
    if (Compare(mgram, s) > 0)
      sp = s + 1;
    else
      st = s;
  }

  int64_t et = GetOriginalSize() - 1;
  int64_t ep = sp - 1;
  int64_t e;

  while (ep < et) {
    e = ceil((double) (ep + et) / 2);
    if (Compare(mgram, e) == 0)
      ep = e;
    else
      et = e - 1;
  }

  return std::pair<int64_t, int64_t>(sp, ep);
}

std::pair<int64_t, int64_t> SuccinctCore::ContinueFwdSearch(
    std::string mgram, std::pair<int64_t, int64_t> range, size_t len) {

  if (mgram == "")
    return range;

  int64_t st = range.second;
  int64_t sp = range.first;
  int64_t s;
  while (sp < st) {
    s = (sp + st) / 2;
    if (Compare(mgram, s, len) > 0)
      sp = s + 1;
    else
      st = s;
  }

  int64_t et = range.second;
  int64_t ep = sp - 1;
  int64_t e;

  while (ep < et) {
    e = ceil((double) (ep + et) / 2);
    if (Compare(mgram, e, len) == 0)
      ep = e;
    else
      et = e - 1;
  }

  return std::pair<int64_t, int64_t>(sp, ep);
}

SampledArray *SuccinctCore::GetSA() {
  return sa_;
}

SampledArray *SuccinctCore::GetISA() {
  return isa_;
}

NPA *SuccinctCore::GetNPA() {
  return npa_;
}

char *SuccinctCore::GetAlphabet() {
  return alphabet_;
}
