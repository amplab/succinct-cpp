#ifndef DYNAMIC_ADAPT_BENCHMARK_HPP
#define DYNAMIC_ADAPT_BENCHMARK_HPP

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "Benchmark.hpp"
#include "thrift/AdaptiveQueryService.h"
#include "thrift/ports.h"

#include <thread>
#include <sstream>
#include <unistd.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class DynamicAdaptBenchmark : public Benchmark {
private:
    AdaptiveQueryServiceClient *query_client;
    AdaptiveQueryServiceClient *mgmt_client;
    boost::shared_ptr<TTransport> query_transport;
    boost::shared_ptr<TTransport> layer_transport;
    std::vector<uint32_t> request_rates;
    std::vector<uint32_t> durations;
    std::string reqfile;
    std::string resfile;

    void generate_randoms() {
        count_t q_cnt = WARMUP_N + COOLDOWN_N + MEASURE_N;
        for(count_t i = 0; i < q_cnt; i++) {
            randoms.push_back(rand() % query_client->get_num_keys());
        }
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

    void parse_config_file(std::string configfile) {
        std::ifstream conf(configfile);
        assert(conf.is_open());
        std::string conf_entry;
        while(std::getline(conf, conf_entry, '\n')) {
            std::istringstream iss(conf_entry);
            std::string rr, dur;
            iss >> rr;
            iss >> dur;
            request_rates.push_back(atoi(rr.c_str()));
            durations.push_back(atoi(dur.c_str()));
        }
        assert(request_rates.size() == durations.size());
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
    DynamicAdaptBenchmark(std::string configfile, std::string reqfile, std::string resfile,
            std::string queryfile = "") : Benchmark() {
        this->query_client = this->get_client(query_transport);
        fprintf(stderr, "Created query client.\n");
        this->mgmt_client = this->get_client(layer_transport);
        fprintf(stderr, "Created layer client.\n");
        this->reqfile = reqfile;
        this->resfile = resfile;

        generate_randoms();
        if(queryfile != "") {
            read_queries(queryfile);
        }
        parse_config_file(configfile);
    }

    static void send_requests(AdaptiveQueryServiceClient *query_client,
            boost::shared_ptr<TTransport> query_transport,
            std::vector<int64_t> randoms,
            std::vector<uint32_t> request_rates,
            std::vector<uint32_t> durations,
            std::string reqfile) {
        time_t cur_time;
        const time_t MEASURE_INTERVAL = 5000000;
        time_t measure_start_time = get_timestamp();
        std::ofstream req_stream(reqfile);
        uint64_t num_requests = 0;
        for(uint32_t stage = 0; stage < request_rates.size(); stage++) {
            time_t duration = durations[stage] * 1000 * 1000;   // Seconds to microseconds
            time_t sleep_time = 1000 * 1000 / request_rates[stage];
            uint64_t i = 0;
            fprintf(stderr, "Starting stage %u: request-rate = %u, duration = %u\n",
                    stage, request_rates[stage], durations[stage]);
            time_t start_time = get_timestamp();
            while(get_timestamp() - start_time < duration) {
                query_client->send_get(randoms[i % randoms.size()]);
                i++;
                num_requests++;
                usleep(sleep_time);
                if((cur_time = get_timestamp()) - measure_start_time >= MEASURE_INTERVAL) {
                    time_t diff = cur_time - measure_start_time;
                    double rr = ((double) num_requests * 1000 * 1000) / ((double)diff);
                    req_stream << cur_time << "\t" << rr << "\n";
                    req_stream.flush();
                    num_requests = 0;
                    measure_start_time = get_timestamp();
                }
            }
        }
        time_t diff = cur_time - measure_start_time;
        double rr = ((double) num_requests * 1000 * 1000) / ((double)diff);
        req_stream << cur_time << "\t" << rr << "\n";
        req_stream.flush();
        query_transport->close();
    }

    static void measure_responses(AdaptiveQueryServiceClient *query_client,
            AdaptiveQueryServiceClient *mgmt_client, std::string resfile) {
        std::string res;
        const time_t MEASURE_INTERVAL = 5000000;
        uint32_t num_responses = 0;
        time_t cur_time;
        std::ofstream res_stream(resfile);
        time_t measure_start_time = get_timestamp();
        while(true) {
            try {
                query_client->recv_get(res);
                num_responses++;
                if((cur_time = get_timestamp()) - measure_start_time >= MEASURE_INTERVAL) {
                    time_t diff = cur_time - measure_start_time;
                    double thput = ((double) num_responses * 1000 * 1000) / ((double)diff);
                    res_stream << cur_time << "\t" << thput << "\t" << mgmt_client->storage_size() << "\n";
                    res_stream.flush();
                    num_responses = 0;
                    measure_start_time = get_timestamp();
                }
            } catch(std::exception& e) {
                break;
            }
        }
        time_t diff = cur_time - measure_start_time;
        double thput = ((double) num_responses * 1000 * 1000) / ((double) diff);
        res_stream << cur_time << "\t" << thput << "\t" << mgmt_client->storage_size() << "\n";
        res_stream.close();
    }

    void run_benchmark() {
        std::thread req(&DynamicAdaptBenchmark::send_requests, query_client, query_transport,
                randoms, request_rates, durations, reqfile);
        std::thread res(&DynamicAdaptBenchmark::measure_responses, query_client, mgmt_client, resfile);

        if(req.joinable()) {
            req.join();
            fprintf(stderr, "Request thread terminated.\n");
        }

        if(res.joinable()) {
            res.join();
            fprintf(stderr, "Response thread terminated.\n");
        }
    }
};

#endif
