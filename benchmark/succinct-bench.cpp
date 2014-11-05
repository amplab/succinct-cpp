#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct/bench/SuccinctBenchmark.hpp"
#include "succinct/SuccinctFile.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [file]\n", exec);
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 4) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    uint32_t mode = 0;
    while((c = getopt(argc, argv, "m:")) != -1) {
        switch(c) {
        case 'm':
            mode = atoi(optarg);
            break;
        default:
            mode = 0;
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string inputpath = std::string(argv[optind]);

    if(mode == 0) {
        SuccinctFile fd(inputpath);

        // Serialize and save to file
        std::ofstream s_out(inputpath + ".succinct");
        fd.serialize(s_out);
        s_out.close();

        // Benchmark core functions
        SuccinctBenchmark s_bench(&fd);
        s_bench.benchmark_core();
    } else if(mode == 1) {
        // Benchmark core functions
        SuccinctBenchmark s_bench(inputpath);
        s_bench.benchmark_core();
    } else {
        // Only modes 0, 1 supported for now
        assert(0);
    }

    return 0;
}
