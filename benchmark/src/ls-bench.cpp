#include <iostream>
#include <fstream>
#include <unistd.h>

#include "LayeredSuccinctShardBenchmark.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-c] [-z skew] [-i isa_sampling_rate] [-s sa_sampling_rate] [-o output-path] file\n", exec);
}

int main(int argc, char **argv) {
    if(argc > 11) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    std::string outpath = "benchmark/res";
    double skew = 1.0;  // Pure uniform
    uint32_t isa_sampling_rate = 32, sa_sampling_rate = 32;
    bool construct = false;
    while((c = getopt(argc, argv, "co:z:i:s:")) != -1) {
        switch(c) {
        case 'c':
            construct = true;
            break;
        case 'o':
            outpath = std::string(optarg);
            break;
        case 'z':
            skew = atof(optarg);
            break;
        case 'i':
            isa_sampling_rate = atoi(optarg);
            break;
        case 's':
            sa_sampling_rate = atoi(optarg);
            break;
        default:
            outpath = "benchmark/res";
            skew = 1.0;
            construct = false;
            isa_sampling_rate = 32;
            sa_sampling_rate = 32;
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string inputpath = std::string(argv[optind]);

    // Benchmark core functions
    std::string respath = outpath + "/ls-bench";
    LayeredSuccinctShardBenchmark ls_bench(inputpath, construct, isa_sampling_rate, sa_sampling_rate, respath, skew);
    for(int32_t i = -1; i < 10; i++) {
        ls_bench.delete_layer(i);
        ls_bench.measure_get_throughput();
        ls_bench.measure_access_throughput();
    }

    return 0;
}
