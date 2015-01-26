#include <iostream>
#include <fstream>
#include <unistd.h>

#include "../include/succinct/KVStoreShard.hpp"
#include "succinct/bench/SuccinctBenchmark.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-t type] [-l len] [file]\n", exec);
}

int main(int argc, char **argv) {
    if(argc != 6) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    std::string type = "latency-get";
    int32_t len = 100;
    while((c = getopt(argc, argv, "t:l:")) != -1) {
        switch(c) {
        case 't':
            type = std::string(optarg);
            break;
        case 'l':
            len = atoi(optarg);
            break;
        default:
            type = "latency-get";
            len = 100;
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string inputpath = std::string(argv[optind]);

    // Benchmark core functions
    SuccinctBenchmark s_bench(inputpath);
    if(type == "latency-get") {
        s_bench.benchmark_get_latency();
    } else if(type == "latency-access") {
        s_bench.benchmark_access_latency(len);
    } else if(type == "throughput-access") {
        s_bench.benchmark_access_throughput(len);
    } else {
        // Not supported
        assert(0);
    }

    return 0;
}
