#include <iostream>
#include <fstream>
#include <unistd.h>

#include "../include/succinct/KVStoreShard.hpp"
#include "thrift/bench/SuccinctServerBenchmark.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-t num-threads] bench-type\n", exec);
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 8) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    uint32_t num_threads = 1;
    uint32_t num_shards = 1;
    uint32_t num_keys = 10000;
    while((c = getopt(argc, argv, "t:s:k:")) != -1) {
        switch(c) {
        case 't':
            num_threads = atoi(optarg);
            break;
        case 's':
            num_shards = atoi(optarg);
            break;
        case 'k':
            num_keys = atoi(optarg);
            break;
        default:
            num_threads = 1;
            num_shards = 1;
            num_keys = 10000;
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string benchmark_type = std::string(argv[optind]);

    // Benchmark core functions
    SuccinctServerBenchmark s_bench(benchmark_type, num_shards, num_keys);
    if(benchmark_type == "latency") {
        s_bench.benchmark_latency_get("latency_results_get");
    } else if(benchmark_type == "throughput") {
        s_bench.benchmark_throughput_get(num_threads);
    } else {
        // Not supported yet
        assert(0);
    }

    return 0;
}
