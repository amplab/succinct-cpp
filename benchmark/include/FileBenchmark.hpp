#ifndef FILE_BENCHMARK_HPP
#define FILE_BENCHMARK_HPP

#include "Benchmark.hpp"
#include "succinct/SuccinctFile.hpp"


class FileBenchmark : public Benchmark {

private:
    void generate_randoms() {
        count_t q_cnt = WARMUP_N + COOLDOWN_N + MEASURE_N;
        for(count_t i = 0; i < q_cnt; i++) {
            randoms.push_back(rand() % fd->original_size());
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

    FileBenchmark(SuccinctFile *fd, std::string queryfile = "") : Benchmark() {
        this->fd = fd;
        generate_randoms();
        if(queryfile != "") {
            read_queries(queryfile);
        }
    }

    void benchmark_idx_fn(uint64_t (SuccinctFile::*f)(uint64_t), std::string res_path) {

        time_t t0, t1, tdiff;
        uint64_t res;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
            res = (fd->*f)(randoms[i]);
            sum = (sum + res) % fd->original_size();
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
            t0 = rdtsc();
            res = (fd->*f)(randoms[i]);
            t1 = rdtsc();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << res << "\t" << tdiff << "\n";
            sum = (sum + res) % fd->original_size();
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            res = (fd->*f)(randoms[i]);
            sum = (sum + res) % fd->original_size();
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();

    }

    void benchmark_count(std::string res_path) {

        time_t t0, t1, tdiff;
        uint64_t res;
        count_t sum;
        std::ofstream res_stream(res_path);

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = 0; i < queries.size(); i++) {
            t0 = rdtsc();
            res = fd->count(queries[i]);
            t1 = rdtsc();
            tdiff = t1 - t0;
            res_stream << res << "\t" << tdiff << "\n";
            sum = (sum + res) % fd->original_size();
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
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = 0; i < queries.size(); i++) {
            std::vector<int64_t> res;
            t0 = rdtsc();
            fd->search(res, queries[i]);
            t1 = rdtsc();
            tdiff = t1 - t0;
            res_stream << res.size() << "\t" << tdiff << "\n";
            sum = (sum + res.size()) % fd->original_size();
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        res_stream.close();

    }

    void benchmark_extract(std::string res_path) {

        time_t t0, t1, tdiff;
        count_t sum;
        uint64_t extract_length = 64;
        std::ofstream res_stream(res_path);

        // Warmup
        sum = 0;
        fprintf(stderr, "Warming up for %lu queries...\n", WARMUP_N);
        for(uint64_t i = 0; i < WARMUP_N; i++) {
            std::string res;
            fd->extract(res, randoms[i], extract_length);
            sum = (sum + res.length()) % fd->original_size();
        }
        fprintf(stderr, "Warmup chksum = %lu\n", sum);
        fprintf(stderr, "Warmup complete.\n");

        // Measure
        sum = 0;
        fprintf(stderr, "Measuring for %lu queries...\n", MEASURE_N);
        for(uint64_t i = WARMUP_N; i < WARMUP_N + MEASURE_N; i++) {
            std::string res;
            t0 = rdtsc();
            fd->extract(res, randoms[i], extract_length);
            t1 = rdtsc();
            tdiff = t1 - t0;
            res_stream << randoms[i] << "\t" << res << "\t" << tdiff << "\n";
            sum = (sum + res.length()) % fd->original_size();
        }
        fprintf(stderr, "Measure chksum = %lu\n", sum);
        fprintf(stderr, "Measure complete.\n");

        // Cooldown
        sum = 0;
        fprintf(stderr, "Cooling down for %lu queries...\n", COOLDOWN_N);
        for(uint64_t i = WARMUP_N + MEASURE_N; i < randoms.size(); i++) {
            std::string res;
            fd->extract(res, randoms[i], extract_length);
            sum = (sum + res.length()) % fd->original_size();
        }
        fprintf(stderr, "Cooldown chksum = %lu\n", sum);
        fprintf(stderr, "Cooldown complete.\n");

        res_stream.close();

    }

    void benchmark_core() {
        fprintf(stderr, "Benchmarking Core Functions...\n\n");
        fprintf(stderr, "Benchmarking lookupNPA...\n");
        benchmark_idx_fn(&SuccinctFile::lookupNPA, fd->name() + ".res_npa");
        fprintf(stderr, "Done!\n\n");
        fprintf(stderr, "Benchmarking lookupSA...\n");
        benchmark_idx_fn(&SuccinctFile::lookupSA, fd->name() + ".res_sa");
        fprintf(stderr, "Done!\n\n");
        fprintf(stderr, "Benchmarking lookupISA...\n");
        benchmark_idx_fn(&SuccinctFile::lookupISA, fd->name() + ".res_isa");
        fprintf(stderr, "Done!\n\n");
    }

    void benchmark_file() {
        fprintf(stderr, "Benchmarking File Functions...\n\n");
        fprintf(stderr, "Benchmarking extract...\n");
        benchmark_extract(fd->name() + ".res_extract");
        fprintf(stderr, "Done!\n\n");
        if(queries.size() == 0) {
            fprintf(stderr, "[WARNING]: No queries have been loaded.\n");
            fprintf(stderr, "[WARNING]: Skipping count and search benchmarks.\n");
        } else {
            fprintf(stderr, "Benchmarking count...\n");
            benchmark_count(fd->name() + ".res_count");
            fprintf(stderr, "Done!\n\n");
            fprintf(stderr, "Benchmarking search...\n");
            benchmark_search(fd->name() + ".res_search");
            fprintf(stderr, "Done!\n\n");
        }
    }

private:
    std::vector<uint64_t> randoms;
    std::vector<std::string> queries;
    SuccinctFile *fd;
};

#endif
