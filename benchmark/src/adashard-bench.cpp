#include <iostream>
#include <fstream>
#include <unistd.h>

#include "../include/AdaptBenchmark.hpp"

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
    while((c = getopt(argc, argv, "c:o:")) != -1) {
        switch(c) {
        case 'c':
            configfile = std::string(optarg);
            break;
        case 'o':
            outpath = std::string(optarg);
            break;
        default:
            configfile = "benchmark/conf/adashard-bench.conf";
            outpath = "benchmark/res";
        }
    }

    // Benchmark core functions
    std::string reqpath = outpath + "/adashard-bench.req";
    std::string respath = outpath + "/adashard-bench.res";
    std::string addpath = outpath + "/adashard-bench.add";
    std::string delpath = outpath + "/adashard-bench.del";
    AdaptBenchmark d_bench(configfile, reqpath, respath, addpath, delpath);
    d_bench.run_benchmark();

    return 0;
}
