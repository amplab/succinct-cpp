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
    probvals *zdist;    // The probability distribution

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
            zdist[i].prob = c / pow((double) (i + 1), (double) (expo));
            sumc +=  zdist[i].prob;
            zdist[i].cum_prob = sumc;
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
        this->zdist = new probvals[N];
        this->gen_zipf();
    }

    ~ZipfGenerator() {
        delete[] zdist;
    }

    // Returns the next zipf value
    uint64_t next() {
        double r = ((double) rand() / (RAND_MAX));
        for(uint64_t i = 0; i < N; i++) {
            if(r < zdist[i].cum_prob) {
                return i;
            }
        }
        return N;
    }

    // Displays the distribution
    void display() {
        fprintf(stderr, "Printing out distribution:\n");
        for(uint64_t i = 0; i < N; i++) {
            fprintf(stderr, "%f\t%f\n", i, zdist[i].prob, zdist[i].cum_prob);
        }
    }

};

#endif
