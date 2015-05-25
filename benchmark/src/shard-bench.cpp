#include <iostream>
#include <fstream>
#include <unistd.h>

#include "succinct/SuccinctShard.hpp"
#include "ShardBenchmark.hpp"

#include "thrift/DynamicLoadBalancer.hpp"

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [-s sa_sampling_rate] [-i isa_sampling_rate] [-x sampling_scheme] [-d delete-layers] [-c create-layers] [-n npa_sampling_rate] [-t type] [-l len] [-q queryfile] [file]\n", exec);
}

SamplingScheme scheme_from_opt(int opt) {
    switch(opt) {
    case 0:
        return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
    case 1:
        return SamplingScheme::FLAT_SAMPLE_BY_VALUE;
    case 2:
        return SamplingScheme::LAYERED_SAMPLE_BY_INDEX;
    case 3:
        return SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX;
    default:
        return SamplingScheme::FLAT_SAMPLE_BY_INDEX;
    }
}

int main(int argc, char **argv) {
    if(argc < 2 || argc > 22) {
        print_usage(argv[0]);
        return -1;
    }

    int c;
    std::vector<uint32_t> deleted_layers;
    std::vector<uint32_t> created_layers;
    uint32_t mode = 0;
    uint32_t sa_sampling_rate = 32;
    uint32_t isa_sampling_rate = 32;
    uint32_t npa_sampling_rate = 128;
    std::string type = "latency-get";
    int32_t len = 100;
    SamplingScheme scheme = SamplingScheme::FLAT_SAMPLE_BY_INDEX;
    std::string querypath = "";

    while((c = getopt(argc, argv, "m:s:i:x:c:d:n:t:l:q:")) != -1) {
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
        case 'c':
        {
            std::string add_list = std::string(optarg);
            size_t pos = 0;
            std::string token;
            std::string delimiter = ",";
            while ((pos = add_list.find(delimiter)) != std::string::npos) {
                token = add_list.substr(0, pos);
                created_layers.push_back(atoi(token.c_str()));
                add_list.erase(0, pos + delimiter.length());
            }
            created_layers.push_back(atoi(add_list.c_str()));
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
        case 'q':
        {
            querypath = std::string(optarg);
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
            querypath = "";
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
        fprintf(stderr, "SuccinctMode = Construct in memory.\n");

        // sanity check as construction process is very expensive
        struct stat st;
        std::string succinct_path = inputpath + ".succinct";
        if (stat(succinct_path.c_str(), &st) != 0)
            if (mkdir(succinct_path.c_str(), (mode_t)(S_IRWXU | S_IRGRP |  S_IXGRP | S_IROTH | S_IXOTH)) != 0) {
                fprintf(stderr, "cannot mkdir directory '%s', terminating\n", succinct_path.c_str());
                return 1;
            }

        fd = new SuccinctShard(0, inputpath, SuccinctMode::CONSTRUCT_IN_MEMORY, sa_sampling_rate, isa_sampling_rate, npa_sampling_rate, scheme, scheme);
        // Serialize
        fd->serialize();
    } else if(mode == 1) {
        fprintf(stderr, "SuccinctMode = Load in memory.\n");
        fd = new SuccinctShard(0, inputpath, SuccinctMode::LOAD_IN_MEMORY, sa_sampling_rate, isa_sampling_rate, npa_sampling_rate, scheme, scheme);
    } else if(mode == 2) {
        fprintf(stderr, "SuccinctMode = Load memory mapped.\n");
        fd = new SuccinctShard(0, inputpath, SuccinctMode::LOAD_MEMORY_MAPPED, sa_sampling_rate, isa_sampling_rate, npa_sampling_rate, scheme, scheme);
    } else {
        // Only modes 0, 1, 3 supported for now
        assert(0);
    }

    fprintf(stderr, "shard size = %lu\n", fd->storage_size());
    fprintf(stderr, "sampling scheme = %u\n", scheme);

    if(scheme == SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {
        fprintf(stderr, "Scheme = LayeredSampledByIndex\n");
        if(!deleted_layers.empty()) {
            LayeredSampledArray *SA = (LayeredSampledArray *)fd->getSA();
            LayeredSampledArray *ISA = (LayeredSampledArray *)fd->getISA();

            size_t deleted_size = 0;
            for(size_t i = 0; i < deleted_layers.size(); i++) {
                uint32_t layer_id = deleted_layers.at(i);
                deleted_size += SA->delete_layer(layer_id);
                deleted_size += ISA->delete_layer(layer_id);
            }

            fprintf(stderr, "Deleted data size = %lu\n", deleted_size / 8);
        }
    } else if(scheme == SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX) {
        fprintf(stderr, "Scheme = OpportunisticLayeredSampledByIndex\n");
        if(!deleted_layers.empty()) {
            OpportunisticLayeredSampledArray *SA = (OpportunisticLayeredSampledArray *)fd->getSA();
            OpportunisticLayeredSampledArray *ISA = (OpportunisticLayeredSampledArray *)fd->getISA();

            size_t deleted_size = 0;
            for(size_t i = 0; i < deleted_layers.size(); i++) {
                uint32_t layer_id = deleted_layers.at(i);
                deleted_size += SA->delete_layer(layer_id);
                deleted_size += ISA->delete_layer(layer_id);
            }
            fprintf(stderr, "Deleted data size = %lu\n", deleted_size / 8);
        }
    }

    ShardBenchmark s_bench(fd, querypath);
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
    } else if(type == "latency-count") {
        s_bench.benchmark_count_latency("latency_results_count");
    } else if(type == "latency-search") {
        s_bench.benchmark_search_latency("latency_results_search");
    } else if(type == "throughput-access") {
        s_bench.benchmark_access_throughput(len);
    } else if(type == "throughput-get") {
        s_bench.benchmark_get_throughput();
    } else if(type == "throughput-count") {
        s_bench.benchmark_count_throughput();
    } else if(type == "throughput-search") {
        s_bench.benchmark_search_throughput();
    } else if(type == "reconstruct-sa") {
        if(scheme != SamplingScheme::LAYERED_SAMPLE_BY_INDEX &&
                scheme != SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX) {
            fprintf(stderr, "Invalid sampling scheme for reconstruction.\n");
            return 0;
        }
        LayeredSampledArray *SA = (LayeredSampledArray *)fd->getSA();
        for(uint32_t i = 0; i < created_layers.size(); i++) {
            uint64_t start_time = Benchmark::get_timestamp();
            if(scheme == SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {
                ((LayeredSampledSA *)SA)->reconstruct_layer(created_layers.at(i));
            } else {
                ((OpportunisticLayeredSampledSA *)SA)->reconstruct_layer(created_layers.at(i));
            }
            uint64_t end_time = Benchmark::get_timestamp();
            fprintf(stderr, "Time to reconstruct layer %u = %llu\n",
                    created_layers.at(i), end_time - start_time);
        }
    } else if(type == "reconstruct-isa") {
        if(scheme != SamplingScheme::LAYERED_SAMPLE_BY_INDEX &&
                scheme != SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX) {
            fprintf(stderr, "Invalid sampling scheme for reconstruction.\n");
            return 0;
        }
        LayeredSampledArray *ISA = (LayeredSampledArray *)fd->getISA();
        for(uint32_t i = 0; i < created_layers.size(); i++) {
            uint64_t start_time = Benchmark::get_timestamp();
            if(scheme == SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {
                ((LayeredSampledISA *)ISA)->reconstruct_layer(created_layers.at(i));
            } else {
                ((OpportunisticLayeredSampledISA *)ISA)->reconstruct_layer(created_layers.at(i));
            }
            uint64_t end_time = Benchmark::get_timestamp();
            fprintf(stderr, "Time to reconstruct layer %u = %llu\n",
                    created_layers.at(i), end_time - start_time);
        }
    } else if(type == "storage-breakdown") {
        fd->print_storage_breakdown();
    } else{
        assert(0);
    }

    return 0;
}
