#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct/SuccinctShard.hpp"
#include "ShardBenchmark.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [-s sa_sampling_rate] [-i isa_sampling_rate] [-x sampling_scheme] [-d deleted-layers] [-n npa_sampling_rate] [-t type] [-l len] [file]\n", exec);
}

SamplingScheme scheme_from_opt(int opt) {
    switch(opt) {
    case 0:
        return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
    case 1:
        return SamplingScheme::FLAT_SAMPLE_BY_VALUE;
    case 2:
        return SamplingScheme::LAYERED_SAMPLE_BY_INDEX;
    default:
        return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
    }
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 18) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    std::vector<uint32_t> deleted_layers;
    uint32_t mode = 0;
    uint32_t sa_sampling_rate = 32;
    uint32_t isa_sampling_rate = 32;
    uint32_t npa_sampling_rate = 128;
    std::string type = "latency-get";
    int32_t len = 100;
    SamplingScheme scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX;

    while((c = getopt(argc, argv, "m:s:i:x:d:n:t:l:")) != -1) {
        switch(c) {
        case 'm':
        {
            mode = atoi(optarg);
            break;
        }
        case 's':
        {
            sa_sampling_rate = atoi(optarg);
            break;
        }
        case 'i':
        {
            isa_sampling_rate = atoi(optarg);
            break;
        }
        case 'x':
        {
            scheme = scheme_from_opt(atoi(optarg));
            break;
        }
        case 'd':
        {
            std::string del_list = std::string(optarg);
            size_t pos = 0;
            std::string token;
            std::string delimiter = ",";
            while ((pos = del_list.find(delimiter)) != std::string::npos) {
                token = del_list.substr(0, pos);
                deleted_layers.push_back(atoi(token.c_str()));
                del_list.erase(0, pos + delimiter.length());
            }
            deleted_layers.push_back(atoi(del_list.c_str()));
            break;
        }
        case 'n':
        {
            npa_sampling_rate = atoi(optarg);
            break;
        }
        case 't':
        {
            type = std::string(optarg);
            break;
        }
        case 'l':
        {
            len = atoi(optarg);
            break;
        }
        default:
        {
            mode = 0;
            sa_sampling_rate = 32;
            isa_sampling_rate = 32;
            npa_sampling_rate = 128;
            type = "latency-get";
            len = 100;
            scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX;
        }
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string inputpath = std::string(argv[optind]);

    SuccinctShard *fd;
    if(mode == 0) {
        fd = new SuccinctShard(0, inputpath, true, sa_sampling_rate, isa_sampling_rate, npa_sampling_rate, scheme, scheme);

        // Serialize and save to file
        std::ofstream s_out(inputpath + ".succinct");
        fd->serialize(s_out);
        s_out.close();
    } else if(mode == 1) {
        fd = new SuccinctShard(0, inputpath, false, sa_sampling_rate, isa_sampling_rate, npa_sampling_rate, scheme, scheme);
    } else {
        // Only modes 0, 1 supported for now
        assert(0);
    }

    if(scheme == SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {
        if(!deleted_layers.empty()) {
            LayeredSampledArray *SA = (LayeredSampledArray *)fd->getSA();
            LayeredSampledArray *ISA = (LayeredSampledArray *)fd->getISA();

            for(size_t i = 0; i < deleted_layers.size(); i++) {
                uint32_t layer_id = deleted_layers.at(i);
                SA->delete_layer(layer_id);
                ISA->delete_layer(layer_id);
            }
        }
    }

    ShardBenchmark s_bench(fd);
    if(type == "latency-sa") {
        s_bench.benchmark_idx_fn(&SuccinctShard::lookupSA, "latency_results_sa");
    } else if(type == "latency-isa") {
        s_bench.benchmark_idx_fn(&SuccinctShard::lookupISA, "latency_results_isa");
    } else if(type == "latency-npa") {
        s_bench.benchmark_idx_fn(&SuccinctShard::lookupNPA, "latency_results_npa");
    } else if(type == "latency-access") {
        s_bench.benchmark_access_latency("latency_results_access", len);
    } else if(type == "latency-get") {
        s_bench.benchmark_get_latency("latency_results_get");
    } else if(type == "throughput-access") {
        s_bench.benchmark_access_throughput(len);
    } else if(type == "throughput-get") {
        s_bench.benchmark_get_throughput();
    } else {
        // Not supported
        assert(0);
    }

    return 0;
}
