#include <iostream>
#include <fstream>
#include <unistd.h>

#include "../include/succinct/KVStoreShard.hpp"
#include "succinct/bench/SuccinctBenchmark.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [file]\n", exec);
}

int main(int argc, char **argv) {
    if(argc != 2) {
        print_usage(argv[0]);
        return -1;
    }

    std::string inputpath = std::string(argv[1]);

    // Benchmark core functions
    SuccinctBenchmark s_bench(inputpath);
    s_bench.benchmark_file();

    return 0;
}
