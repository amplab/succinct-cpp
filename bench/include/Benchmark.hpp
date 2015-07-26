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
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
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
protected:
    typedef unsigned long long int time_t;
    typedef unsigned long count_t;

    const count_t WARMUP_N = 100000;
    const count_t MEASURE_N = 100000;
    const count_t COOLDOWN_N = 5000;

    static const count_t WARMUP_T = 120000000;
    static const count_t MEASURE_T = 300000000;
    static const count_t COOLDOWN_T = 5000000;

    std::vector<int64_t> randoms;
    std::vector<std::string> queries;

public:
    Benchmark() {}

    virtual ~Benchmark() {
        randoms.clear();
        queries.clear();
    }

    static time_t get_timestamp() {
        struct timeval now;
        gettimeofday (&now, NULL);

        return  now.tv_usec + (time_t)now.tv_sec * 1000000;
    }
};

#endif
