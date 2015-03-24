#ifndef ADAPTIVE_QUERY_SERVER_BENCHMARK_HPP
#define ADAPTIVE_QUERY_SERVER_BENCHMARK_HPP

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "Benchmark.hpp"
#include "ZipfGenerator.hpp"
#include "thrift/AdaptiveQueryService.h"
#include "thrift/ports.h"

#include <thread>
#include <sstream>
#include <unistd.h>
#include <atomic>
#include <functional>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class AdaptiveQueryServerBenchmark : public Benchmark {
private:
    std::vector<int32_t> lengths;
    std::string reqfile;
    std::string resfile;
    double skew_keys;
    double skew_lengths;

    int32_t len;

    void generate_randoms() {
        boost::shared_ptr<TTransport> transport;
        AdaptiveQueryServiceClient *client = get_client(transport);
        count_t q_cnt = client->get_num_keys();
        fprintf(stderr, "Generating zipf distribution with theta=%f, N=%lu...\n", skew_keys, q_cnt);
        ZipfGenerator z(skew_keys, q_cnt);
        fprintf(stderr, "Generated zipf distribution, generating keys...\n");
        for(count_t i = 0; i < 100000; i++) {
            randoms.push_back(z.next());
        }
        fprintf(stderr, "Generated keys.\n");
        transport->close();
        delete client;
    }

    void generate_lengths() {
        int32_t min_len = 10;
        int32_t max_len = 1000;
        fprintf(stderr, "Generating zipf distribution with theta=%f, N=%u...\n", skew_lengths, (max_len - min_len));
        ZipfGenerator z(skew_lengths, max_len - min_len);
        fprintf(stderr, "Generated zipf distribution, generating lengths...\n");

        for(count_t i = 0; i < 100000; i++) {
            // Map zipf value to a length
            int32_t len = z.next() + min_len;
            assert(len >= min_len);
            assert(len <= max_len);
            lengths.push_back(len);
        }
        fprintf(stderr, "Generated lengths.\n");
    }

    void read_queries(std::string filename) {
        std::ifstream inputfile(filename);
        if(!inputfile.is_open()) {
            fprintf(stderr, "Error: Query file [%s] may be missing.\n",
                    filename.c_str());
            return;
        }

        std::string line, bin, query;
        while (getline(inputfile, line)) {
            // Extract key and value
            int split_index = line.find_first_of('\t');
            bin = line.substr(0, split_index);
            query = line.substr(split_index + 1);
            queries.push_back(query);
        }
        inputfile.close();
    }

    AdaptiveQueryServiceClient *get_client(boost::shared_ptr<TTransport> &c_transport) {
        int port = QUERY_SERVER_PORT;
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        AdaptiveQueryServiceClient *client = new AdaptiveQueryServiceClient(protocol);
        transport->open();
        c_transport = transport;
        return client;
    }

public:
    AdaptiveQueryServerBenchmark(std::string reqfile, std::string resfile, double skew, int32_t len, std::string queryfile = "") : Benchmark() {

        this->reqfile = reqfile;
        this->resfile = resfile;
        this->skew_keys = skew;
        this->skew_lengths = 1.0; // Pure uniform for now

        this->len = len;

        generate_randoms();
        generate_lengths();
        if(queryfile != "") {
            read_queries(queryfile);
        }
    }

    static void search_send_requests(AdaptiveQueryServiceClient *query_client,
            int64_t storage_size,
            std::vector<std::string> queries,
            std::string reqfile) {

        std::ofstream req_stream(reqfile, std::ofstream::out | std::ofstream::app);
        uint64_t num_requests = 0;

        time_t start_time = get_timestamp();
        while(num_requests <= queries.size()) {
            query_client->send_search(queries[num_requests % queries.size()]);
            num_requests++;
        }
        time_t diff = get_timestamp() - start_time;
        double rr = ((double) num_requests * 1000 * 1000) / ((double)diff);
        req_stream << storage_size << "\t" << rr << "\n";
        req_stream.close();
    }

    static void search_measure_responses(AdaptiveQueryServiceClient *query_client,
            int64_t storage_size,
            std::vector<std::string> queries,
            std::string resfile) {

        std::ofstream res_stream(resfile, std::ofstream::out | std::ofstream::app);
        uint64_t num_responses = 0;
        time_t start_time = get_timestamp();
        while(num_responses <= queries.size()) {
            try {
                std::set<int64_t> res;
                query_client->recv_search(res);
                num_responses++;
            } catch(std::exception& e) {
                break;
            }
        }
        time_t diff = get_timestamp() - start_time;
        double rr = ((double) num_responses * 1000 * 1000) / ((double) diff);
        res_stream << storage_size << "\t" << rr << "\n";
        res_stream.close();
    }

    static void send_requests(AdaptiveQueryServiceClient *query_client,
            int64_t storage_size,
            std::vector<int64_t> randoms,
            uint32_t len,
            std::string reqfile) {

        std::ofstream req_stream(reqfile, std::ofstream::out | std::ofstream::app);
        uint64_t num_requests = 0;

        time_t start_time = get_timestamp();
        while(num_requests <= randoms.size()) {
            query_client->send_access(randoms[num_requests % randoms.size()], 0, len);
            num_requests++;
        }
        time_t diff = get_timestamp() - start_time;
        double rr = ((double) num_requests * 1000 * 1000) / ((double)diff);
        req_stream << storage_size << "\t" << rr << "\n";
        req_stream.close();
    }

    static void measure_responses(AdaptiveQueryServiceClient *query_client,
            int64_t storage_size,
            std::vector<int64_t> randoms,
            std::string resfile) {

        std::string res;
        std::ofstream res_stream(resfile, std::ofstream::out | std::ofstream::app);
        uint64_t num_responses = 0;
        time_t start_time = get_timestamp();
        while(num_responses <= randoms.size()) {
            try {
                query_client->recv_access(res);
                num_responses++;
            } catch(std::exception& e) {
                break;
            }
        }
        time_t diff = get_timestamp() - start_time;
        double rr = ((double) num_responses * 1000 * 1000) / ((double) diff);
        res_stream << storage_size << "\t" << rr << "\n";
        res_stream.close();
    }

    static void send_requests_batch(AdaptiveQueryServiceClient *query_client,
                int64_t storage_size,
                std::vector<int64_t> randoms,
                uint32_t len,
                uint32_t batch_size,
                std::string reqfile) {

        std::ofstream req_stream(reqfile, std::ofstream::out | std::ofstream::app);
        uint64_t num_requests = 0;

        time_t start_time = get_timestamp();
        while(num_requests <= randoms.size()) {
            // Prepare batch
            std::vector<int64_t> keys;
            for(uint32_t i = 0; i < batch_size; i++) {
                keys.push_back(randoms[(num_requests + i) % randoms.size()]);
            }
            query_client->send_batch_access(keys, 0, len);
            num_requests += batch_size;
        }
        time_t diff = get_timestamp() - start_time;
        double rr = ((double) num_requests * 1000 * 1000) / ((double)diff);
        req_stream << storage_size << "\t" << rr << "\n";
        req_stream.close();
    }

    static void measure_responses_batch(AdaptiveQueryServiceClient *query_client,
            int64_t storage_size,
            std::vector<int64_t> randoms,
            uint32_t batch_size,
            std::string resfile) {


        std::ofstream res_stream(resfile, std::ofstream::out | std::ofstream::app);
        uint64_t num_responses = 0;
        time_t start_time = get_timestamp();
        while(num_responses <= randoms.size()) {
            try {
                std::vector<std::string> res;
                res.resize(batch_size);
                query_client->recv_batch_access(res);
                num_responses += res.size();
                res.clear();
            } catch(std::exception& e) {
                break;
            }
        }
        time_t diff = get_timestamp() - start_time;
        double rr = ((double) num_responses * 1000 * 1000) / ((double) diff);
        res_stream << storage_size << "\t" << rr << "\n";
        res_stream.close();
    }

    void delete_layer(int32_t layer_to_delete) {
        boost::shared_ptr<TTransport> query_transport;
        AdaptiveQueryServiceClient *query_client = get_client(query_transport);

        if(layer_to_delete != -1) {
            fprintf(stderr, "Removing layer %d\n", layer_to_delete);
            query_client->remove_layer(layer_to_delete);
            fprintf(stderr, "Removed layer %d\n", layer_to_delete);
        }

        query_transport->close();
        delete query_client;
    }

    void measure_throughput_search() {

        boost::shared_ptr<TTransport> query_transport;
        AdaptiveQueryServiceClient *query_client = get_client(query_transport);

        int64_t storage_size = query_client->storage_size();

        fprintf(stderr, "Starting request thread...\n");
        std::thread req(&AdaptiveQueryServerBenchmark::search_send_requests, query_client, storage_size, queries, reqfile);

        fprintf(stderr, "Starting response thread...\n");
        std::thread res(&AdaptiveQueryServerBenchmark::search_measure_responses, query_client, storage_size, queries, resfile);

        if(req.joinable()) {
            req.join();
            fprintf(stderr, "Request thread terminated.\n");
        }

        if(res.joinable()) {
            res.join();
            fprintf(stderr, "Response thread terminated.\n");
        }

        query_transport->close();
        delete query_client;
    }

    void measure_throughput() {

        boost::shared_ptr<TTransport> query_transport;
        AdaptiveQueryServiceClient *query_client = get_client(query_transport);

        int64_t storage_size = query_client->storage_size();

        fprintf(stderr, "Starting request thread...\n");
        std::thread req(&AdaptiveQueryServerBenchmark::send_requests, query_client, storage_size, randoms, len, reqfile);

        fprintf(stderr, "Starting response thread...\n");
        std::thread res(&AdaptiveQueryServerBenchmark::measure_responses, query_client, storage_size, randoms, resfile);

        if(req.joinable()) {
            req.join();
            fprintf(stderr, "Request thread terminated.\n");
        }

        if(res.joinable()) {
            res.join();
            fprintf(stderr, "Response thread terminated.\n");
        }

        query_transport->close();
        delete query_client;
    }

    void measure_throughput_batch(int32_t batch_size) {
        boost::shared_ptr<TTransport> query_transport;
        AdaptiveQueryServiceClient *query_client = get_client(query_transport);

        int64_t storage_size = query_client->storage_size();

        fprintf(stderr, "Starting request thread...\n");
        std::thread req(&AdaptiveQueryServerBenchmark::send_requests_batch, query_client, storage_size, randoms, len, batch_size, reqfile + ".batch." + std::to_string(batch_size));

        fprintf(stderr, "Starting response thread...\n");
        std::thread res(&AdaptiveQueryServerBenchmark::measure_responses_batch, query_client, storage_size, randoms, batch_size, resfile + ".batch." + std::to_string(batch_size));

        if(req.joinable()) {
            req.join();
            fprintf(stderr, "Request thread terminated.\n");
        }

        if(res.joinable()) {
            res.join();
            fprintf(stderr, "Response thread terminated.\n");
        }

        query_transport->close();
        delete query_client;
    }
};

#endif
