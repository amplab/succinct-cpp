#ifndef SHARD_CONFIG_H_
#define SHARD_CONFIG_H_

#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cassert>

typedef int32_t IdType;
typedef enum {
  /* For Replication */
  kReplica = 0,
  kPrimary = 1,
  /* For Erasure Coding */
  kParity = 2,
  kData = 3
} ShardType;

// Erasure code metadata
typedef struct Stripe {
 public:

  typedef std::vector<int32_t>::iterator BlockIdIterator;
  BlockIdIterator FindDataBlock(int32_t block_id) {
    return std::find(data_blocks.begin(), data_blocks.end(), block_id);
  }

  BlockIdIterator FindParityBlock(int32_t block_id) {
    return std::find(parity_blocks.begin(), parity_blocks.end(), block_id);
  }

  bool ContainsBlock(int32_t block_id) {
    return (FindDataBlock(block_id) != data_blocks.end())
        || (FindParityBlock(block_id) != parity_blocks.end());
  }

  void GetRecoveryBlocks(std::vector<int32_t>& recovery_blocks,
                         int32_t failed_block) {
    BlockIdIterator data_pos = FindDataBlock(failed_block);
    BlockIdIterator parity_pos = FindParityBlock(failed_block);

    if (data_pos != data_blocks.end()) {
      fprintf(stderr, "Failed Block is a data block\n");

      // Pick all parity blocks
      for (auto parity_block : parity_blocks) {
        recovery_blocks.push_back(parity_block);
      }

      // Pick 8 remaining data blocks
      for (auto data_block : data_blocks) {
        if (data_block == failed_block)
          continue;
        recovery_blocks.push_back(data_block);
        if (recovery_blocks.size() == 10)
          break;
      }

    } else if (parity_pos != parity_blocks.end()) {
      fprintf(stderr, "Failed Block is a parity block\n");

      // Pick one parity block
      for (auto parity_block : parity_blocks) {
        if (parity_block == failed_block)
          continue;
        recovery_blocks.push_back(parity_block);
      }

      // Pick 9 remaining data blocks
      for (auto data_block : data_blocks) {
        recovery_blocks.push_back(data_block);
        if (recovery_blocks.size() == 10) break;
      }
    } else {
      // This should not happen
      assert(0);
    }
  }

  std::vector<int32_t> data_blocks;
  std::vector<int32_t> parity_blocks;
} Stripe;

typedef std::vector<Stripe> StripeList;

typedef struct ShardMetadata {
 public:
  ShardMetadata() {
    sampling_rate = 0;
    shard_type = ShardType::kPrimary;
  }

  ShardMetadata(int32_t sampling_rate, ShardType shard_type,
                std::vector<IdType>& replicas,
                std::vector<double>& distribution) {
    this->sampling_rate = sampling_rate;
    this->shard_type = shard_type;
    this->replicas = replicas;
    this->distribution = distribution;
    ComputeCumulativeDistribution();
  }

  bool ContainsReplica(IdType id) {
    return std::find(replicas.begin(), replicas.end(), id) != replicas.end();
  }

  void ComputeCumulativeDistribution() {
    double sum = 0;
    cum_dist.clear();
    for (size_t i = 0; i < distribution.size(); i++) {
      sum += distribution.at(i);
      cum_dist.push_back(sum);
    }
  }

  int32_t sampling_rate;                // Sampling Rate for the shard
  ShardType shard_type;                 // Type of the shard

  std::vector<IdType> replicas;        // List of replicas; empty if not primary
  std::vector<double> distribution;  // Fraction of queries to be directed to replicas
  std::vector<double> cum_dist;  // Cumulative distribution of queries directed to replicas
} ShardMetadata;

typedef std::map<IdType, ShardMetadata> ConfigMap;

template<typename T>
void ParseCsvEntry(std::vector<T> &out, std::string csv_entry) {
  std::string delimiter = ",";
  size_t pos = 0;
  std::string elem;
  while ((pos = csv_entry.find(delimiter)) != std::string::npos) {
    elem = csv_entry.substr(0, pos);
    out.push_back((T) atof(elem.c_str()));
    csv_entry.erase(0, pos + delimiter.length());
  }

  if (csv_entry != "-") {
    out.push_back(atof(csv_entry.c_str()));
  } else {
    assert(out.empty());
  }
}

void ParseConfig(ConfigMap& conf, std::string& conf_file) {
  std::ifstream conf_stream(conf_file);

  std::string line;
  while (std::getline(conf_stream, line)) {
    IdType id;
    ShardMetadata sdata;
    std::istringstream iss(line);

    // Get ID and sampling rate
    iss >> id >> sdata.sampling_rate;

    // Get shard type
    uint32_t shard_type;
    iss >> shard_type;
    sdata.shard_type = (ShardType) shard_type;

    // Get and parse ids
    std::string ids;
    iss >> ids;
    ParseCsvEntry<int32_t>(sdata.replicas, ids);

    // Get and parse distributions
    std::string distributions;
    iss >> distributions;
    ParseCsvEntry<double>(sdata.distribution, distributions);

    sdata.ComputeCumulativeDistribution();

    conf[id] = sdata;

    fprintf(stderr,
            "Shard ID = %d, Sampling Rate = %d, Shard Type = %d, Replicas: ",
            id, sdata.sampling_rate, sdata.shard_type);

    for (auto replica : sdata.replicas) {
      fprintf(stderr, "%d;", replica);
    }

    fprintf(stderr, ", Distribution: ");
    for (auto prob : sdata.distribution) {
      fprintf(stderr, "%f;", prob);
    }

    fprintf(stderr, ", Cumulative Distribution: ");
    for (auto cum_prob : sdata.cum_dist) {
      fprintf(stderr, "%f;", cum_prob);
    }

    fprintf(stderr, "\n");
  }

  fprintf(stderr, "Read %zu configurations\n", conf.size());
}

void ParseErasureConfig(StripeList& s_list, std::string conf_file) {
  std::ifstream conf_stream(conf_file);
  std::string line;
  while (std::getline(conf_stream, line)) {
    IdType id;
    Stripe sdata;
    std::string data, parity;
    std::istringstream iss(line);

    // Get data and parity CSV entries
    iss >> data >> parity;

    ParseCsvEntry(sdata.data_blocks, data);
    ParseCsvEntry(sdata.parity_blocks, parity);

    fprintf(stderr, "Stripe: Data: ");

    for (auto data_block : sdata.data_blocks) {
      fprintf(stderr, "%d;", data_block);
    }

    fprintf(stderr, ", Parity: ");
    for (auto parity_blocks : sdata.parity_blocks) {
      fprintf(stderr, "%d;", parity_blocks);
    }
    fprintf(stderr, "\n");

    s_list.push_back(sdata);
  }

  fprintf(stderr, "Read %zu stripes\n", s_list.size());
}

#endif
