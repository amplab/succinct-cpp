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
        fprintf(stderr, "Initialized load balancer with replication = %u\n", replication);
    }

    uint32_t get_replica(std::atomic<double> *priorities) {

        // Get the distribution
        double sum = 0.0;
        for(uint32_t i = 0; i < replication; i++) {
            double prio = priorities[i];
            sum += (1 / (prio + 1));  // To prevent zero priority cases
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

    uint32_t get_replica(std::atomic<uint64_t> *priorities) {
        // Get the distribution
        double sum = 0.0;
        for(uint32_t i = 0; i < replication; i++) {
            double prio = priorities[i];
            sum += (1 / (prio + 1));  // To prevent zero priority cases
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

    void get_latest_allocations(std::vector<double> &allocations) {
        double prev = 0.0;
        for(uint32_t i = 0; i < replication; i++) {
            allocations.push_back((cum_prio[i] - prev) / cum_prio[replication - 1]);
            prev = cum_prio[i];
        }
    }
};

#endif
