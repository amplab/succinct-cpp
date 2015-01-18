#ifndef SUCCINCT_SERVER_BENCHMARK_H
#define SUCCINCT_SERVER_BENCHMARK_H

#include <cstdio>
#include <fstream>
#include <vector>

#include <sys/time.h>

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "thrift/SuccinctService.h"
#include "thrift/ports.h"

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

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class SuccinctServerBenchmark {

private:
    typedef unsigned long long int time_t;
    typedef unsigned long count_t;

    const count_t WARMUP_N = 10000;
    const count_t COOLDOWN_N = 10000;
    const count_t MEASURE_N = 100000;
    const count_t MAXSUM = 10000;

    static time_t get_timestamp() {
        struct timeval now;
        gettimeofday (&now, NULL);

        return  now.tv_usec + (time_t)now.tv_sec * 1000000;
    }

    void generate_randoms() {
        count_t q_cnt = WARMUP_N + COOLDOWN_N + MEASURE_N;
        int64_t MAX_KEYS = 1L << 32;

        fprintf(stderr, "Generating random keys...\n");
        uint64_t num_hosts = fd->get_num_hosts();

        for(count_t i = 0; i < q_cnt; i++) {
            // Pick a host
            uint64_t host_id = rand() % num_hosts;

            // Pick a shard
            uint64_t num_shards = fd->get_num_shards(host_id);
            uint64_t shard_id = host_id * num_shards  + rand() % num_shards;

            // Pick a key
            uint64_t num_keys = fd->get_num_keys(shard_id);
            uint64_t key = rand() % num_keys;

            randoms.push_back(shard_id * MAX_KEYS + key);
        }
        fprintf(stderr, "Generated %lu random keys\n", q_cnt);
    }

public:

    SuccinctServerBenchmark(std::string filename) {
        this->filename = filename;
        int port = QUERY_HANDLER_PORT;

        fprintf(stderr, "Connecting to server...\n");
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        this->fd = new SuccinctServiceClient(protocol);
        transport->open();
        fprintf(stderr, "Connected!\n");
        fd->connect_to_handlers();
        generate_randoms();
    }

    void benchmark_latency_get(std::string res_path) {

        time_t t0, t1, tdiff;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
            std::string res;
            fd->get(res, randoms[i]);
            sum = (sum + res.length()) % MAXSUM;
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
            std::string res;
            t0 = rdtsc();
            fd->get(res, randoms[i]);
            t1 = rdtsc();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << res << "\t" << tdiff << "\n";
            sum = (sum + res.length()) % MAXSUM;
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            std::string res;
            fd->get(res, randoms[i]);
            sum = (sum + res.length()) % MAXSUM;
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();

    }

    void benchmark_functions() {
        fprintf(stderr, "Benchmarking get...\n");
        benchmark_latency_get(filename + ".res_get");
        fprintf(stderr, "Done!\n\n");
    }

private:
    std::vector<uint64_t> randoms;
    std::string filename;
    SuccinctServiceClient *fd;
};

#endif
