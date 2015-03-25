#include <iostream>
#include <fstream>
#include <unistd.h>

#include "DynamicLoadBalancerBenchmark.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-c test-config-file] [-o output-path]\n", exec);
}

int main(int argc, char **argv) {
    if(argc > 13) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    std::string configfile = "benchmark/conf/adashard-bench.conf";
    std::string hostsfile = "conf/hosts";
    std::string outpath = "benchmark/res";
    double skew = 1.0;  // Pure uniform
    int32_t num_active_replicas = 1;
    std::string querypath = "";
    while((c = getopt(argc, argv, "c:h:o:z:n:q:")) != -1) {
        switch(c) {
        case 'c':
            configfile = std::string(optarg);
            break;
        case 'h':
            hostsfile = std::string(optarg);
            break;
        case 'o':
            outpath = std::string(optarg);
            break;
        case 'z':
            skew = atof(optarg);
            break;
        case 'n':
            num_active_replicas = atoi(optarg);
            break;
        case 'q':
            querypath = std::string(optarg);
            break;
        default:
            configfile = "benchmark/conf/adashard-bench.conf";
            outpath = "benchmark/res";
            skew = 1.0;
            querypath = "";
        }
    }

    std::ifstream hosts(hostsfile);
    std::string host;
    std::vector<std::string> hostnames;
    while(std::getline(hosts, host, '\n')) {
        hostnames.push_back(host);
    }

    // Benchmark core functions
    std::string reqpath = outpath + "/dlb-bench.req";
    std::string respath = outpath + "/dlb-bench.res";
    std::string addpath = outpath + "/dlb-bench.add";
    std::string delpath = outpath + "/dlb-bench.del";
    DynamicLoadBalancerBenchmark dlb_bench(configfile, reqpath, respath, addpath, delpath, skew, hostnames, num_active_replicas, querypath);
    dlb_bench.run_benchmark();

    return 0;
}
