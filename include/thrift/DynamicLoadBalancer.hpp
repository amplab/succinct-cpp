#ifndef DYNAMIC_LOAD_BALANCER_HPP
#define DYNAMIC_LOAD_BALANCER_HPP

#include <cassert>
#include <cstdlib>
#include <atomic>

class DynamicLoadBalancer {
private:
    uint32_t replication;
    double *cum_prio;

public:
    DynamicLoadBalancer(uint32_t replication) {
        this->replication = replication;
        this->cum_prio = new double[replication];
    }

    uint32_t get_replica(std::atomic<uint64_t> *priorities) {

        // Get the distribution
        double sum = 0.0;
        for(uint32_t i = 0; i < replication; i++) {
            sum += priorities[i];
            this->cum_prio[i] = sum;
        }

        double r = ((double) rand() / (RAND_MAX));
        for(size_t i = 0; i < replication; i++) {
            if(r < (cum_prio[i] / sum)) {
                return i;
            }
        }
        return 0;
    }
};

#endif
