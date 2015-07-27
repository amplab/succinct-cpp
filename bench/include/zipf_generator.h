#ifndef ZIPF_GENERATOR_HPP
#define ZIPF_GENERATOR_HPP

#include <cassert>
#include <cstdio>
#include <cmath>
#include <ctime>

class ZipfGenerator {
 public:
  // Constructor for zipf distribution
  ZipfGenerator(double theta, uint64_t n) {
    // Ensure parameters are sane
    assert(n > 0);
    assert(theta >= 0.0);
    assert(theta <= 1.0);

    // srand (time(NULL));

    this->theta_ = theta;
    this->n_ = n;
    this->zdist_ = new double[n];
    this->GenerateZipf();
  }

  ~ZipfGenerator() {
    delete[] zdist_;
  }

  // Returns the next zipf value
  uint64_t Next() {
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
    int64_t hi = n_;
    while (lo != hi) {
      int64_t mid = (lo + hi) / 2;
      if (zdist_[mid] <= r) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }

    return lo;
  }

 private:
  // Generates the zipf probability distribution
  void GenerateZipf() {
    double sum = 0.0;
    double c = 0.0;
    double expo = 1.0 - theta_;
    double sumc = 0.0;
    uint64_t i;

    /*
     * zipfian - p(i) = c / i ^^ (1 - theta)
     * At theta = 1, uniform
     * At theta = 0, pure zipfian
     */

    for (i = 1; i <= n_; i++) {
      sum += 1.0 / pow((double) i, (double) (expo));

    }
    c = 1.0 / sum;

    for (i = 0; i < n_; i++) {
      sumc += c / pow((double) (i + 1), (double) (expo));
      zdist_[i] = sumc;
    }
  }

  typedef struct {
    double probability;               // The access probability
    double cumulative_probability;    // The cumulative access probability
  } ProbabilityValue;

  double theta_;       // The skew parameter (0=pure zipf, 1=pure uniform)
  uint64_t n_;         // The number of objects
  double *zdist_;

};

#endif
