#include <iostream>
#include <fstream>
#include <unistd.h>

#include "FileBenchmark.hpp"
#include "SuccinctFile.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [file]\n", exec);
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 8) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    uint32_t mode = 0;
    std::string querypath = "";
    std::string respath = "";
    while((c = getopt(argc, argv, "m:q:r:")) != -1) {
        switch(c) {
        case 'm':
            mode = atoi(optarg);
            break;
        case 'q':
        	querypath = std::string(optarg);
        	break;
        case 'r':
          respath = std::string(optarg);
          break;
        default:
            fprintf(stderr, "Invalid mode!\n");
            exit(0);
        }
    }

    if(respath == "") {
      respath = "res";
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string inputpath = std::string(argv[optind]);

    FileBenchmark *f_bench;
    if(mode == 0) {
        SuccinctFile *fd = new SuccinctFile(inputpath);

        // Serialize and save to file
        std::ofstream s_out(inputpath + ".succinct");
        fd->serialize();
        s_out.close();

        // Benchmark core and file functions
        f_bench = new FileBenchmark(fd);
    } else if(mode == 1) {
        SuccinctFile *fd = new SuccinctFile(inputpath, SuccinctMode::LOAD_IN_MEMORY);

        // Benchmark core and file functions
        f_bench = new FileBenchmark(fd);
    } else {
        // Only modes 0, 1 supported for now
        assert(0);
    }

    // HACK
    f_bench->benchmark_regex_breakdown(respath, querypath);

    return 0;
}
