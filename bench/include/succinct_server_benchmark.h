#ifndef SUCCINCT_SERVER_BENCHMARK_H
#define SUCCINCT_SERVER_BENCHMARK_H

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include "zipf_generator.h"
#include "succinct_shard.h"
#include "benchmark.h"
#include "SuccinctService.h"
#include "ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class SuccinctServerBenchmark : public Benchmark {
 public:

  SuccinctServerBenchmark(std::string benchmark_type, uint32_t num_shards,
                          uint32_t num_keys, std::string query_file)
      : Benchmark() {
    benchmark_type_ = benchmark_type;
    int port = QUERY_HANDLER_PORT;
    client_ = NULL;

    GenerateRandoms(num_shards, num_keys);
    ReadQueries(query_file);
  }

  static void *GetThroughput(void *ptr) {
    ThreadData data = *((ThreadData*) ptr);
    std::cout << "GET\n";

    SuccinctServiceClient client = *(data.client);
    std::string value;

    double thput = 0;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        client.Get(value, data.keys[i % data.keys.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        client.Get(value, data.keys[i % data.keys.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        client.Get(value, data.keys[i % data.keys.size()]);
        i++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Get throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("throughput_results_get", std::ofstream::out | std::ofstream::app);
    ofs << thput << "\n";
    ofs.close();

    return 0;
  }

  static void *SearchThroughput(void *ptr) {
    ThreadData data = *((ThreadData*) ptr);
    std::cout << "GET\n";

    SuccinctServiceClient client = *(data.client);
    std::set<int64_t> res;

    double thput = 0;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        client.Search(res, data.keys[i % data.keys.size()], data.queries[i % data.queries.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        client.Search(res, data.keys[i % data.keys.size()], data.queries[i % data.queries.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        client.Search(res, data.keys[i % data.keys.size()], data.queries[i % data.queries.size()]);
        i++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Search throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("throughput_results_search", std::ofstream::out | std::ofstream::app);
    ofs << thput << "\n";
    ofs.close();

    return 0;
  }

  static void *SearchGetThroughput(void *ptr) {
    ThreadData data = *((ThreadData*) ptr);
    std::cout << "GET\n";

    SuccinctServiceClient client = *(data.client);
    std::set<int64_t> res;
    std::string value;

    double thput = 0;
    try {
      // Warmup phase
      uint64_t num_q = 0;
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        if (num_q % 2 == 0) {
          client.Search(res, data.keys[i % data.keys.size()], data.queries[i % data.queries.size()]);
        } else {
          client.Get(value, data.keys[i % data.keys.size()]);
        }
        i++; num_q++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        if (num_q % 2 == 0) {
          client.Search(res, data.keys[i % data.keys.size()], data.queries[i % data.queries.size()]);
        } else {
          client.Get(value, data.keys[i % data.keys.size()]);
        }
        i++; num_q++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        if (num_q % 2 == 0) {
          client.Search(res, data.keys[i % data.keys.size()], data.queries[i % data.queries.size()]);
        } else {
          client.Get(value, data.keys[i % data.keys.size()]);
        }
        i++; num_q++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Search throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("throughput_results_search", std::ofstream::out | std::ofstream::app);
    ofs << thput << "\n";
    ofs.close();

    return 0;
  }

  int BenchmarkGetThroughput(uint32_t num_threads) {
    pthread_t thread[num_threads];
    std::vector<ThreadData> data;
    fprintf(stderr, "Starting all threads...\n");
    for (uint32_t i = 0; i < num_threads; i++) {
      try {
        boost::shared_ptr<TSocket> socket(
            new TSocket("localhost", QUERY_HANDLER_PORT));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        SuccinctServiceClient *client = new SuccinctServiceClient(protocol);
        transport->open();
        client->ConnectToHandlers();
        ThreadData th_data;
        th_data.client = client;
        th_data.transport = transport;
        th_data.keys = keys_;
        data.push_back(th_data);
      } catch (std::exception& e) {
        fprintf(stderr, "Could not connect to handler on localhost : %s\n",
                e.what());
        return -1;
      }
    }
    fprintf(stderr, "Started %zu clients.\n", data.size());

    for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
      int result = 0;
      result = pthread_create(&thread[current_t], NULL,
                              SuccinctServerBenchmark::GetThroughput,
                              static_cast<void*>(&(data[current_t])));
      if (result != 0) {
        fprintf(stderr, "Error creating thread %d; return code = %d\n",
                current_t, result);
      }
    }

    for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
      pthread_join(thread[current_t], NULL);
    }
    fprintf(stderr, "All threads completed.\n");

    for (uint32_t i = 0; i < num_threads; i++) {
      data[i].transport->close();
    }

    data.clear();
    return 0;
  }

  int BenchmarkSearchThroughput(uint32_t num_threads) {
    pthread_t thread[num_threads];
    std::vector<ThreadData> data;
    fprintf(stderr, "Starting all threads...\n");
    for (uint32_t i = 0; i < num_threads; i++) {
      try {
        boost::shared_ptr<TSocket> socket(
            new TSocket("localhost", QUERY_HANDLER_PORT));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        SuccinctServiceClient *client = new SuccinctServiceClient(protocol);
        transport->open();
        client->ConnectToHandlers();
        ThreadData th_data;
        th_data.client = client;
        th_data.transport = transport;
        th_data.keys = keys_;
        th_data.queries = queries_;
        data.push_back(th_data);
      } catch (std::exception& e) {
        fprintf(stderr, "Could not connect to handler on localhost : %s\n",
                e.what());
        return -1;
      }
    }
    fprintf(stderr, "Started %zu clients.\n", data.size());

    for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
      int result = 0;
      result = pthread_create(&thread[current_t], NULL,
                              SuccinctServerBenchmark::SearchThroughput,
                              static_cast<void*>(&(data[current_t])));
      if (result != 0) {
        fprintf(stderr, "Error creating thread %d; return code = %d\n",
                current_t, result);
      }
    }

    for (uint32_t current_t = 0; current_t < num_threads; current_t++) {
      pthread_join(thread[current_t], NULL);
    }
    fprintf(stderr, "All threads completed.\n");

    for (uint32_t i = 0; i < num_threads; i++) {
      data[i].transport->close();
    }

    data.clear();
    return 0;
  }

 private:
  typedef struct {
    SuccinctServiceClient *client;
    boost::shared_ptr<TTransport> transport;
    std::vector<int32_t> shard_ids;
    std::vector<int64_t> keys;
    std::vector<std::string> queries;
  } ThreadData;

  static const uint64_t kMaxSum = 10000;

  unsigned long long llrand() {
    unsigned long long r = 0;

    for (int i = 0; i < 5; ++i) {
      r = (r << 15) | (rand() & 0x7FFF);
    }

    return r & 0xFFFFFFFFFFFFFFFFULL;
  }

  void GenerateRandoms(uint32_t num_shards, uint32_t num_keys,
                       std::vector<int32_t> &shard_ids, std::vector<int32_t> keys) {
    uint64_t query_count = kWarmupCount + kCooldownCount + kMeasureCount;

    fprintf(stderr, "Generating random keys, shard ids...\n");
    ZipfGenerator z_k(0.01, num_keys);
    ZipfGenerator z_k(0.01, num_shards);

    for (uint64_t i = 0; i < query_count; i++) {
      // Pick a host
      int64_t shard_id = rand() % num_shards;
      int64_t key = rand() % num_keys;
      keys_.push_back(shard_id * SuccinctShard::MAX_KEYS + key);
    }
    fprintf(stderr, "Generated %llu random keys\n", query_count);
  }

  void ReadQueries(std::string filename) {
    std::ifstream inputfile(filename);
    if (!inputfile.is_open()) {
      fprintf(stderr, "Error: Query file [%s] may be missing.\n",
              filename.c_str());
      return;
    }

    std::string line, query, query_count;
    while (getline(inputfile, line)) {
      // Extract key and value
      int split_index = line.find_first_of('\t');
      query = line.substr(0, split_index);
      query_count = line.substr(split_index + 1);
      queries_.push_back(query);
    }
    inputfile.close();
  }

  std::vector<int32_t> shard_ids_;
  std::vector<int64_t> keys_;
  std::vector<std::string> queries_;
  std::string benchmark_type_;
  SuccinctServiceClient *client_;
};

#endif
