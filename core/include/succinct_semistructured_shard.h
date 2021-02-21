#ifndef SUCCINCT_SEMISTRUCTURED_SHARD_H_
#define SUCCINCT_SEMISTRUCTURED_SHARD_H_

#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "succinct_shard.h"

class SuccinctSemistructuredShard : public SuccinctShard {
 public:
  typedef std::unordered_map<std::string, uint8_t> FwdMap;
  typedef std::unordered_map<uint8_t, std::string> BwdMap;

  explicit SuccinctSemistructuredShard(const std::string &filename,
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
                                       uint32_t sampling_range = 1024)
      : SuccinctShard() {
    switch (s_mode) {
      case SuccinctMode::CONSTRUCT_IN_MEMORY: {
        std::string formatted = Format(filename);
        Construct(formatted, sa_sampling_rate, isa_sampling_rate,
                  npa_sampling_rate, context_len, sa_sampling_scheme,
                  isa_sampling_scheme, npa_encoding_scheme, sampling_range);
        invalid_offsets_ = new Bitmap;
        InitBitmap(&invalid_offsets_, keys_.size(), s_allocator);
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

        // Read keys, value offsets, and invalid bitmap from file
        std::ifstream keyval(filename + "/keyval");

        // Read keys
        size_t keys_size;
        keyval.read(reinterpret_cast<char *>(&keys_size), sizeof(size_t));
        keys_.reserve(keys_size);
        for (size_t i = 0; i < keys_size; i++) {
          uint64_t key;
          keyval.read(reinterpret_cast<char *>(&key), sizeof(int64_t));
          keys_.push_back(key);
        }

        // Read values
        size_t value_offsets_size;
        keyval.read(reinterpret_cast<char *>(&value_offsets_size), sizeof(size_t));
        value_offsets_.reserve(value_offsets_size);
        for (size_t i = 0; i < value_offsets_size; i++) {
          uint64_t value_offset;
          keyval.read(reinterpret_cast<char *>(&value_offset), sizeof(int64_t));
          value_offsets_.push_back(value_offset);
        }

        // Read bitmap
        SuccinctBase::DeserializeBitmap(&invalid_offsets_, keyval);
        break;
      }
      case SuccinctMode::LOAD_MEMORY_MAPPED: {
        Allocate(sa_sampling_rate, isa_sampling_rate, npa_sampling_rate,
                 context_len, sa_sampling_scheme, isa_sampling_scheme,
                 npa_encoding_scheme, sampling_range);
        MemoryMap(filename);
        uint8_t *data, *data_beg;
        data = data_beg = (uint8_t *) SuccinctUtils::MemoryMap(
            filename + "/keyval");

        // Read keys
        size_t keys_size = *((size_t *) data);
        data += sizeof(size_t);
        buf_allocator<int64_t> key_allocator((int64_t *) data);
        keys_ = std::vector<int64_t>((int64_t *) data,(int64_t *) data + keys_size, key_allocator);
        data += (sizeof(int64_t) * keys_size);

        // Read values
        size_t value_offsets_size = *((size_t *) data);
        data += sizeof(size_t);
        buf_allocator<int64_t> value_offsets_allocator((int64_t *) data);
        value_offsets_ = std::vector<int64_t>(
            (int64_t *) data, (int64_t *) data + value_offsets_size,
            value_offsets_allocator);
        data += (sizeof(int64_t) * value_offsets_size);

        // Read bitmap
        data += SuccinctBase::MemoryMapBitmap(&invalid_offsets_, data);
        break;
      }
    }
  }

  int64_t CountAttribute(const std::string &attr_key,
                         const std::string &attr_val) {
    if (attr_key_to_delimiter_map_.find(attr_key)
        == attr_key_to_delimiter_map_.end())
      return 0;

    char delim = attr_key_to_delimiter_map_.at(attr_key);
    std::string query = delim + attr_val + delim;
    return Count(query);
  }

  void SearchAttribute(std::set<int64_t> &keys, const std::string &attr_key,
                       const std::string &attr_val) {
    if (attr_key_to_delimiter_map_.find(attr_key)
        == attr_key_to_delimiter_map_.end())
      return;

    char delim = attr_key_to_delimiter_map_.at(attr_key);
    std::string query = delim + attr_val + delim;
    Search(keys, query);
  }

  void Get(std::string &result, int64_t key) override {
    std::string data;
    SuccinctShard::Get(data, key);

    // Format
    size_t i = 0;
    while (i < data.size()) {
      uint8_t delim = data[i];
      if (i++ != 0)
        result += ",";
      std::string attr_key = delimiter_to_attr_key_map_.at(delim);
      std::string attr_val = ExtractField(data, i, delim);
      result += (attr_key + "=" + attr_val);
      i += (attr_val.size() + 1);
    }
  }

  void Get(std::string &result, int64_t key, std::string &attr_key) {
    std::string data;
    SuccinctShard::Get(data, key);

    // Format
    size_t i = 0;
    while (i < data.size()) {
      uint8_t delim = data[i++];
      std::string _attr_key = delimiter_to_attr_key_map_.at(delim);
      std::string _attr_val = ExtractField(data, i, delim);
      if (_attr_key == attr_key) {
        result = _attr_val;
        return;
      }
      i += (_attr_val.size() + 1);
    }
  }

 protected:
  static std::string ExtractField(std::string &data, size_t start_offset, uint8_t delim) {
    size_t i = start_offset;
    while (((uint8_t) data[i]) != delim) i++;
    return data.substr(start_offset, i - start_offset);
  }

  std::string Format(const std::string &filename, char delim = ',') {
    std::string outf = filename + ".tmp.formatted";
    std::ifstream infile(filename);
    std::ofstream formatted(outf);
    std::string line;
    int64_t line_no = 0;
    while (std::getline(infile, line)) {
      if (line_no != 0) {
        char newline = '\n';
        formatted.write(reinterpret_cast<const char *>(&newline), sizeof(uint8_t));
      }
      std::stringstream linestream(line);
      std::string attr_val_pair;
      int64_t attr_val_no = 1;
      keys_.push_back(line_no);
      value_offsets_.push_back(formatted.tellp());
      while (std::getline(linestream, attr_val_pair, delim)) {
        std::string::size_type pos = attr_val_pair.find('=');
        if (pos != std::string::npos) {
          std::string attr_key = attr_val_pair.substr(0, pos);
          std::string attr_val = attr_val_pair.substr(pos + 1);
          if (attr_key_to_delimiter_map_.find(attr_key)
              == attr_key_to_delimiter_map_.end()) {
            if (cur_delim_ == 255) {
              fprintf(stderr, "Currently support <= 128 unique attribute keys.\n");
              exit(0);
            }
            // Create new entry
            attr_key_to_delimiter_map_.insert(FwdMap::value_type(attr_key, cur_delim_));
            delimiter_to_attr_key_map_.insert(BwdMap::value_type(cur_delim_, attr_key));
            cur_delim_++;
          }
          uint8_t attr_delim = attr_key_to_delimiter_map_.at(attr_key);
          const char *attr_val_str = attr_val.c_str();
          formatted.write(reinterpret_cast<const char *>(&attr_delim), sizeof(uint8_t));
          formatted.write(reinterpret_cast<const char *>(attr_val_str), sizeof(char) * attr_val.length());
          formatted.write(reinterpret_cast<const char *>(&attr_delim), sizeof(uint8_t));
        } else {
          fprintf(stderr, "Invalid attribute-value pair [%s] %lld on line %lld\n",
                  attr_val_pair.c_str(), attr_val_no, line_no + 1);
          exit(0);
        }
      }
      line_no++;
    }
    formatted.close();
    infile.close();
    return outf;
  }

  FwdMap attr_key_to_delimiter_map_;
  BwdMap delimiter_to_attr_key_map_;
  uint8_t cur_delim_ = 128;
};

#endif /* SUCCINCT_SEMISTRUCTURED_SHARD_H_ */
