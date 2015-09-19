#ifndef SHARD_CONFIG_H_
#define SHARD_CONFIG_H_

typedef int32_t IdType;

typedef struct ShardMetadata {
 public:
  ShardMetadata() {
    sampling_rate = 0;
    is_primary = false;
  }

  ShardMetadata(int32_t sampling_rate, bool is_primary,
                std::vector<IdType>& replicas,
                std::vector<double>& distribution) {
    this->sampling_rate = sampling_rate;
    this->is_primary = is_primary;
    this->replicas = replicas;
    this->distribution = distribution;
    ComputeCumulativeDistribution();
  }

  void ComputeCumulativeDistribution() {
    double sum = 0;
    cum_dist.clear();
    for (size_t i = 0; i < distribution.size(); i++) {
      sum += distribution.at(i);
      cum_dist.push_back(sum);
    }
  }

  int32_t sampling_rate;              // Sampling Rate for the shard
  bool is_primary;                    // Is this a primary shard?
  std::vector<IdType> replicas;       // List of replicas; empty if not primary
  std::vector<double> distribution;  // Fraction of queries to be directed to replicas
  std::vector<double> cum_dist;

} ShardMetadata;

typedef std::map<IdType, ShardMetadata> ConfigMap;

#endif
