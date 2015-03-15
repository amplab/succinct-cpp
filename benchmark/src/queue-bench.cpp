#include <iostream>
#include <fstream>
#include <unistd.h>

#include "DynamicAdaptBenchmark.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-c test-config-file] [-o output-path]\n", exec);
}

int main(int argc, char **argv) {
    if(argc > 5) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    std::string configfile = "benchmark/conf/adashard-bench.conf";
    std::string outpath = "benchmark/res";
    double skew = 1.0;  // Pure uniform
    while((c = getopt(argc, argv, "c:o:z:")) != -1) {
        switch(c) {
        case 'c':
            configfile = std::string(optarg);
            break;
        case 'o':
            outpath = std::string(optarg);
            break;
        case 'z':
            skew = atof(optarg);
            break;
        default:
            configfile = "benchmark/conf/adashard-bench.conf";
            outpath = "benchmark/res";
            skew = 1.0;
        }
    }

    // Benchmark core functions
    std::string reqpath = outpath + "/adashard-bench.req";
    std::string respath = outpath + "/adashard-bench.res";
    std::string addpath = outpath + "/adashard-bench.add";
    std::string delpath = outpath + "/adashard-bench.del";
    DynamicAdaptBenchmark q_bench(configfile, reqpath, respath, addpath, delpath, skew);
    q_bench.run_benchmark();

    return 0;
}
