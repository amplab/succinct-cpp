#include "succinct_core.h"

SuccinctCore::SuccinctCore(const std::string &filename, SuccinctMode s_mode,
                           uint32_t sa_sampling_rate,
                           uint32_t isa_sampling_rate,
                           uint32_t npa_sampling_rate, uint32_t context_len,
                           SamplingScheme sa_sampling_scheme,
                           SamplingScheme isa_sampling_scheme,
                           NPA::NPAEncodingScheme npa_encoding_scheme,
                           uint32_t sampling_range)
    : SuccinctBase() {

  this->alphabet_ = nullptr;
  this->sa_ = nullptr;
  this->isa_ = nullptr;
  this->npa_ = nullptr;
  this->alphabet_size_ = 0;
  this->input_size_ = 0;
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
      Allocate(sa_sampling_rate, isa_sampling_rate, npa_sampling_rate,
               context_len, sa_sampling_scheme, isa_sampling_scheme,
               npa_encoding_scheme, sampling_range);
      Deserialize(filename);
      break;
    }
    case SuccinctMode::LOAD_MEMORY_MAPPED: {
      Allocate(sa_sampling_rate, isa_sampling_rate, npa_sampling_rate,
               context_len, sa_sampling_scheme, isa_sampling_scheme,
               npa_encoding_scheme, sampling_range);
      MemoryMap(filename);
      break;
    }
  }

}

void SuccinctCore::Allocate(uint32_t sa_sampling_rate,
                            uint32_t isa_sampling_rate,
                            uint32_t npa_sampling_rate, uint32_t context_len,
                            SamplingScheme sa_sampling_scheme,
                            SamplingScheme isa_sampling_scheme,
                            NPA::NPAEncodingScheme npa_encoding_scheme,
                            uint32_t sampling_range) {
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
    default:npa_ = nullptr;
  }

  assert(npa_ != nullptr);

  switch (sa_sampling_scheme) {
    case SamplingScheme::FLAT_SAMPLE_BY_INDEX:sa_ = new SampledByIndexSA(sa_sampling_rate, npa_, s_allocator);
      break;
    case SamplingScheme::FLAT_SAMPLE_BY_VALUE:sa_ = new SampledByValueSA(sa_sampling_rate, npa_, s_allocator);
      break;
    case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
      sa_ = new LayeredSampledSA(sa_sampling_rate,
                                 sa_sampling_rate * sampling_range, npa_,
                                 s_allocator);
      break;
    case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
      sa_ = new OpportunisticLayeredSampledSA(sa_sampling_rate,
                                              sa_sampling_rate * sampling_range,
                                              npa_, s_allocator);
      break;
    default:sa_ = nullptr;
  }

  assert(sa_ != nullptr);

  switch (isa_sampling_scheme) {
    case SamplingScheme::FLAT_SAMPLE_BY_INDEX:isa_ = new SampledByIndexISA(isa_sampling_rate, npa_, s_allocator);
      break;
    case SamplingScheme::FLAT_SAMPLE_BY_VALUE:assert(sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
      isa_ = new SampledByValueISA(sa_sampling_rate, npa_, s_allocator);
      break;
    case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
      isa_ = new LayeredSampledISA(isa_sampling_rate,
                                   isa_sampling_rate * sampling_range,
                                   npa_,
                                   s_allocator);
      break;
    case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
      isa_ = new OpportunisticLayeredSampledISA(
          isa_sampling_rate, isa_sampling_rate * sampling_range, npa_,
          s_allocator);
      break;
    default:isa_ = nullptr;
  }

  assert(isa_ != nullptr);
}

void SuccinctCore::Construct(const std::string &filename,
                             uint32_t sa_sampling_rate,
                             uint32_t isa_sampling_rate,
                             uint32_t npa_sampling_rate, uint32_t context_len,
                             SamplingScheme sa_sampling_scheme,
                             SamplingScheme isa_sampling_scheme,
                             NPA::NPAEncodingScheme npa_encoding_scheme,
                             uint32_t sampling_range) {
  // Get input size
  FILE *f = fopen(filename.c_str(), "r");
  fseek(f, 0, SEEK_END);
  uint64_t fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  // Read input from file
  auto *data = (uint8_t *) s_allocator.s_malloc(fsize + 1);
  fread(data, fsize, 1, f);
  fclose(f);
  data[fsize] = 1;

  Construct(data, fsize + 1, sa_sampling_rate, isa_sampling_rate,
            npa_sampling_rate, context_len, sa_sampling_scheme,
            isa_sampling_scheme, npa_encoding_scheme, sampling_range);
}

/* Primary Construct function */
void SuccinctCore::Construct(uint8_t *input, size_t input_size,
                             uint32_t sa_sampling_rate,
                             uint32_t isa_sampling_rate,
                             uint32_t npa_sampling_rate, uint32_t context_len,
                             SamplingScheme sa_sampling_scheme,
                             SamplingScheme isa_sampling_scheme,
                             NPA::NPAEncodingScheme npa_encoding_scheme,
                             uint32_t sampling_range) {

  std::string sa_file = ".tmp.sa";
  std::string isa_file = ".tmp.isa";
  std::string npa_file = ".tmp.npa";

  // Save metadata
  input_size_ = input_size;
  uint32_t bits = SuccinctUtils::IntegerLog2(input_size_ + 1);

  // Construct Suffix Array
  auto *lSA = (int64_t *) s_allocator.s_calloc(sizeof(int64_t), input_size_);
  divsufsortxx::constructSA(input, (input + input_size_), lSA,
                            lSA + input_size_, 256);

  // Write Suffix Array to file
  SuccinctUtils::WriteToFile(lSA, input_size_, sa_file);

  ArrayStream sa_stream(sa_file);
  s_allocator.s_free(lSA);

  // Allocate space for Inverse Suffix Array
  auto *lISA = (int64_t *) s_allocator.s_calloc(sizeof(int64_t),
                                                input_size_);

  // Auxiliary Data Structures for NPA
  std::vector<uint64_t> col_offsets;
  uint64_t cur_sa, prv_sa;

  prv_sa = cur_sa = sa_stream.Get();
  lISA[cur_sa] = 0;
  alphabet_size_ = 1;
  alphabet_map_[input[cur_sa]] = std::pair<uint64_t, uint32_t>(0, 0);
  col_offsets.push_back(0);
  for (uint64_t i = 1; i < input_size_; i++) {
    cur_sa = sa_stream.Get();
    lISA[cur_sa] = i;
    if (input[cur_sa] != input[prv_sa]) {
      alphabet_map_[input[cur_sa]] = std::pair<uint64_t, uint32_t>(
          i, alphabet_size_++);
      col_offsets.push_back(i);
    }
    prv_sa = cur_sa;
  }

  alphabet_map_[(char) 0] = std::pair<uint64_t, uint32_t>(input_size_,
                                                          alphabet_size_);
  assert(sa_stream.GetCurrentIndex() == input_size_);
  sa_stream.Reset();

  alphabet_ = new char[alphabet_size_ + 1];
  for (auto alphabet_entry : alphabet_map_) {
    alphabet_[alphabet_entry.second.second] = alphabet_entry.first;
  }

  // Write Inverse Suffix Array to file
  SuccinctUtils::WriteToFile(lISA, input_size_, isa_file);
  s_allocator.s_free(lISA);
  ArrayStream isa_stream(isa_file);

  // Compact input data (if needed)
  Bitmap *data_bitmap = nullptr;
  if (npa_encoding_scheme == NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED) {
    data_bitmap = new Bitmap;
    int sigma_bits = SuccinctUtils::IntegerLog2(alphabet_size_ + 1);
    InitBitmap(&data_bitmap, input_size_ * sigma_bits, s_allocator);
    for (uint64_t i = 0; i < input_size_; i++) {
      SetBitmapArray(&data_bitmap, i, alphabet_map_[input[i]].second,
                     sigma_bits);
    }
  }
  s_allocator.s_free(input);

  switch (npa_encoding_scheme) {
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED: {
      npa_ = new EliasGammaEncodedNPA(input_size_, alphabet_size_, context_len,
                                      npa_sampling_rate, isa_file, col_offsets,
                                      npa_file, s_allocator);
      break;
    }
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED: {
      npa_ = new EliasDeltaEncodedNPA(input_size_, alphabet_size_, context_len,
                                      npa_sampling_rate, isa_file, col_offsets,
                                      npa_file, s_allocator);
      return;
    }
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED: {
      isa_stream.CloseAndRemove();
      Bitmap *compactSA = ReadAsBitmap(input_size_, bits, s_allocator, sa_file);
      Bitmap *compactISA = ReadAsBitmap(input_size_, bits, s_allocator,
                                        isa_file);
      npa_ = new WaveletTreeEncodedNPA(input_size_, alphabet_size_, context_len,
                                       npa_sampling_rate, data_bitmap,
                                       compactSA, compactISA, s_allocator);
      DestroyBitmap(&data_bitmap, s_allocator);
      break;
    }
    default:npa_ = nullptr;
  }
  assert(npa_ != nullptr);

  switch (sa_sampling_scheme) {
    case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
      sa_ = new SampledByIndexSA(sa_sampling_rate, npa_, sa_stream, input_size_,
                                 s_allocator);
      break;
    case SamplingScheme::FLAT_SAMPLE_BY_VALUE:
      sa_ = new SampledByValueSA(sa_sampling_rate, npa_, sa_stream, input_size_,
                                 s_allocator);
      break;
    case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
      sa_ = new LayeredSampledSA(sa_sampling_rate,
                                 sa_sampling_rate * sampling_range, npa_,
                                 sa_stream, input_size_, s_allocator);
      break;
    case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
      sa_ = new OpportunisticLayeredSampledSA(sa_sampling_rate,
                                              sa_sampling_rate * sampling_range,
                                              npa_, sa_stream, input_size_,
                                              s_allocator);
      break;
    default:sa_ = nullptr;
  }
  sa_stream.Reset();
  assert(sa_ != nullptr);

  switch (isa_sampling_scheme) {
    case SamplingScheme::FLAT_SAMPLE_BY_INDEX:
      isa_ = new SampledByIndexISA(isa_sampling_rate, npa_, sa_stream,
                                   input_size_, s_allocator);
      break;
    case SamplingScheme::FLAT_SAMPLE_BY_VALUE:assert(sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
      isa_ = new SampledByValueISA(
          sa_sampling_rate, npa_, sa_stream, input_size_,
          ((SampledByValueSA *) sa_)->GetSampledPositions(), s_allocator);
      break;
    case SamplingScheme::LAYERED_SAMPLE_BY_INDEX:
      isa_ = new LayeredSampledISA(isa_sampling_rate,
                                   isa_sampling_rate * sampling_range, npa_,
                                   sa_stream, input_size_, s_allocator);
      break;
    case SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX:
      isa_ = new OpportunisticLayeredSampledISA(
          isa_sampling_rate, isa_sampling_rate * sampling_range, npa_,
          sa_stream, input_size_, s_allocator);
      break;
    default:isa_ = nullptr;
  }
  sa_stream.Reset();
  assert(isa_ != nullptr);

  sa_stream.CloseAndRemove();
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
  return GetRank1(&npa_->col_offsets_, i) - 1;
}

char SuccinctCore::CharAt(uint64_t i) {
  return alphabet_[LookupC(LookupISA(i))];
}

size_t SuccinctCore::Serialize(const std::string &path) {
  size_t out_size = 0;
  typedef std::map<char, std::pair<uint64_t, uint32_t> >::iterator iterator_t;
  struct stat st{};
  mode_t create_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  if (stat(path.c_str(), &st) != 0) {
    if (mkdir(path.c_str(), create_mode) != 0) {
      fprintf(stderr, "Failed to create path '%s'\n", path.c_str());
      fprintf(stderr, "Terminating the serialization process.\n");
      return 0;
    }
  }
  std::ofstream out(path + "/" + path.substr(0, path.size()-9) + ".succinct.metadata");
  // std::ofstream sa_out(path + "/sa");
  // std::ofstream isa_out(path + "/isa");
  // std::ofstream npa_out(path + "/npa");

  // Output size of input file
  out.write(reinterpret_cast<const char *>(&(input_size_)), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  // Output cmap size
  uint64_t cmap_size = alphabet_map_.size();
  out.write(reinterpret_cast<const char *>(&(cmap_size)), sizeof(uint64_t));
  out_size += sizeof(uint64_t);
  for (auto &it : alphabet_map_) {
    out.write(reinterpret_cast<const char *>(&(it.first)), sizeof(char));
    out_size += sizeof(char);
    out.write(reinterpret_cast<const char *>(&(it.second.first)), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
    out.write(reinterpret_cast<const char *>(&(it.second.second)), sizeof(uint32_t));
    out_size += sizeof(uint32_t);
  }

  out.write(reinterpret_cast<const char *>(&alphabet_size_), sizeof(uint32_t));
  out_size += sizeof(uint32_t);
  for (uint32_t i = 0; i < alphabet_size_ + 1; i++) {
    out.write(reinterpret_cast<const char *>(&alphabet_[i]), sizeof(char));
  }

  out_size += sa_->Serialize(out);
  out_size += isa_->Serialize(out);

  if (sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
    assert(isa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
    out_size += SerializeDictionary(((SampledByValueSA *) sa_)->GetSampledPositions(), out);
  }

  out_size += npa_->Serialize(out);

  out.close();

  return out_size;
}

size_t SuccinctCore::Deserialize(const std::string &path) {
  // Check if directory exists
  struct stat st{};
  assert(stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

  std::ifstream in(path + "/" + path.substr(0, path.size()-9) + ".succinct.metadata");
  // std::ifstream sa_in(path + "/sa");
  // std::ifstream isa_in(path + "/isa");
  // std::ifstream npa_in(path + "/npa");

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
  in_size += sa_->Deserialize(in);
  in_size += isa_->Deserialize(in);

  // Deserialize bitmap marking positions of sampled values if the sampling scheme
  // is sample by value.
  if (sa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE) {
    assert(isa_->GetSamplingScheme() == SamplingScheme::FLAT_SAMPLE_BY_VALUE);
    auto *sampled_positions = new Dictionary;
    in_size += DeserializeDictionary(sampled_positions, in);
    ((SampledByValueSA *) sa_)->SetSampledPositions(sampled_positions);
    ((SampledByValueISA *) isa_)->SetSampledPositions(sampled_positions);
  }

  // Deserialize NPA based on the NPA encoding scheme.
  switch (npa_->GetEncodingScheme()) {
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:in_size += ((EliasDeltaEncodedNPA *) npa_)->Deserialize(in);
      break;
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:in_size += ((EliasGammaEncodedNPA *) npa_)->Deserialize(in);
      break;
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:in_size += ((WaveletTreeEncodedNPA *) npa_)->Deserialize(in);
      break;
    default:assert(0);
  }

  in.close();

  return in_size;
}

size_t SuccinctCore::MemoryMap(const std::string &path) {
  // Check if directory exists
  struct stat st{};
  assert(stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

  uint8_t *data_beg, *data;
  data = data_beg = (uint8_t *) SuccinctUtils::MemoryMap(path + "/" + path.substr(0, path.size()-9) + ".succinct.metadata");

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
  data += sa_->MemoryMap(data);
  data += isa_->MemoryMap(data);

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
    case NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED:data += ((EliasDeltaEncodedNPA *) npa_)->MemoryMap(data);
      break;
    case NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED:data += ((EliasGammaEncodedNPA *) npa_)->MemoryMap(data);
      break;
    case NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED:
      data += ((WaveletTreeEncodedNPA *) npa_)->MemoryMap(data);
      break;
    default:assert(0);
  }

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
    return std::make_pair(0, -1);

  for (int64_t i = m_len - 2; i >= 0; i--) {
    if (alphabet_map_.find(mgram[i]) != alphabet_map_.end()) {
      col_range.first = alphabet_map_[mgram[i]].first;
      col_range.second = alphabet_map_[alphabet_[alphabet_map_[mgram[i]].second
          + 1]].first - 1;
    } else
      return std::make_pair(0, -1);

    if (col_range.first > col_range.second)
      return std::make_pair(0, -1);

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
      return std::make_pair(0, -1);

    if (col_range.first > col_range.second)
      return std::make_pair(0, -1);

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

std::pair<int64_t, int64_t> SuccinctCore::FwdSearch(const std::string &mgram) {

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

  return std::make_pair(sp, ep);
}

std::pair<int64_t, int64_t> SuccinctCore::ContinueFwdSearch(
    const std::string &mgram, std::pair<int64_t, int64_t> range, size_t len) {

  if (mgram.empty())
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

  return std::make_pair(sp, ep);
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
