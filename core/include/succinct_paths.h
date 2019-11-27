#ifndef SUCCINCT_SUCCINCT_PATHS_H
#define SUCCINCT_SUCCINCT_PATHS_H

#include "succinct_semistructured_shard.h"

template<typename T>
std::string pad(T val, size_t len = std::numeric_limits<T>::digits10 + 1) {
  std::ostringstream ss;
  ss << std::setw(len) << std::setfill('0') << val;
  return ss.str();
}

#define WRITE(attr_name) \
  fout.attr_key_to_delimiter_.at(#attr_name) << attr_name << fout.attr_key_to_delimiter_.at(#attr_name)

#define WRITE_PAD(attr_name) \
  fout.attr_key_to_delimiter_.at(#attr_name) << pad(attr_name) << fout.attr_key_to_delimiter_.at(#attr_name)

void FormatPathsInput(FormatterOutput &fout, const std::string &filename) {
  std::ifstream in(filename);
  std::string line;
  std::ofstream out(filename + ".formatted");

  uint8_t cur_delim = 128;

  // Initialize delimiters
  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("inode_num", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "inode_num"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("file_change_type", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "file_change_type"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("file_type", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "file_type"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("create_time", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "create_time"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("modify_time", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "modify_time"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("access_time", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "access_time"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("change_time", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "change_time"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("owner_id", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "owner_id"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("group_id", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "group_id"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("file_size", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "file_size"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("file_name_size", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "file_name_size"));
  ++cur_delim;

  fout.attr_key_to_delimiter_.insert(FwdMap::value_type("file_name", cur_delim));
  fout.delimiter_to_attr_key_.insert(BwdMap::value_type(cur_delim, "file_name"));

  fprintf(stderr, "Parsing input %s\n", filename.c_str());

  while (std::getline(in, line)) {
    std::istringstream iss(line);
    uint64_t inode_num;
    char file_change_type, file_type;
    int64_t create_time, modify_time, access_time, change_time, file_size;
    int32_t owner_id, group_id, file_name_size;
    char file_name[2048];
    if (!(iss >> inode_num >> file_change_type >> file_type >> create_time >> modify_time >> access_time
              >> change_time >> owner_id >> group_id >> file_size >> file_name_size >> file_name)) {
      fprintf(stderr, "Incorrect formatting for paths file.\n");
      exit(0);
    } // error
    fout.keys_.push_back(inode_num);
    fout.value_offsets_.push_back(out.tellp());
    out << WRITE_PAD(inode_num) << WRITE(file_change_type) << WRITE(file_type) << WRITE_PAD(create_time)
        << WRITE_PAD(modify_time) << WRITE_PAD(access_time) << WRITE_PAD(change_time) << WRITE_PAD(owner_id)
        << WRITE_PAD(group_id) << WRITE_PAD(file_size) << WRITE_PAD(file_name_size) << WRITE(file_name);
  }
  out.close();
}

class SuccinctPathsShard : public SuccinctSemistructuredShard {
 public:
  explicit SuccinctPathsShard(const std::string &filename,
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
                              uint32_t sampling_range = 1024) : SuccinctSemistructuredShard(filename,
                                                                                            FormatPathsInput,
                                                                                            s_mode,
                                                                                            sa_sampling_rate,
                                                                                            isa_sampling_rate,
                                                                                            npa_sampling_rate,
                                                                                            context_len,
                                                                                            sa_sampling_scheme,
                                                                                            isa_sampling_scheme,
                                                                                            npa_encoding_scheme,
                                                                                            sampling_range) {}

};

#endif //SUCCINCT_SUCCINCT_PATHS_H
