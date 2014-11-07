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
        for(count_t i = 0; i < q_cnt; i++) {
            randoms.push_back(rand() % MAXSUM);
        }
        fprintf(stderr, "Generated %lu random integers\n", q_cnt);
    }

    void read_queries(std::string queryfile) {
        std::ifstream inputfile(queryfile);
        if(!inputfile.is_open()) {
            fprintf(stderr, "Error: Query file [%s] may be missing.\n",
                    queryfile.c_str());
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
        fprintf(stderr, "Read %lu queries\n", queries.size());
        inputfile.close();
    }

public:

    SuccinctServerBenchmark(std::string filename, std::string queryfile = "") {
        this->filename = filename;
        int port = SUCCINCT_SERVER_PORT;
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        this->fd = new SuccinctServiceClient(protocol);
        transport->open();
        generate_randoms();
        if(queryfile != "") {
            read_queries(queryfile);
        }
    }

    void benchmark_count(std::string res_path) {

        time_t t0, t1, tdiff;
        uint64_t res;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", queries.size());
        for(uint64_t i = 0; i < queries.size(); i++) {
            t0 = get_timestamp();
            res = fd->count(queries[i]);
            t1 = get_timestamp();
            tdiff = t1 - t0;
            res_stream << res << "\t" << tdiff << "\n";
            sum = (sum + res) % MAXSUM;
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        res_stream.close();

    }

    void benchmark_search(std::string res_path) {

        time_t t0, t1, tdiff;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", queries.size());
        for(uint64_t i = 0; i < queries.size(); i++) {
            std::vector<int64_t> res;
            t0 = get_timestamp();
            fd->search(res, queries[i]);
            t1 = get_timestamp();
            tdiff = t1 - t0;
            res_stream << "\t" << res.size() << "\t" << tdiff << "\n";
            sum = (sum + res.size()) % MAXSUM;
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        res_stream.close();

    }

    void benchmark_extract(std::string res_path) {

        time_t t0, t1, tdiff;
        count_t sum;
        uint64_t extract_length = 1000;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
            std::string res;
            fd->extract(res, randoms[i], extract_length);
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
            fd->extract(res, randoms[i], extract_length);
            t1 = get_timestamp();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << res.length() << "\t" << tdiff << "\n";
            sum = (sum + res.length()) % MAXSUM;
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            std::string res;
            fd->extract(res, randoms[i], extract_length);
            sum = (sum + res.length()) % MAXSUM;
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();

    }

    void benchmark_functions() {
        fprintf(stderr, "Benchmarking File Functions...\n\n");
        fprintf(stderr, "Benchmarking extract...\n");
        benchmark_extract(filename + ".res_extract");
        fprintf(stderr, "Done!\n\n");
        if(queries.size() == 0) {
            fprintf(stderr, "[WARNING]: No queries have been loaded.\n");
            fprintf(stderr, "[WARNING]: Skipping count and search benchmarks.\n");
        } else {
            fprintf(stderr, "Benchmarking count...\n");
            benchmark_count(filename + ".res_count");
            fprintf(stderr, "Done!\n\n");
            fprintf(stderr, "Benchmarking search...\n");
            benchmark_search(filename + ".res_search");
            fprintf(stderr, "Done!\n\n");
        }
    }

private:
    std::vector<uint64_t> randoms;
    std::vector<std::string> queries;
    std::string filename;
    SuccinctServiceClient *fd;
};

#endif
