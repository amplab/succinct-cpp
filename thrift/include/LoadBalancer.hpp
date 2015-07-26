#ifndef LOAD_BALANCER_HPP
#define LOAD_BALANCER_HPP

#include <vector>
#include <cstdlib>
#include <assert.h>

class LoadBalancer {
private:
    std::vector<double> cum_dist;

public:
    LoadBalancer(std::vector<double> distribution) {
        std::sort(distribution.begin(), distribution.end());

        double sum = 0;
        for(size_t i = 0; i < distribution.size(); i++) {
            sum += distribution.at(i);
            cum_dist.push_back(sum);
        }
        assert(sum == 1.0);
    }

    uint32_t get_replica() {
        double r = ((double) rand() / (RAND_MAX));
        for(size_t i = 0; i < cum_dist.size(); i++) {
            if(r < cum_dist.at(i)) {
                return i;
            }
        }
        return 0;
    }

    uint32_t num_replicas() {
        return cum_dist.size();
    }
};

#endif /* LOAD_BALANCER_HPP */
