#ifndef SUCCINCT_BENCHMARK_H
#define SUCCINCT_BENCHMARK_H

#include <cstdio>
#include <fstream>
#include <vector>

#include <sys/time.h>

#include "../SuccinctShard.hpp"

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


class SuccinctBenchmark {

private:
    typedef unsigned long long int time_t;
    typedef unsigned long count_t;

    const count_t WARMUP_N = 1000;
    const count_t COOLDOWN_N = 1000;
    const count_t MEASURE_N = 10000;

    static const count_t WARMUP_T = 10000000;
    static const count_t MEASURE_T = 60000000;
    static const count_t COOLDOWN_T = 10000000;

    static time_t get_timestamp() {
		struct timeval now;
		gettimeofday (&now, NULL);

		return  now.tv_usec + (time_t)now.tv_sec * 1000000;
	}

    void generate_randoms() {
        count_t q_cnt = WARMUP_N + COOLDOWN_N + MEASURE_N;
        for(count_t i = 0; i < q_cnt; i++) {
            randoms.push_back(rand() % shard->num_keys());
        }
    }

public:

    SuccinctBenchmark(std::string filename) {
        std::ifstream input(filename);
        uint32_t num_keys = std::count(std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>(), '\n');
        shard = new SuccinctShard(0, filename, num_keys, false);
        generate_randoms();
    }

    SuccinctBenchmark(SuccinctShard *shard) {
        this->shard = shard;
        generate_randoms();
    }

    void benchmark_idx_fn(uint64_t (SuccinctShard::*f)(uint64_t), std::string res_path) {

        time_t t0, t1, tdiff;
        uint64_t res;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
            res = (shard->*f)(randoms[i]);
            sum = (sum + res) % shard->original_size();
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
            t0 = rdtsc();
            res = (shard->*f)(randoms[i]);
            t1 = rdtsc();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << res << "\t" << tdiff << "\n";
            sum = (sum + res) % shard->original_size();
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            res = (shard->*f)(randoms[i]);
            sum = (sum + res) % shard->original_size();
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();

    }

    void benchmark_get_latency(std::string res_path) {

        time_t t0, t1, tdiff;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
            std::string res;
            shard->get(res, randoms[i]);
            sum = (sum + res.length()) % shard->original_size();
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
            std::string res;
            t0 = get_timestamp();
            shard->get(res, randoms[i]);
            t1 = get_timestamp();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << res << "\t" << tdiff << "\n";
            sum = (sum + res.length()) % shard->original_size();
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            std::string res;
            shard->get(res, randoms[i]);
            sum = (sum + res.length()) % shard->original_size();
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();

    }

    void benchmark_access_throughput(int32_t len) {
        double thput = 0;
        std::string value;
        try {
            // Warmup phase
            long i = 0;
            time_t warmup_start = get_timestamp();
            while (get_timestamp() - warmup_start < WARMUP_T) {
                shard->access(value, randoms[i % randoms.size()], len);
                i++;
            }

            // Measure phase
            i = 0;
            time_t start = get_timestamp();
            while (get_timestamp() - start < MEASURE_T) {
                shard->access(value, randoms[i % randoms.size()], len);
                i++;
            }
            time_t end = get_timestamp();
            double totsecs = (double) (end - start) / (1000.0 * 1000.0);
            thput = ((double) i / totsecs);

            i = 0;
            time_t cooldown_start = get_timestamp();
            while (get_timestamp() - cooldown_start < COOLDOWN_T) {
                shard->access(value, randoms[i % randoms.size()], len);
                i++;
            }

        } catch (std::exception &e) {
            fprintf(stderr, "Throughput test ends...\n");
        }

        printf("Get throughput: %lf\n", thput);

        std::ofstream ofs;
        ofs.open("thput",
                std::ofstream::out | std::ofstream::app);
        ofs << thput << "\n";
        ofs.close();
    }

private:
    std::vector<uint64_t> randoms;
    SuccinctShard *shard;
};

#endif
