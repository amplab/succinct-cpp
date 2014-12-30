#include <iostream>
#include <fstream>
#include <unistd.h>

#include "../include/succinct/SuccinctShard.hpp"
#include "thrift/bench/SuccinctServerBenchmark.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [inputfile] [queryfile]\n", exec);
}

int main(int argc, char **argv) {
    if(argc != 3) {
        print_usage(argv[0]);
        return -1;
    }

    std::string inputpath = std::string(argv[1]);
    std::string querypath = std::string(argv[2]);

    // Benchmark core functions
    SuccinctServerBenchmark s_bench(inputpath, querypath);
    s_bench.benchmark_functions();

    return 0;
}
