#include <iostream>
#include <fstream>
#include <unistd.h>

#include "shard_benchmark.h"
#include "succinct_shard.h"
#include "npa/npa.h"

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-m mode] [-s sa_sampling_rate] [-i isa_sampling_rate] [-x sampling_scheme] [-d delete-layers] [-c create-layers] [-n npa_sampling_rate] [-r npa_encoding_scheme] [-t type] [-l len] [-q queryfile] [file]\n",
      exec);
}

SamplingScheme SamplingSchemeFromOption(int opt) {
  switch (opt) {
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

NPA::NPAEncodingScheme EncodingSchemeFromOption(int opt) {
  switch (opt) {
    case 0:
      return NPA::NPAEncodingScheme::ELIAS_DELTA_ENCODED;
    case 1:
      return NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
    case 2:
      return NPA::NPAEncodingScheme::WAVELET_TREE_ENCODED;
    default:
      return NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
  }
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 26) {
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
  NPA::NPAEncodingScheme npa_scheme =
      NPA::NPAEncodingScheme::ELIAS_GAMMA_ENCODED;
  std::string querypath = "";
  int32_t nthreads = 1;

  while ((c = getopt(argc, argv, "m:s:i:x:c:d:n:r:t:l:q:z:")) != -1) {
    switch (c) {
      case 'm': {
        mode = atoi(optarg);
        break;
      }
      case 's': {
        sa_sampling_rate = atoi(optarg);
        break;
      }
      case 'i': {
        isa_sampling_rate = atoi(optarg);
        break;
      }
      case 'x': {
        scheme = SamplingSchemeFromOption(atoi(optarg));
        break;
      }
      case 'c': {
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
      case 'd': {
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
      case 'n': {
        npa_sampling_rate = atoi(optarg);
        break;
      }
      case 'r': {
        npa_scheme = EncodingSchemeFromOption(atoi(optarg));
        break;
      }
      case 't': {
        type = std::string(optarg);
        break;
      }
      case 'l': {
        len = atoi(optarg);
        break;
      }
      case 'q': {
        querypath = std::string(optarg);
        break;
      }
      case 'z': {
        nthreads = atoi(optarg);
        break;
      }
      default: {
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

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string inputpath = std::string(argv[optind]);

  SuccinctShard *fd;
  if (mode == 0) {
    fprintf(stderr, "SuccinctMode = Construct in memory.\n");
    fd = new SuccinctShard(0, inputpath, SuccinctMode::CONSTRUCT_IN_MEMORY,
                           sa_sampling_rate, isa_sampling_rate,
                           npa_sampling_rate, scheme, scheme, npa_scheme);
    // Serialize
    fd->Serialize();
  } else if (mode == 1) {
    fprintf(stderr, "SuccinctMode = Load in memory.\n");
    fd = new SuccinctShard(0, inputpath, SuccinctMode::LOAD_IN_MEMORY,
                           sa_sampling_rate, isa_sampling_rate,
                           npa_sampling_rate, scheme, scheme, npa_scheme);
  } else if (mode == 2) {
    fprintf(stderr, "SuccinctMode = Load memory mapped.\n");
    fd = new SuccinctShard(0, inputpath, SuccinctMode::LOAD_MEMORY_MAPPED,
                           sa_sampling_rate, isa_sampling_rate,
                           npa_sampling_rate, scheme, scheme, npa_scheme);
  } else {
    // Only modes 0, 1, 3 supported for now
    assert(0);
  }

  fprintf(stderr, "shard size = %lu\n", fd->StorageSize());
  fprintf(stderr, "sampling scheme = %u\n", scheme);

  if (scheme == SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {
    fprintf(stderr, "Scheme = LayeredSampledByIndex\n");
    if (!deleted_layers.empty()) {
      LayeredSampledArray *SA = (LayeredSampledArray *) fd->GetSA();
      LayeredSampledArray *ISA = (LayeredSampledArray *) fd->GetISA();

      size_t deleted_size = 0;
      for (size_t i = 0; i < deleted_layers.size(); i++) {
        uint32_t layer_id = deleted_layers.at(i);
        deleted_size += SA->DestroyLayer(layer_id);
        deleted_size += ISA->DestroyLayer(layer_id);
      }

      fprintf(stderr, "Deleted data size = %lu\n", deleted_size / 8);
    }
  } else if (scheme == SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX) {
    fprintf(stderr, "Scheme = OpportunisticLayeredSampledByIndex\n");
    if (!deleted_layers.empty()) {
      OpportunisticLayeredSampledArray *SA =
          (OpportunisticLayeredSampledArray *) fd->GetSA();
      OpportunisticLayeredSampledArray *ISA =
          (OpportunisticLayeredSampledArray *) fd->GetISA();

      size_t deleted_size = 0;
      for (size_t i = 0; i < deleted_layers.size(); i++) {
        uint32_t layer_id = deleted_layers.at(i);
        deleted_size += SA->DestroyLayer(layer_id);
        deleted_size += ISA->DestroyLayer(layer_id);
      }
      fprintf(stderr, "Deleted data size = %lu\n", deleted_size / 8);
    }
  }

  ShardBenchmark s_bench(fd, querypath);
  if (type == "latency-sa") {
    s_bench.BenchmarkLookupFunction(&SuccinctShard::LookupSA,
                                    "latency_results_sa");
  } else if (type == "latency-isa") {
    s_bench.BenchmarkLookupFunction(&SuccinctShard::LookupISA,
                                    "latency_results_isa");
  } else if (type == "latency-npa") {
    s_bench.BenchmarkLookupFunction(&SuccinctShard::LookupNPA,
                                    "latency_results_npa");
  } else if (type == "latency-access") {
    s_bench.BenchmarkAccessLatency("latency_results_access", len);
  } else if (type == "latency-get") {
    s_bench.BenchmarkGetLatency("latency_results_get");
  } else if (type == "latency-count") {
    s_bench.BenchmarkCountLatency("latency_results_count");
  } else if (type == "latency-search") {
    s_bench.BenchmarkSearchLatency("latency_results_search");
  } else if (type == "throughput-get") {
    s_bench.BenchmarkGetThroughput(nthreads);
  } else if (type == "throughput-search") {
    s_bench.BenchmarkSearchThroughput(nthreads);
  } else if (type == "reconstruct-sa") {
    if (scheme != SamplingScheme::LAYERED_SAMPLE_BY_INDEX
        && scheme != SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX) {
      fprintf(stderr, "Invalid sampling scheme for reconstruction.\n");
      return 0;
    }
    LayeredSampledArray *SA = (LayeredSampledArray *) fd->GetSA();
    for (uint32_t i = 0; i < created_layers.size(); i++) {
      uint64_t start_time = Benchmark::GetTimestamp();
      if (scheme == SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {
        ((LayeredSampledSA *) SA)->ReconstructLayer(created_layers.at(i));
      } else {
        ((OpportunisticLayeredSampledSA *) SA)->ReconstructLayer(
            created_layers.at(i));
      }
      uint64_t end_time = Benchmark::GetTimestamp();
      fprintf(stderr, "Time to reconstruct layer %u = %llu\n",
              created_layers.at(i), end_time - start_time);
    }
  } else if (type == "reconstruct-isa") {
    if (scheme != SamplingScheme::LAYERED_SAMPLE_BY_INDEX
        && scheme != SamplingScheme::OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX) {
      fprintf(stderr, "Invalid sampling scheme for reconstruction.\n");
      return 0;
    }
    LayeredSampledArray *ISA = (LayeredSampledArray *) fd->GetISA();
    for (uint32_t i = 0; i < created_layers.size(); i++) {
      uint64_t start_time = Benchmark::GetTimestamp();
      if (scheme == SamplingScheme::LAYERED_SAMPLE_BY_INDEX) {
        ((LayeredSampledISA *) ISA)->ReconstructLayer(created_layers.at(i));
      } else {
        ((OpportunisticLayeredSampledISA *) ISA)->ReconstructLayer(
            created_layers.at(i));
      }
      uint64_t end_time = Benchmark::GetTimestamp();
      fprintf(stderr, "Time to reconstruct layer %u = %llu\n",
              created_layers.at(i), end_time - start_time);
    }
  } else if (type == "storage-breakdown") {
    fd->PrintStorageBreakdown();
  } else {
    assert(0);
  }

  return 0;
}
