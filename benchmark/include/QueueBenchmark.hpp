#ifndef ADAPT_BENCHMARK_HPP
#define ADAPT_BENCHMARK_HPP

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "Benchmark.hpp"
#include "thrift/AdaptiveSuccinctService.h"
#include "thrift/ports.h"

#include <thread>
#include <sstream>
#include <unistd.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class QueueBenchmark : public Benchmark {
private:
    AdaptiveSuccinctServiceClient *query_client;
    AdaptiveSuccinctServiceClient *stats_client;
    AdaptiveSuccinctServiceClient *resp_client;
    boost::shared_ptr<TTransport> query_transport;
    boost::shared_ptr<TTransport> stats_transport;
    boost::shared_ptr<TTransport> resp_transport;
    std::vector<uint32_t> request_rates;
    std::vector<uint32_t> durations;
    std::string reqfile;
    std::string resfile;

    void generate_randoms() {
        count_t q_cnt = query_client->get_num_keys(0);
        for(count_t i = 0; i < q_cnt; i++) {
            randoms.push_back(rand() % q_cnt);
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
            std::string rr, dur, add, del;
            std::vector<uint32_t> l_add, l_del;
            iss >> rr;
            iss >> dur;
            fprintf(stderr, "rr = %s, dur = %s\n",
                    rr.c_str(), dur.c_str());

            request_rates.push_back(atoi(rr.c_str()));
            durations.push_back(atoi(dur.c_str()));
        }
        assert(request_rates.size() == durations.size());
    }

    AdaptiveSuccinctServiceClient *get_client(boost::shared_ptr<TTransport> &c_transport) {
        int port = QUERY_HANDLER_PORT;
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        AdaptiveSuccinctServiceClient *client = new AdaptiveSuccinctServiceClient(protocol);
        transport->open();
        c_transport = transport;
        return client;
    }

public:
    QueueBenchmark(std::string configfile, std::string reqfile, std::string resfile,
            std::string queryfile = "") : Benchmark() {

        this->query_client = this->get_client(query_transport);
        fprintf(stderr, "Created query client.\n");
        this->stats_client = this->get_client(stats_transport);
        fprintf(stderr, "Created stats client.\n");
        this->resp_client = this->get_client(resp_transport);
        fprintf(stderr, "Created response client.\n");

        this->reqfile = reqfile;
        this->resfile = resfile;

        generate_randoms();
        if(queryfile != "") {
            read_queries(queryfile);
        }
        parse_config_file(configfile);
    }

    static void send_requests(AdaptiveSuccinctServiceClient *query_client,
            boost::shared_ptr<TTransport> query_transport,
            boost::shared_ptr<TTransport> resp_transport,
            std::vector<int64_t> randoms,
            std::vector<uint32_t> request_rates,
            std::vector<uint32_t> durations,
            std::string reqfile) {

        time_t cur_time;
        const time_t MEASURE_INTERVAL = 5000000;
        time_t measure_start_time = get_timestamp();
        std::ofstream req_stream(reqfile);
        uint64_t num_requests = 0;

        uint64_t i = 0;
        for(uint32_t stage = 0; stage < request_rates.size(); stage++) {
            time_t duration = ((uint64_t)durations[stage]) * 1000L * 1000L;   // Seconds to microseconds
            time_t sleep_time = 1000 * 1000 / request_rates[stage];
            fprintf(stderr, "Starting stage %u: request-rate = %u Ops/sec, duration = %llu us\n",
                    stage, request_rates[stage], duration);
            time_t start_time = get_timestamp();
            while((cur_time = get_timestamp()) - start_time <= duration) {
                query_client->get_request(randoms[i % randoms.size()]);
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
            fprintf(stderr, "Finished stage %u, spent %llu us.\n", stage, (cur_time  - start_time));
        }
        time_t diff = cur_time - measure_start_time;
        double rr = ((double) num_requests * 1000 * 1000) / ((double)diff);
        req_stream << cur_time << "\t" << rr << "\n";
        req_stream.flush();

        // Sleep for some time and let other threads finish
        fprintf(stderr, "Request thread sleeping for 10 seconds...\n");
        sleep(10);
        fprintf(stderr, "Finished sending queries, attempting to close query socket...\n");
        query_transport->close();
        fprintf(stderr, "Closed query socket.\n");

        resp_transport->close();
        fprintf(stderr, "Closed response socket.\n");
    }

    static void measure_responses(AdaptiveSuccinctServiceClient *query_client,
            AdaptiveSuccinctServiceClient *stats_client, std::vector<int64_t> randoms,
            std::string resfile) {
        std::string res;
        const time_t MEASURE_INTERVAL = 5000000;
        uint32_t num_responses = 0;
        time_t cur_time;
        std::ofstream res_stream(resfile);
        uint64_t i = 0;
        time_t measure_start_time = get_timestamp();
        while(true) {
            try {
                query_client->get_response(res, randoms[i % randoms.size()]);
                num_responses++;
                i++;
                if((cur_time = get_timestamp()) - measure_start_time >= MEASURE_INTERVAL) {
                    time_t diff = cur_time - measure_start_time;
                    double thput = ((double) num_responses * 1000 * 1000) / ((double)diff);
                    res_stream << cur_time << "\t" << thput << "\t" << stats_client->get_queue_length(0) << "\n";
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
        res_stream << cur_time << "\t" << thput << "\t" << stats_client->get_queue_length(0) << "\n";
        res_stream.close();
    }

    void run_benchmark() {
        std::thread req(&QueueBenchmark::send_requests,
                query_client, query_transport,
                resp_transport,
                randoms, request_rates, durations,
                reqfile);
        fprintf(stderr, "Started request thread!\n");

        std::thread res(&QueueBenchmark::measure_responses, resp_client,
                stats_client, randoms, resfile);
        fprintf(stderr, "Started response thread!\n");

        if(req.joinable()) {
            req.join();
            fprintf(stderr, "Request thread terminated.\n");
        }

        if(res.joinable()) {
            res.join();
            fprintf(stderr, "Response thread terminated.\n");
        }

        stats_transport->close();
        fprintf(stderr, "Closed stats socket.\n");
    }
};

#endif
