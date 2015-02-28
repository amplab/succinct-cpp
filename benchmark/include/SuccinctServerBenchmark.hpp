#ifndef SUCCINCT_SERVER_BENCHMARK_H
#define SUCCINCT_SERVER_BENCHMARK_H

#include <cstdio>
#include <fstream>
#include <vector>

#include <sys/time.h>

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "succinct/SuccinctShard.hpp"
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

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class SuccinctServerBenchmark {

private:
    typedef unsigned long long int time_t;
    typedef unsigned long count_t;

    typedef struct {
        SuccinctServiceClient *client;
        boost::shared_ptr<TTransport> transport;
        std::vector<int64_t> randoms;
        int32_t len;
    } thread_data_t;

    static const count_t WARMUP_N = 10000;
    static const count_t COOLDOWN_N = 10000;
    static const count_t MEASURE_N = 100000;
    static const count_t MAXSUM = 10000;

    static const count_t WARMUP_T = 10000000;
    static const count_t MEASURE_T = 60000000;
    static const count_t COOLDOWN_T = 10000000;

    static time_t get_timestamp() {
        struct timeval now;
        gettimeofday(&now, NULL);

        return now.tv_usec + (time_t) now.tv_sec * 1000000;
    }

    void generate_randoms(uint32_t num_shards, uint32_t num_keys) {
        count_t q_cnt = WARMUP_N + COOLDOWN_N + MEASURE_N;

        fprintf(stderr, "Generating random keys...\n");

        for (count_t i = 0; i < q_cnt; i++) {
            // Pick a host
            int64_t shard_id = rand() % num_shards;
            int64_t key = rand() % num_keys;
            randoms.push_back(shard_id * SuccinctShard::MAX_KEYS + key);
        }
        fprintf(stderr, "Generated %lu random keys\n", q_cnt);
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

    SuccinctServerBenchmark(std::string bench_type, uint32_t num_shards, uint32_t num_keys, std::string queryfile = "") {
        this->benchmark_type = bench_type;
        int port = QUERY_HANDLER_PORT;

        if (bench_type == "latency") {
            fprintf(stderr, "Connecting to server...\n");
            boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
            boost::shared_ptr<TTransport> transport(
                    new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(
                    new TBinaryProtocol(transport));
            this->fd = new SuccinctServiceClient(protocol);
            transport->open();
            fprintf(stderr, "Connected!\n");
            fd->connect_to_handlers();
        } else {
            fd = NULL;
        }

        generate_randoms(num_shards, num_keys);
        if(queryfile != "") {
            read_queries(queryfile);
        }
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
            t0 = get_timestamp();
            fd->get(res, randoms[i]);
            t1 = get_timestamp();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << tdiff << "\n";
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

    void benchmark_access_latency(std::string res_path, int32_t len) {

        time_t t0, t1, tdiff;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
            std::string res;
            fd->access(res, randoms[i], 0, len);
            sum = (sum + res.length()) % MAXSUM;
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
            std::string res;
            t0 = get_timestamp();
            fd->access(res, randoms[i], 0, len);
            t1 = get_timestamp();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << tdiff << "\n";
            sum = (sum + res.length()) % MAXSUM;
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            std::string res;
            fd->access(res, randoms[i], 0, len);
            sum = (sum + res.length()) % MAXSUM;
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
           res = fd->count(queries[i]);
           sum = (sum + res) % MAXSUM;
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
           uint64_t res;
           t0 = get_timestamp();
           res = fd->count(queries[i]);
           t1 = get_timestamp();
           tdiff = t1 - t0;
           res_stream << queries[i] << "\t" << tdiff << "\n";
           sum = (sum + res) % MAXSUM;
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
           uint64_t res;
           res = fd->count(queries[i]);
           sum = (sum + res) % MAXSUM;
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
            fd->search(res, queries[i]);
            sum = (sum + res.size()) % MAXSUM;
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
            std::set<int64_t> res;
            t0 = get_timestamp();
            fd->search(res, queries[i]);
            t1 = get_timestamp();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << tdiff << "\n";
            sum = (sum + res.size()) % MAXSUM;
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            std::set<int64_t> res;
            fd->search(res, queries[i]);
            sum = (sum + res.size()) % MAXSUM;
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();
    }

    static void *getth(void *ptr) {
        thread_data_t data = *((thread_data_t*) ptr);
        std::cout << "GET\n";

        SuccinctServiceClient client = *(data.client);
        std::string value;

        double thput = 0;
        try {
            // Warmup phase
            long i = 0;
            time_t warmup_start = get_timestamp();
            while (get_timestamp() - warmup_start < WARMUP_T) {
                client.get(value, data.randoms[i % data.randoms.size()]);
                i++;
            }

            // Measure phase
            i = 0;
            time_t start = get_timestamp();
            while (get_timestamp() - start < MEASURE_T) {
                client.get(value, data.randoms[i % data.randoms.size()]);
                i++;
            }
            time_t end = get_timestamp();
            double totsecs = (double) (end - start) / (1000.0 * 1000.0);
            thput = ((double) i / totsecs);

            i = 0;
            time_t cooldown_start = get_timestamp();
            while (get_timestamp() - cooldown_start < COOLDOWN_T) {
                client.get(value, data.randoms[i % data.randoms.size()]);
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

        return 0;
    }

    int benchmark_throughput_get(uint32_t num_threads) {
        pthread_t thread[num_threads];
        std::vector<thread_data_t> data;
        fprintf(stderr, "Starting all threads...\n");
        for (uint32_t i = 0; i < num_threads; i++) {
            try {
                boost::shared_ptr<TSocket> socket(new TSocket("localhost", QUERY_HANDLER_PORT));
                boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
                boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
                SuccinctServiceClient *client = new SuccinctServiceClient(protocol);
                transport->open();
                client->connect_to_handlers();
                thread_data_t th_data;
                th_data.client = client;
                th_data.transport = transport;
                th_data.randoms = randoms;
                data.push_back(th_data);
            } catch (std::exception& e) {
                fprintf(stderr, "Could not connect to handler on localhost : %s\n", e.what());
                return -1;
            }
        }
        fprintf(stderr, "Started %lu clients.\n", data.size());

        for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
            int result = 0;
            result = pthread_create(&thread[current_t], NULL, SuccinctServerBenchmark::getth,
                    static_cast<void*>(&(data[current_t])));
            if (result != 0) {
                fprintf(stderr, "Error creating thread %d; return code = %d\n", current_t, result);
            }
        }

        for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
            pthread_join(thread[current_t], NULL);
        }
        fprintf(stderr, "All threads completed.\n");

        for (uint32_t i = 0; i < num_threads; i++) {
            data[i].transport->close();
        }

        data.clear();
        return 0;
    }

    static void *accessth(void *ptr) {
        thread_data_t data = *((thread_data_t*) ptr);
        std::cout << "GET\n";

        SuccinctServiceClient client = *(data.client);
        std::string value;

        double thput = 0;
        try {
            // Warmup phase
            long i = 0;
            time_t warmup_start = get_timestamp();
            while (get_timestamp() - warmup_start < WARMUP_T) {
                client.access(value, data.randoms[i % data.randoms.size()], 0, data.len);
                i++;
            }

            // Measure phase
            i = 0;
            time_t start = get_timestamp();
            while (get_timestamp() - start < MEASURE_T) {
                client.access(value, data.randoms[i % data.randoms.size()], 0, data.len);
                i++;
            }
            time_t end = get_timestamp();
            double totsecs = (double) (end - start) / (1000.0 * 1000.0);
            thput = ((double) i / totsecs);

            i = 0;
            time_t cooldown_start = get_timestamp();
            while (get_timestamp() - cooldown_start < COOLDOWN_T) {
                client.access(value, data.randoms[i % data.randoms.size()], 0,  data.len);
                i++;
            }

        } catch (std::exception &e) {
            fprintf(stderr, "Throughput test ends...\n");
        }

        printf("Get throughput: %lf\n", thput);

        std::ofstream ofs;
        ofs.open("throughput_results_access",
                std::ofstream::out | std::ofstream::app);
        ofs << thput << "\n";
        ofs.close();

        return 0;
    }

    int benchmark_throughput_access(uint32_t num_threads, int32_t len) {
        pthread_t thread[num_threads];
        std::vector<thread_data_t> data;
        fprintf(stderr, "Starting all threads...\n");
        for (uint32_t i = 0; i < num_threads; i++) {
            try {
                boost::shared_ptr<TSocket> socket(new TSocket("localhost", QUERY_HANDLER_PORT));
                boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
                boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
                SuccinctServiceClient *client = new SuccinctServiceClient(protocol);
                transport->open();
                client->connect_to_handlers();
                thread_data_t th_data;
                th_data.client = client;
                th_data.transport = transport;
                th_data.randoms = randoms;
                th_data.len = len;
                data.push_back(th_data);
            } catch (std::exception& e) {
                fprintf(stderr, "Could not connect to handler on localhost : %s\n", e.what());
                return -1;
            }
        }
        fprintf(stderr, "Started %lu clients.\n", data.size());

        for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
            int result = 0;
            result = pthread_create(&thread[current_t], NULL, SuccinctServerBenchmark::accessth,
                    static_cast<void*>(&(data[current_t])));
            if (result != 0) {
                fprintf(stderr, "Error creating thread %d; return code = %d\n", current_t, result);
            }
        }

        for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
            pthread_join(thread[current_t], NULL);
        }
        fprintf(stderr, "All threads completed.\n");

        for (uint32_t i = 0; i < num_threads; i++) {
            data[i].transport->close();
        }

        data.clear();
        return 0;
    }

private:
    std::vector<int64_t> randoms;
    std::vector<std::string> queries;
    std::string benchmark_type;
    SuccinctServiceClient *fd;
};

#endif
