#ifndef SHARD_BENCHMARK_H
#define SHARD_BENCHMARK_H

#include <cstdio>
#include <fstream>
#include <vector>

#include <sys/time.h>

#include "succinct/SuccinctShard.hpp"

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


class ShardBenchmark {

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

    void read_queries(std::string filename) {
        std::ifstream inputfile(filename);
        if(!inputfile.is_open()) {
            fprintf(stderr, "Error: Query file [%s] may be missing.\n",
                    filename.c_str());
            return;
        }

        std::string line, bin, query;
        while (getline(inputfile, line)) {
            // Extract key and value
            int split_index = line.find_first_of('\t');
            bin = line.substr(0, split_index);
            query = line.substr(split_index + 1);
            queries.push_back(query);
        }
        inputfile.close();
    }

public:

    ShardBenchmark(std::string filename, std::string queryfile = "") {
        std::ifstream input(filename);
        uint32_t num_keys = std::count(std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>(), '\n');
        shard = new SuccinctShard(0, filename, num_keys, false);
        generate_randoms();
        if(queryfile != "") {
            read_queries(queryfile);
        }
    }

    ShardBenchmark(SuccinctShard *shard, std::string queryfile = "") {
        this->shard = shard;
        generate_randoms();
        if(queryfile != "") {
            read_queries(queryfile);
        }
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
            res_stream << randoms[i] << "\t" << tdiff << "\n";
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

    void benchmark_access_latency(std::string res_path, int32_t len) {

        time_t t0, t1, tdiff;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
            std::string res;
            shard->access(res, randoms[i], 0, len);
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
            shard->access(res, randoms[i], 0, len);
            t1 = get_timestamp();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << tdiff << "\n";
            sum = (sum + res.length()) % shard->original_size();
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            std::string res;
            shard->access(res, randoms[i], 0, len);
            sum = (sum + res.length()) % shard->original_size();
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();

    }

    void benchmark_count_latency(std::string res_path) {

        time_t t0, t1, tdiff;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
           uint64_t res;
           res = shard->count(queries[i]);
           sum = (sum + res) % shard->original_size();
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
           uint64_t res;
           t0 = get_timestamp();
           res = shard->count(queries[i]);
           t1 = get_timestamp();
           tdiff = t1 - t0;
           res_stream << queries[i] << "\t" << tdiff << "\n";
           sum = (sum + res) % shard->original_size();
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
           uint64_t res;
           res = shard->count(queries[i]);
           sum = (sum + res) % shard->original_size();
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();

    }

    void benchmark_search_latency(std::string res_path) {
        time_t t0, t1, tdiff;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
            std::set<int64_t> res;
            shard->search(res, queries[i]);
            sum = (sum + res.size()) % shard->original_size();
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
            std::set<int64_t> res;
            t0 = get_timestamp();
            shard->search(res, queries[i]);
            t1 = get_timestamp();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << tdiff << "\n";
            sum = (sum + res.size()) % shard->original_size();
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            std::set<int64_t> res;
            shard->search(res, queries[i]);
            sum = (sum + res.size()) % shard->original_size();
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();
    }


    void benchmark_get_throughput() {
        double thput = 0;
        std::string value;
        try {
            // Warmup phase
            long i = 0;
            time_t warmup_start = get_timestamp();
            while (get_timestamp() - warmup_start < WARMUP_T) {
                shard->get(value, randoms[i % randoms.size()]);
                i++;
            }

            // Measure phase
            i = 0;
            time_t start = get_timestamp();
            while (get_timestamp() - start < MEASURE_T) {
                shard->get(value, randoms[i % randoms.size()]);
                i++;
            }
            time_t end = get_timestamp();
            double totsecs = (double) (end - start) / (1000.0 * 1000.0);
            thput = ((double) i / totsecs);

            // Cooldown phase
            i = 0;
            time_t cooldown_start = get_timestamp();
            while (get_timestamp() - cooldown_start < COOLDOWN_T) {
                shard->get(value, randoms[i % randoms.size()]);
                i++;
            }

        } catch (std::exception &e) {
            fprintf(stderr, "Throughput test ends...\n");
        }

        printf("Get throughput: %lf\n", thput);

        std::ofstream ofs;
        ofs.open("throughput_results_get",
                std::ofstream::out | std::ofstream::app);
        ofs << thput << "\n";
        ofs.close();
    }

    void benchmark_access_throughput(int32_t len) {
        double thput = 0;
        std::string value;
        try {
            // Warmup phase
            long i = 0;
            time_t warmup_start = get_timestamp();
            while (get_timestamp() - warmup_start < WARMUP_T) {
                shard->access(value, randoms[i % randoms.size()], 0, len);
                i++;
            }

            // Measure phase
            i = 0;
            time_t start = get_timestamp();
            while (get_timestamp() - start < MEASURE_T) {
                shard->access(value, randoms[i % randoms.size()], 0, len);
                i++;
            }
            time_t end = get_timestamp();
            double totsecs = (double) (end - start) / (1000.0 * 1000.0);
            thput = ((double) i / totsecs);

            // Cooldown phase
            i = 0;
            time_t cooldown_start = get_timestamp();
            while (get_timestamp() - cooldown_start < COOLDOWN_T) {
                shard->access(value, randoms[i % randoms.size()], 0, len);
                i++;
            }

        } catch (std::exception &e) {
            fprintf(stderr, "Throughput test ends...\n");
        }

        printf("Access throughput: %lf\n", thput);

        std::ofstream ofs;
        ofs.open("throughput_results_access",
                std::ofstream::out | std::ofstream::app);
        ofs << thput << "\n";
        ofs.close();
    }

    void benchmark_count_throughput() {
        double thput = 0;
        int64_t value;
        try {
            // Warmup phase
            long i = 0;
            time_t warmup_start = get_timestamp();
            while (get_timestamp() - warmup_start < WARMUP_T) {
                value = shard->count(queries[i % randoms.size()]);
                i++;
            }

            // Measure phase
            i = 0;
            time_t start = get_timestamp();
            while (get_timestamp() - start < MEASURE_T) {
                value = shard->count(queries[i % randoms.size()]);
                i++;
            }
            time_t end = get_timestamp();
            double totsecs = (double) (end - start) / (1000.0 * 1000.0);
            thput = ((double) i / totsecs);

            // Cooldown phase
            i = 0;
            time_t cooldown_start = get_timestamp();
            while (get_timestamp() - cooldown_start < COOLDOWN_T) {
                value = shard->count(queries[i % randoms.size()]);
                i++;
            }

        } catch (std::exception &e) {
            fprintf(stderr, "Throughput test ends...\n");
        }

        printf("Count throughput: %lf\n", thput);

        std::ofstream ofs;
        ofs.open("throughput_results_count",
                std::ofstream::out | std::ofstream::app);
        ofs << thput << "\n";
        ofs.close();
    }

    void benchmark_search_throughput() {
        double thput = 0;
        std::string value;
        try {
            // Warmup phase
            long i = 0;
            time_t warmup_start = get_timestamp();
            while (get_timestamp() - warmup_start < WARMUP_T) {
                shard->get(value, randoms[i % randoms.size()]);
                i++;
            }

            // Measure phase
            i = 0;
            time_t start = get_timestamp();
            while (get_timestamp() - start < MEASURE_T) {
                shard->get(value, randoms[i % randoms.size()]);
                i++;
            }
            time_t end = get_timestamp();
            double totsecs = (double) (end - start) / (1000.0 * 1000.0);
            thput = ((double) i / totsecs);

            // Cooldown phase
            i = 0;
            time_t cooldown_start = get_timestamp();
            while (get_timestamp() - cooldown_start < COOLDOWN_T) {
                shard->get(value, randoms[i % randoms.size()]);
                i++;
            }

        } catch (std::exception &e) {
            fprintf(stderr, "Throughput test ends...\n");
        }

        printf("Get throughput: %lf\n", thput);

        std::ofstream ofs;
        ofs.open("throughput_results_get",
                std::ofstream::out | std::ofstream::app);
        ofs << thput << "\n";
        ofs.close();
    }

private:
    std::vector<uint64_t> randoms;
    std::vector<std::string> queries;
    SuccinctShard *shard;
};

#endif
