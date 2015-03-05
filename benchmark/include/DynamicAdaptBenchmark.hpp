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
    AdaptiveQueryServiceClient *stats_client;
    AdaptiveQueryServiceClient *mgmnt_client;
    boost::shared_ptr<TTransport> query_transport;
    boost::shared_ptr<TTransport> stats_transport;
    boost::shared_ptr<TTransport> mgmnt_transport;
    std::vector<uint32_t> request_rates;
    std::vector<uint32_t> durations;
    std::vector<std::vector<uint32_t>> layers_to_delete;
    std::vector<std::vector<uint32_t>> layers_to_create;
    std::string reqfile;
    std::string resfile;
    std::string addfile;
    std::string delfile;

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

    void parse_csv_entry(std::vector<uint32_t> &out, std::string csv_entry) {
        std::string delimiter = ",";
        size_t pos = 0;
        std::string elem;
        while ((pos = csv_entry.find(delimiter)) != std::string::npos) {
            elem = csv_entry.substr(0, pos);
            out.push_back(atoi(elem.c_str()));
            csv_entry.erase(0, pos + delimiter.length());
        }
        if(csv_entry != "-") {
            out.push_back(atoi(csv_entry.c_str()));
            std::sort(out.begin(), out.end(), std::greater<uint32_t>());
        } else {
            assert(out.empty());
        }
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
            iss >> add;
            iss >> del;
            fprintf(stderr, "rr = %s, dur = %s, add = %s, del = %s\n",
                    rr.c_str(), dur.c_str(), add.c_str(), del.c_str());

            request_rates.push_back(atoi(rr.c_str()));
            durations.push_back(atoi(dur.c_str()));
            parse_csv_entry(l_add, add);
            parse_csv_entry(l_del, del);
            layers_to_create.push_back(l_add);
            layers_to_delete.push_back(l_del);
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
            std::string addfile, std::string delfile, std::string queryfile = "") : Benchmark() {

        this->query_client = this->get_client(query_transport);
        fprintf(stderr, "Created query client.\n");
        this->mgmnt_client = this->get_client(mgmnt_transport);
        fprintf(stderr, "Created management client.\n");
        this->stats_client = this->get_client(stats_transport);
        fprintf(stderr, "Created stats client.\n");

        this->reqfile = reqfile;
        this->resfile = resfile;
        this->addfile = addfile;
        this->delfile = delfile;

        generate_randoms();
        if(queryfile != "") {
            read_queries(queryfile);
        }
        parse_config_file(configfile);
    }

    static void send_requests(AdaptiveQueryServiceClient *query_client,
            boost::shared_ptr<TTransport> query_transport,
            AdaptiveQueryServiceClient *mgmt_client,
            boost::shared_ptr<TTransport> mgmt_transport,
            std::vector<int64_t> randoms,
            std::vector<uint32_t> request_rates,
            std::vector<uint32_t> durations,
            std::vector<std::vector<uint32_t>> layers_to_create,
            std::vector<std::vector<uint32_t>> layers_to_delete,
            std::string reqfile) {

        time_t cur_time;
        const time_t MEASURE_INTERVAL = 5000000;
        time_t measure_start_time = get_timestamp();
        std::ofstream req_stream(reqfile);
        uint64_t num_requests = 0;

        for(uint32_t stage = 0; stage < request_rates.size(); stage++) {
            time_t duration = ((uint64_t)durations[stage]) * 1000L * 1000L;   // Seconds to microseconds
            time_t sleep_time = 1000 * 1000 / request_rates[stage];
            uint64_t i = 0;
            fprintf(stderr, "Starting stage %u: request-rate = %u Ops/sec, duration = %llu us\n",
                    stage, request_rates[stage], duration);
            time_t start_time = get_timestamp();
            while((cur_time = get_timestamp()) - start_time <= duration) {
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
    }

    static void measure_responses(AdaptiveQueryServiceClient *query_client,
            AdaptiveQueryServiceClient *stats_client, std::string resfile) {
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
                    res_stream << cur_time << "\t" << thput << "\t" << stats_client->storage_size() << "\n";
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
        res_stream << cur_time << "\t" << thput << "\t" << stats_client->storage_size() << "\n";
        res_stream.close();
    }

    static void create_layers(AdaptiveQueryServiceClient *mgmt_client,
            std::vector<std::vector<uint32_t>> layers_to_create,
            std::vector<uint32_t> durations,
            std::string addfile) {
        std::ofstream add_stream(addfile);
        time_t cur_time;

        for(size_t stage = 0; stage < layers_to_create.size(); stage++) {
            time_t duration = durations[stage] * 1000 * 1000;   // Seconds to microseconds
            time_t start_time = get_timestamp();
            for(size_t i = 0; i < layers_to_create[stage].size(); i++) {
                try {
                    size_t add_size = mgmt_client->reconstruct_layer(layers_to_create[stage][i]);
                    fprintf(stderr, "Created layer with size = %zu\n", add_size);
                    add_stream << get_timestamp() << "\t" << i << "\t" << add_size << "\n";
                    add_stream.flush();
                } catch(std::exception& e) {
                    break;
                }
            }
            // Sleep if there is still time left
            if((cur_time = get_timestamp()) - start_time < duration) {
                usleep(duration - (cur_time - start_time));
            }
        }
    }

    static void delete_layers(AdaptiveQueryServiceClient *mgmt_client,
                std::vector<std::vector<uint32_t>> layers_to_delete,
                std::vector<uint32_t> durations,
                std::string delfile) {
        std::ofstream del_stream(delfile);
        time_t cur_time;

        for(size_t stage = 0; stage < layers_to_delete.size(); stage++) {
            time_t duration = durations[stage] * 1000 * 1000;   // Seconds to microseconds
            time_t start_time = get_timestamp();
            for(size_t i = 0; i < layers_to_delete[stage].size(); i++) {
                try {
                    size_t del_size = mgmt_client->remove_layer(layers_to_delete[stage][i]);
                    fprintf(stderr, "Deleted layer with size = %zu\n", del_size);
                    del_stream << get_timestamp() << "\t" << i << "\t" << del_size << "\n";
                    del_stream.flush();
                } catch(std::exception& e) {
                    fprintf(stderr, "Error: %s\n", e.what());
                    break;
                }
            }
            // Sleep if there is still time left
            if((cur_time = get_timestamp()) - start_time < duration) {
                usleep(duration - (cur_time - start_time));
            }
        }
    }

    void run_benchmark() {
        std::thread req(&DynamicAdaptBenchmark::send_requests,
                query_client, query_transport,
                mgmnt_client, mgmnt_transport,
                randoms, request_rates, durations, layers_to_create, layers_to_delete,
                reqfile);
        std::thread res(&DynamicAdaptBenchmark::measure_responses, query_client,
                stats_client, resfile);
        std::thread add(&DynamicAdaptBenchmark::create_layers, mgmnt_client,
                layers_to_create, durations, addfile);
        std::thread del(&DynamicAdaptBenchmark::delete_layers, mgmnt_client,
                layers_to_delete, durations, delfile);

        if(req.joinable()) {
            req.join();
            fprintf(stderr, "Request thread terminated.\n");
        }

        if(res.joinable()) {
            res.join();
            fprintf(stderr, "Response thread terminated.\n");
        }

        if(add.joinable()) {
            add.join();
            fprintf(stderr, "Layer creation thread terminated.\n");
        }

        if(del.joinable()) {
            del.join();
            fprintf(stderr, "Layer deletion thread terminated.\n");
        }

        mgmnt_transport->close();
        fprintf(stderr, "Closed management socket.\n");

        stats_transport->close();
        fprintf(stderr, "Closed stats socket.\n");
    }
};

#endif
