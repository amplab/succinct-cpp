#ifndef ZIPF_GENERATOR_HPP
#define ZIPF_GENERATOR_HPP

#include <cassert>
#include <cstdio>

class ZipfGenerator {
private:

    typedef struct {
        double prob;        // The access probability
        double cum_prob;    // The cumulative access probability
    } probvals;

    double theta;       // The skew parameter (0=pure zipf, 1=pure uniform)
    uint64_t N;         // The number of objects
    double *zdist;

    // Generates the zipf probability distribution
    void gen_zipf() {
        double sum = 0.0;
        double c = 0.0;
        double expo = 1.0 - theta;
        double sumc = 0.0;
        uint64_t i;

        /*
         * zipfian - p(i) = c / i ^^ (1 - theta)
         * At theta = 1, uniform
         * At theta = 0, pure zipfian
         */

        for (i = 1; i <= N; i++) {
           sum += 1.0 / pow((double) i, (double) (expo));

        }
        c = 1.0 / sum;

        for (i = 0; i < N; i++) {
            sumc +=  c / pow((double) (i + 1), (double) (expo));
            zdist[i] = sumc;
        }
    }

public:

    // Constructor for zipf distribution
    ZipfGenerator(double theta, uint64_t N) {
        // Ensure parameters are sane
        assert(N > 0);
        assert(theta >= 0.0);
        assert(theta <= 1.0);

        this->theta = theta;
        this->N = N;
        this->zdist = new double[N];
        this->gen_zipf();
    }

    ~ZipfGenerator() {
        delete[] zdist;
    }

    // Returns the next zipf value
    uint64_t next() {
        double r = ((double) rand() / (RAND_MAX));
        /*
        // Inefficient
        for(uint64_t i = 0; i < N; i++) {
            if(r < cum_zdist[i]) {
                return i;
            }
        }
        */
        int64_t lo = 0;
        int64_t hi = N;
        while (lo != hi) {
            int64_t mid = (lo + hi) / 2;
            if(zdist[mid] <= r) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        return lo;
    }

    // Displays the distribution
    void display() {
        fprintf(stderr, "Printing out distribution:\n");
        for(uint64_t i = 0; i < N; i++) {
            fprintf(stderr, "%f\n", i, zdist[i]);
        }
    }

};

#endif
