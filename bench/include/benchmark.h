#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <cstdio>
#include <fstream>
#include <vector>

#include <sys/time.h>

#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void) {
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}
#elif defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void) {
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

#elif defined(__powerpc__)

static __inline__ unsigned long long rdtsc(void) {
  unsigned long long int result=0;
  unsigned long int upper, lower,tmp;
  __asm__ volatile(
      "0:                  \n"
      "\tmftbu   %0           \n"
      "\tmftb    %1           \n"
      "\tmftbu   %2           \n"
      "\tcmpw    %2,%0        \n"
      "\tbne     0b         \n"
      : "=r"(upper),"=r"(lower),"=r"(tmp)
  );
  result = upper;
  result = result<<32;
  result = result|lower;

  return(result);
}

#else

#error "No tick counter is available!"

#endif

class Benchmark {
 public:
  typedef unsigned long long int TimeStamp;

  Benchmark() {
  }

  virtual ~Benchmark() {
    randoms_.clear();
    queries_.clear();
  }

  static TimeStamp GetTimestamp() {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_usec + (TimeStamp) now.tv_sec * 1000000;
  }

 protected:
  static const uint64_t kWarmupCount = 1000;
  static const uint64_t kMeasureCount = 100000;
  static const uint64_t kCooldownCount = 1000;

  static const uint64_t kWarmupTime = 60000000;
  static const uint64_t kMeasureTime = 120000000;
  static const uint64_t kCooldownTime = 5000000;

  std::vector<int64_t> randoms_;
  std::vector<std::string> queries_;
};

#endif
