#ifndef LAYERED_SUCCINCT_SHARD_BENCHMARK_HPP
#define LAYERED_SUCCINCT_SHARD_BENCHMARK_HPP

#include "Benchmark.hpp"
#include "ZipfGenerator.hpp"
#include "succinct/LayeredSuccinctShard.hpp"

#include <thread>
#include <sstream>
#include <unistd.h>

class LayeredSuccinctShardBenchmark : public Benchmark {
private:
    LayeredSuccinctShard *fd;
    std::vector<int32_t> lengths;
    std::string resfile;
    double skew_keys;
    double skew_lengths;

    void generate_randoms() {
        count_t q_cnt = fd->num_keys();
        fprintf(stderr, "Generating zipf distribution with theta=%f, N=%lu...\n", skew_keys, q_cnt);
        ZipfGenerator z(skew_keys, q_cnt);
        fprintf(stderr, "Generated zipf distribution, generating keys...\n");
        for(count_t i = 0; i < 100000; i++) {
            randoms.push_back(z.next());
        }
        fprintf(stderr, "Generated keys.\n");
    }

    void generate_lengths() {
        count_t q_cnt = fd->num_keys();
        int32_t min_len = 100;
        int32_t max_len = 500;
        fprintf(stderr, "Generating zipf distribution with theta=%f, N=%u...\n", skew_lengths, (max_len - min_len));
        ZipfGenerator z(skew_lengths, max_len - min_len);
        fprintf(stderr, "Generated zipf distribution, generating lengths...\n");

        for(count_t i = 0; i < 100000; i++) {
            // Map zipf value to a length
            int32_t len = z.next() + min_len;
            assert(len >= min_len);
            assert(len <= max_len);
            lengths.push_back(len);
        }
        fprintf(stderr, "Generated lengths.\n");
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
    LayeredSuccinctShardBenchmark(std::string filename, bool construct,
            uint32_t isa_sampling_rate, uint32_t sa_sampling_rate,
            std::string resfile, double skew, std::string queryfile = "") : Benchmark() {

        this->resfile = resfile;
        this->skew_keys = skew;
        this->skew_lengths = 1.0; // Pure uniform for now
        this->fd = new LayeredSuccinctShard(0, filename, construct, sa_sampling_rate,
                isa_sampling_rate);
        if(construct) {
            // Serialize and save to file
            fd->serialize();
        }

        generate_randoms();
        generate_lengths();

        if(queryfile != "") {
            read_queries(queryfile);
        }
    }

    void measure_access_throughput(uint32_t len) {
        fprintf(stderr, "Starting access throughput measurement...");
        size_t storage_size = fd->storage_size();
        std::string res;
        std::ofstream res_stream(resfile + ".access", std::ofstream::out | std::ofstream::app);
        uint64_t num_ops = 0;

        time_t start_time = get_timestamp();
        while(num_ops <= randoms.size()) {
            fd->access(res, randoms[num_ops % randoms.size()], 0, len);
            num_ops++;
        }
        time_t diff = get_timestamp() - start_time;
        double thput = ((double) num_ops * 1000 * 1000) / ((double)diff);
        res_stream << storage_size << "\t" << thput << "\n";
        res_stream.close();
        fprintf(stderr, "Done.\n");
    }

    void measure_get_throughput() {
        fprintf(stderr, "Starting get throughput measurement...");
        size_t storage_size = fd->storage_size();
        std::string res;
        std::ofstream res_stream(resfile + ".get", std::ofstream::out | std::ofstream::app);
        uint64_t num_ops = 0;

        time_t start_time = get_timestamp();
        while(num_ops <= randoms.size()) {
            fd->get(res, randoms[num_ops % randoms.size()]);
            num_ops++;
        }
        time_t diff = get_timestamp() - start_time;
        double thput = ((double) num_ops * 1000 * 1000) / ((double)diff);
        res_stream << storage_size << "\t" << thput << "\n";
        res_stream.close();
        fprintf(stderr, "Done.\n");
    }

    void measure_search_throughput() {
        fprintf(stderr, "Starting search throughput measurement...");
        size_t storage_size = fd->storage_size();
        std::set<int64_t> res;
        std::ofstream res_stream(resfile + ".search", std::ofstream::out | std::ofstream::app);
        uint64_t num_ops = 0;

        time_t start_time = get_timestamp();
        while(num_ops <= randoms.size()) {
            fd->search(res, queries[num_ops % queries.size()]);
            num_ops++;
        }
        time_t diff = get_timestamp() - start_time;
        double thput = ((double) num_ops * 1000 * 1000) / ((double)diff);
        res_stream << storage_size << "\t" << thput << "\n";
        res_stream.close();
        fprintf(stderr, "Done.\n");
    }

    void delete_layer(int32_t layer_id) {
        if(layer_id >= 0) {
            fprintf(stderr, "Deleting layer %d...\n", layer_id);
            fd->remove_layer(layer_id);
            fprintf(stderr, "Done.\n");
        }
    }
};

#endif
