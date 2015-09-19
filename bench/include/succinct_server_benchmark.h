#ifndef SUCCINCT_SERVER_BENCHMARK_H
#define SUCCINCT_SERVER_BENCHMARK_H

#include <sstream>

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

  SuccinctServerBenchmark(std::string benchmark_type, double skew,
                          uint32_t num_keys, std::string query_file,
                          std::vector<int32_t>& primary_ids)
      : Benchmark() {
    benchmark_type_ = benchmark_type;
    client_ = NULL;
    num_keys_ = num_keys;
    query_file_ = query_file;
    skew_ = skew;
    primary_ids_ = primary_ids;
  }

  static void *GetThroughput(void *ptr) {
    ThreadData data = *((ThreadData*) ptr);
    std::cout << "GET\n";

    SuccinctServiceClient client = *(data.client);

    double thput = 0;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        std::string value;
        client.Get(value, data.shard_ids[i % data.shard_ids.size()],
                   data.randoms[i % data.randoms.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        std::string value;
        client.Get(value, data.shard_ids[i % data.shard_ids.size()],
                   data.randoms[i % data.randoms.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        std::string value;
        client.Get(value, data.shard_ids[i % data.shard_ids.size()],
                   data.randoms[i % data.randoms.size()]);
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
    std::cout << "SEARCH\n";

    SuccinctServiceClient client = *(data.client);

    double thput = 0;
    try {
      // Warmup phase
      long i = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        std::set<int64_t> res;
        client.Search(res, data.shard_ids[i % data.shard_ids.size()],
                      data.queries[i % data.queries.size()]);
        i++;
      }

      // Measure phase
      i = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        std::set<int64_t> res;
        client.Search(res, data.shard_ids[i % data.shard_ids.size()],
                      data.queries[i % data.queries.size()]);
        i++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        std::set<int64_t> res;
        client.Search(res, data.shard_ids[i % data.shard_ids.size()],
                      data.queries[i % data.queries.size()]);
        i++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Search throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("throughput_results_search",
             std::ofstream::out | std::ofstream::app);
    ofs << thput << "\n";
    ofs.close();

    return 0;
  }

  static void *SearchGetThroughput(void *ptr) {
    ThreadData data = *((ThreadData*) ptr);
    std::cout << "SEARCH_GET\n";

    SuccinctServiceClient client = *(data.client);

    double thput = 0;
    try {
      // Warmup phase
      uint64_t num_q = 0;
      uint64_t i = 0, j = 0, k = 0;
      TimeStamp warmup_start = GetTimestamp();
      while (GetTimestamp() - warmup_start < kWarmupTime) {
        std::set<int64_t> res;
        std::string value;
        if (num_q % 2 == 0) {
          client.Search(res, data.shard_ids[k % data.shard_ids.size()],
                        data.queries[j % data.queries.size()]);
          j++;
        } else {
          client.Get(value, data.shard_ids[k % data.shard_ids.size()],
                     data.randoms[k % data.randoms.size()]);
          k++;
        }
        i++;
        num_q++;
      }

      // Measure phase
      i = 0, j = 0, k = 0;
      TimeStamp start = GetTimestamp();
      while (GetTimestamp() - start < kMeasureTime) {
        std::set<int64_t> res;
        std::string value;
        if (num_q % 2 == 0) {
          client.Search(res, data.shard_ids[k % data.shard_ids.size()],
                        data.queries[j % data.queries.size()]);
          j++;
        } else {
          client.Get(value, data.shard_ids[k % data.shard_ids.size()],
                     data.randoms[k % data.randoms.size()]);
          k++;
        }
        i++;
        num_q++;
      }
      TimeStamp end = GetTimestamp();
      double totsecs = (double) (end - start) / (1000.0 * 1000.0);
      thput = ((double) i / totsecs);

      i = 0, j = 0, k = 0;
      TimeStamp cooldown_start = GetTimestamp();
      while (GetTimestamp() - cooldown_start < kCooldownTime) {
        std::set<int64_t> res;
        std::string value;
        if (num_q % 2 == 0) {
          client.Search(res, data.shard_ids[k % data.shard_ids.size()],
                        data.queries[j % data.queries.size()]);
          j++;
        } else {
          client.Get(value, data.shard_ids[k % data.shard_ids.size()],
                     data.randoms[k % data.randoms.size()]);
          k++;
        }
        i++;
        num_q++;
      }

    } catch (std::exception &e) {
      fprintf(stderr, "Throughput test ends...\n");
    }

    printf("Search+Get throughput: %lf\n", thput);

    std::ofstream ofs;
    ofs.open("throughput_results_search_get",
             std::ofstream::out | std::ofstream::app);
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
        GenerateRandoms(num_keys_, th_data.shard_ids, th_data.randoms);
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
        GenerateRandoms(num_keys_, th_data.shard_ids, th_data.randoms);
        ReadQueries(query_file_, th_data.queries);
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

  int BenchmarkSearchGetThroughput(uint32_t num_threads) {
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
        GenerateRandoms(num_keys_, th_data.shard_ids, th_data.randoms);
        ReadQueries(query_file_, th_data.queries);
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
                              SuccinctServerBenchmark::SearchGetThroughput,
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
    std::vector<int64_t> randoms;
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

  void GenerateRandoms(uint32_t num_keys, std::vector<int32_t> &shard_ids,
                       std::vector<int64_t> &randoms) {
    uint64_t query_count = 1000000;

    fprintf(stderr, "Generating random keys, shard ids...\n");
    fprintf(stderr, "Num Primaries = %zu, Skew = %lf\n", primary_ids_.size(),
            skew_);
    ZipfGenerator z_s(skew_, primary_ids_.size());
    fprintf(stderr, "Num Keys = %u, Skew = %lf\n", num_keys, skew_);
    ZipfGenerator z_k(skew_, num_keys);

    std::map<int32_t, uint64_t> shard_skew;
    for (uint64_t i = 0; i < query_count; i++) {
      shard_ids.push_back(primary_ids_.at(z_s.Next()));
      shard_skew[primary_ids_.at(z_s.Next())]++;
      randoms.push_back(z_k.Next());
    }

    fprintf(stderr, "Shard skew:\n");
    for (auto s_skew : shard_skew) {
      fprintf(stderr, "%d => %llu\n", s_skew.first, s_skew.second);
    }

    fprintf(stderr, "Loaded %llu get queries!\n", query_count);
  }

  void ReadQueries(std::string query_file, std::vector<std::string>& queries) {
    uint64_t query_count = 1000000;

    std::ifstream query_stream(query_file);
    if (!query_stream.is_open()) {
      fprintf(stderr, "Error: Query file [%s] may be missing.\n",
              query_file.c_str());
      return;
    }

    std::string line, query, expected_count;
    std::vector<std::string> queries_temp;
    while (getline(query_stream, line)) {
      // Extract key and value
      int split_index = line.find_first_of('\t');
      expected_count = line.substr(0, split_index);
      query = line.substr(split_index + 1);
      queries_temp.push_back(query);
    }
    query_stream.close();

    ZipfGenerator z_q(skew_, queries_temp.size());

    for (uint64_t i = 0; i < query_count; i++) {
      queries.push_back(queries_temp.at(z_q.Next()));
    }
    queries_temp.clear();
    fprintf(stderr, "Loaded %llu search queries!\n", query_count);
  }

  double skew_;
  uint32_t num_keys_;
  std::vector<int32_t> primary_ids_;
  std::string query_file_;
  std::string benchmark_type_;
  SuccinctServiceClient *client_;
};

#endif
