#ifndef DYNAMIC_LOAD_BALANCER_HPP
#define DYNAMIC_LOAD_BALANCER_HPP

#include <cassert>
#include <cstdlib>

class DynamicLoadBalancer {
private:
    uint32_t num_shards;
    uint32_t replication;
    double **cum_dist;

    void priorities_to_distribution(std::vector<double> &distribution, std::vector<uint64_t> &priorities) {
        distribution.resize(replication);

        // Invert priorities
        double sum = 0.0;
        for(uint32_t i = 0; i < replication; i++) {
            distribution[i] = 1.0 / (double) (priorities[i]);
            sum += distribution[i];
        }

        // Normalize distribution
        for(uint32_t i = 0; i < replication; i++) {
            distribution[i] /= sum;
        }
    }

public:
    DynamicLoadBalancer(uint32_t num_shards, uint32_t replication) {
        this->num_shards = num_shards;
        this->replication = replication;
        this->cum_dist = new double*[num_shards];
        for(uint32_t i = 0; i < num_shards; i++) {
            this->cum_dist[i] = new double[replication];
            double sum = 0.0;
            for(uint32_t j = 0; j < replication; j++) {
                sum += 1.0 / (double)(replication);
                this->cum_dist[i][j] = sum;
            }
            assert(sum == 1.0);
        }
    }

    void update_distribution(uint32_t primary_shard_id, std::vector<uint64_t> &priorities) {
        std::vector<double> dist;
        priorities_to_distribution(dist, priorities);
        double sum = 0.0;
        for(uint32_t i = 0; i < replication; i++) {
            sum += dist[i];
            this->cum_dist[primary_shard_id][i] = sum;
        }
    }

    uint32_t get_replica(uint32_t primary_shard_id) {
        double r = ((double) rand() / (RAND_MAX));
        for(size_t i = 0; i < replication; i++) {
            if(r < cum_dist[primary_shard_id][i]) {
                return i;
            }
        }
        return 0;
    }
};

#endif
