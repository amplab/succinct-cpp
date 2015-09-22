#include "succinct_shard.h"

SuccinctShard::SuccinctShard(uint32_t id, std::string filename,
                             SuccinctMode s_mode, uint32_t sa_sampling_rate,
                             uint32_t isa_sampling_rate,
                             uint32_t npa_sampling_rate,
                             SamplingScheme sa_sampling_scheme,
                             SamplingScheme isa_sampling_scheme,
                             NPA::NPAEncodingScheme npa_encoding_scheme,
                             uint32_t context_len, uint32_t sampling_range)
    : SuccinctCore(filename.c_str(), s_mode, sa_sampling_rate,
                   isa_sampling_rate, npa_sampling_rate, context_len,
                   sa_sampling_scheme, isa_sampling_scheme, npa_encoding_scheme,
                   sampling_range) {

  this->id_ = id;

  switch (s_mode) {
    case SuccinctMode::CONSTRUCT_IN_MEMORY:
    case SuccinctMode::CONSTRUCT_MEMORY_MAPPED: {
      // Determine value offsets from original input
      std::ifstream input(filename);
      int64_t value_offset = 0;
      std::vector<int64_t> value_offsets;
      for (uint32_t i = 0; !input.eof(); i++) {
        value_offsets.push_back(value_offset);
        std::string line;
        std::getline(input, line, '\n');
        value_offset += line.length() + 1;
      }
      input.close();
      num_keys_ = value_offsets.size();
      value_offsets_ = new int64_t[num_keys_];
      std::copy(value_offsets.begin(), value_offsets.end(), value_offsets_);
      value_offsets.clear();
      break;
    }
    case SuccinctMode::LOAD_IN_MEMORY: {
      // Read keys, value offsets, and invalid bitmap from file
      std::ifstream keyval(succinct_path_ + "/keyval");

      // Read values
      keyval.read(reinterpret_cast<char *>(&num_keys_), sizeof(size_t));
      value_offsets_ = new int64_t[num_keys_];
      for (size_t i = 0; i < num_keys_; i++) {
        uint64_t value_offset;
        keyval.read(reinterpret_cast<char *>(&value_offset), sizeof(int64_t));
        value_offsets_[i] = value_offset;
      }
      break;
    }
    case SuccinctMode::LOAD_MEMORY_MAPPED: {
      uint8_t *data, *data_beg;
      data = data_beg = (uint8_t *) SuccinctUtils::MemoryMapUnpopulated(
          succinct_path_ + "/keyval");

      // Read values
      num_keys_ = *((int64_t *) data);
      data += sizeof(size_t);
      value_offsets_ = (int64_t *) data;
      data += (sizeof(int64_t) * num_keys_);

      break;
    }
  }
}

uint64_t SuccinctShard::ComputeContextValue(const char *p, uint64_t i) {
  uint64_t val = 0;

  for (uint64_t t = i; t < i + npa_->GetContextLength(); t++) {
    val = val * alphabet_size_ + alphabet_map_[p[t]].second;
  }

  return val;
}

std::pair<int64_t, int64_t> SuccinctShard::GetRange(const char *p,
                                                    uint64_t len) {
  std::pair<int64_t, int64_t> range(0, -1);
  int64_t sp, ep, c1, c2;

  if (alphabet_map_.find(p[len - 1]) != alphabet_map_.end()) {
    sp = (alphabet_map_[p[len - 1]]).first;
    ep = alphabet_map_[alphabet_[alphabet_map_[p[len - 1]].second + 1]].first
        - 1;
  } else {
    return range;
  }

  for (int64_t i = len - 2; i >= 0; i--) {
    if (alphabet_map_.find(p[i]) != alphabet_map_.end()) {
      c1 = alphabet_map_[p[i]].first;
      c2 = alphabet_map_[alphabet_[alphabet_map_[p[i]].second + 1]].first - 1;
    } else {
      return range;
    }

    if (c2 < c1) {
      return range;
    }

    sp = npa_->BinarySearch(sp, c1, c2, false);
    ep = npa_->BinarySearch(ep, sp, c2, true);

    if (sp > ep) {
      return range;
    }
  }

  range.first = sp;
  range.second = ep;

  return range;
}

std::string SuccinctShard::Name() {
  return filename_;
}

size_t SuccinctShard::GetNumKeys() {
  return num_keys_;
}

uint32_t SuccinctShard::GetSASamplingRate() {
  return sa_->GetSamplingRate();
}

uint32_t SuccinctShard::GetISASamplngRate() {
  return isa_->GetSamplingRate();
}

uint32_t SuccinctShard::GetNPASamplingRate() {
  return npa_->GetSamplingRate();
}

void SuccinctShard::Access(std::string& result, int64_t key, int32_t offset,
                           int32_t len) {
  result = "";
  int64_t pos = key;
  int64_t start = value_offsets_[pos] + offset;
  int64_t end =
      ((size_t) (pos + 1) < num_keys_) ? value_offsets_[pos + 1] : input_size_;
  len = fmin(len, end - start - 1);
  result.resize(len);
  uint64_t idx = LookupISA(start);
  for (int64_t i = 0; i < len; i++) {
    result[i] = alphabet_[LookupC(idx)];
    uint64_t next_pos = start + i + 1;
    if ((next_pos % isa_->GetSamplingRate()) == 0) {
      idx = LookupISA(next_pos);
    } else {
      idx = LookupNPA(idx);
    }
  }
}

void SuccinctShard::Get(std::string& result, int64_t key) {
  result = "";
  int64_t pos = key;
  int64_t start = value_offsets_[pos];
  int64_t end =
      ((size_t) (pos + 1) < num_keys_) ? value_offsets_[pos + 1] : input_size_;
  int64_t len = end - start - 1;
  result.resize(len);
  uint64_t idx = LookupISA(start);
  for (int64_t i = 0; i < len; i++) {
    result[i] = alphabet_[LookupC(idx)];
    uint64_t next_pos = start + i + 1;
    if (isa_->IsSampled(next_pos)) {
      idx = LookupISA(next_pos);
    } else {
      idx = LookupNPA(idx);
    }
  }
}

int64_t SuccinctShard::GetKey(const int64_t value_offset) {
  int64_t pos = std::prev(
      std::upper_bound(value_offsets_, value_offsets_ + num_keys_,
                       value_offset)) - value_offsets_;
  return (pos >= num_keys_) ? -1 : pos;
}

void SuccinctShard::Search(std::set<int64_t> &result, const std::string& str) {
  std::pair<int64_t, int64_t> range = GetRange(str.c_str(), str.length());
  if (range.first > range.second)
    return;
  for (int64_t i = range.first; i <= range.second; i++) {
    int64_t key = GetKey((int64_t) LookupSA(i));
    if (key >= 0) {
      result.insert(key);
    }
  }
}

void SuccinctShard::RegexSearch(std::set<std::pair<size_t, size_t>> &result,
                                const std::string& query, bool opt) {
  SRegEx re(query, this, opt);
  re.execute();
  re.get_results(result);
}

void SuccinctShard::RegexCount(std::vector<size_t> &result,
                               const std::string& query) {
  SRegEx re(query, this);
  re.count(result);
}

int64_t SuccinctShard::Count(const std::string& str) {
  std::set<int64_t> result;
  Search(result, str);
  return result.size();
}

void SuccinctShard::FlatExtract(std::string& result, int64_t offset,
                                int64_t len) {
  result = "";
  uint64_t idx = LookupISA(offset);
  for (uint64_t k = 0; k < len; k++) {
    result += alphabet_[LookupC(idx)];
    idx = LookupNPA(idx);
  }
}

int64_t SuccinctShard::FlatCount(const std::string& str) {
  std::pair<int64_t, int64_t> range = GetRange(str.c_str(), str.length());
  return range.second - range.first + 1;
}

void SuccinctShard::FlatSearch(std::vector<int64_t>& result,
                               const std::string& str) {
  std::pair<int64_t, int64_t> range = GetRange(str.c_str(), str.length());
  if (range.first > range.second)
    return;
  result.reserve((uint64_t) (range.second - range.first + 1));
  for (int64_t i = range.first; i <= range.second; i++) {
    result.push_back((int64_t) LookupSA(i));
  }
}

std::ifstream::pos_type filesize(const std::string& filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

void SuccinctShard::FetchData(std::string& result,
                                 const std::string& filename,
                                 const int64_t offset, const int64_t length) {

  std::string path = succinct_path_ + "/" + filename;
  fprintf(stderr, "Offset = %lld, Length = %lld\n", offset, length);

  int64_t file_size = filesize(path);
  int64_t read_size = SuccinctUtils::Min(length, (file_size - offset));
  result.resize(read_size);

  std::ifstream in(path);
  assert(in);

  in.seekg(offset);
  in.read(&result[0], read_size);
}

size_t SuccinctShard::Serialize() {
  size_t out_size = SuccinctCore::Serialize();

  // Write value offsets to file
  std::ofstream keyval(succinct_path_ + "/keyval");

  keyval.write(reinterpret_cast<const char *>(&(num_keys_)), sizeof(int64_t));
  out_size += sizeof(int64_t);
  for (size_t i = 0; i < num_keys_; i++) {
    keyval.write(reinterpret_cast<const char *>(&value_offsets_[i]),
                 sizeof(int64_t));
    out_size += sizeof(int64_t);
  }

  return out_size;
}

size_t SuccinctShard::Deserialize() {
  size_t in_size = SuccinctCore::Deserialize();

  // Read value offsets from file
  std::ifstream keyval(succinct_path_ + "/keyval");

  // Read values
  keyval.read(reinterpret_cast<char *>(&num_keys_), sizeof(int64_t));
  in_size += sizeof(int64_t);
  value_offsets_ = new int64_t[num_keys_];
  for (size_t i = 0; i < num_keys_; i++) {
    uint64_t value_offset;
    keyval.read(reinterpret_cast<char *>(&value_offset), sizeof(int64_t));
    value_offsets_[i] = value_offset;
    in_size += sizeof(int64_t);
  }

  return in_size;
}

size_t SuccinctShard::MemoryMap() {
  size_t core_size = SuccinctCore::MemoryMap();

  uint8_t *data, *data_beg;
  data = data_beg = (uint8_t *) SuccinctUtils::MemoryMapUnpopulated(
      succinct_path_ + "/keyval");

  // Read values
  num_keys_ = *((int64_t *) data);
  data += sizeof(size_t);
  value_offsets_ = (int64_t *) data;
  data += (sizeof(int64_t) * num_keys_);

  return core_size + (data - data_beg);
}

size_t SuccinctShard::StorageSize() {
  size_t tot_size = SuccinctCore::StorageSize();
  tot_size += sizeof(int64_t) * (num_keys_ + 1);
  return tot_size;
}
