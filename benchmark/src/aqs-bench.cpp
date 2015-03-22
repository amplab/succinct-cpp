#include <iostream>
#include <fstream>
#include <unistd.h>

#include "AdaptiveQueryServerBenchmark.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-o output-path]\n", exec);
}

int main(int argc, char **argv) {
    if(argc > 5) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    std::string outpath = "benchmark/res";
    double skew = 1.0;  // Pure uniform
    while((c = getopt(argc, argv, "o:z:")) != -1) {
        switch(c) {
        case 'o':
            outpath = std::string(optarg);
            break;
        case 'z':
            skew = atof(optarg);
            break;
        default:
            outpath = "benchmark/res";
            skew = 1.0;
        }
    }

    // Benchmark core functions
    std::string reqpath = outpath + "/aqs-bench.req";
    std::string respath = outpath + "/aqs-bench.res";
    AdaptiveQueryServerBenchmark aqs_bench(reqpath, respath, skew);
    for(int32_t i = -1; i < 10; i++) {
        aqs_bench.measure_throughput(i);
    }

    return 0;
}
