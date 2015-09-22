#ifndef LOAD_BALANCER_HPP
#define LOAD_BALANCER_HPP

#include <vector>
#include <cstdlib>
#include <assert.h>

#include "shard_config.h"

class LoadBalancer {
 public:
  LoadBalancer(ConfigMap &conf) {
    conf_ = conf;
  }

  uint32_t GetReplica(IdType primary_id) {
    ShardMetadata sdata = conf_.at(primary_id);
    assert(
        sdata.shard_type == ShardType::kPrimary
            || sdata.shard_type == ShardType::kData);
    double r = ((double) rand() / (RAND_MAX));
    for (size_t i = 0; i < sdata.cum_dist.size(); i++) {
      if (r < sdata.cum_dist.at(i)) {
        return sdata.replicas.at(i);
      }
    }
    return primary_id;
  }

  uint32_t NumReplicas(IdType primary_id) {
    return conf_.at(primary_id).replicas.size();
  }

  ConfigMap GetConf() {
    return conf_;
  }

 private:
  ConfigMap conf_;

};

#endif /* LOAD_BALANCER_HPP */
