#ifndef DYNAMIC_LOAD_BALANCER_BENCHMARK_HPP
#define DYNAMIC_LOAD_BALANCER_BENCHMARK_HPP

#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "Benchmark.hpp"
#include "ZipfGenerator.hpp"
#include "thrift/AdaptiveQueryService.h"
#include "thrift/DynamicLoadBalancer.hpp"
#include "thrift/ports.h"

#include <thread>
#include <sstream>
#include <unistd.h>
#include <atomic>
#include <functional>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

class DynamicLoadBalancerBenchmark : public Benchmark {
private:
    std::vector<AdaptiveQueryServiceClient *> query_client;
    std::vector<AdaptiveQueryServiceClient *> stats_client;
    std::vector<AdaptiveQueryServiceClient *> mgmnt_client;
    std::vector<boost::shared_ptr<TTransport>> query_transport;
    std::vector<boost::shared_ptr<TTransport>> stats_transport;
    std::vector<boost::shared_ptr<TTransport>> mgmnt_transport;
    std::vector<uint32_t> request_rates;
    std::vector<uint32_t> durations;
    std::vector<std::vector<uint32_t>> layers_to_delete;
    std::vector<std::vector<uint32_t>> layers_to_create;
    std::string reqfile;
    std::string resfile;
    std::string addfile;
    std::string delfile;
    std::atomic<uint64_t> *queue_length;
    double skew;
    std::vector<std::string> replicas;
    uint32_t num_active_replicas;

    void generate_randoms() {
        count_t q_cnt = query_client[0]->get_num_keys();
        fprintf(stderr, "Generating zipf distribution with theta=%f, N=%lu...\n", skew, q_cnt);
        ZipfGenerator z(skew, q_cnt);
        fprintf(stderr, "Generated zipf distribution, generating keys...\n");
        for(count_t i = 0; i < q_cnt; i++) {
            randoms.push_back(z.next());
        }
        fprintf(stderr, "Generated keys.\n");
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
            std::sort(l_add.begin(), l_add.end(), std::greater<uint32_t>());
            std::sort(l_del.begin(), l_del.end());
            layers_to_create.push_back(l_add);
            layers_to_delete.push_back(l_del);
        }
        assert(request_rates.size() == durations.size());
    }

    AdaptiveQueryServiceClient *get_client(std::string host, boost::shared_ptr<TTransport> &c_transport) {
        int port = QUERY_SERVER_PORT;
        boost::shared_ptr<TSocket> socket(new TSocket(host, port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        AdaptiveQueryServiceClient *client = new AdaptiveQueryServiceClient(protocol);
        transport->open();
        c_transport = transport;
        return client;
    }

public:
    DynamicLoadBalancerBenchmark(std::string configfile, std::string reqfile, std::string resfile,
            std::string addfile, std::string delfile, double skew, std::vector<std::string> replicas,
            uint32_t num_active_replicas, std::string queryfile) : Benchmark() {

        this->replicas = replicas;
        this->num_active_replicas = num_active_replicas;

        this->query_client.resize(replicas.size());
        this->mgmnt_client.resize(replicas.size());
        this->stats_client.resize(replicas.size());
        this->query_transport.resize(replicas.size());
        this->mgmnt_transport.resize(replicas.size());
        this->stats_transport.resize(replicas.size());
        this->queue_length = new std::atomic<uint64_t>[replicas.size()];
        for(uint32_t i = 0; i < replicas.size(); i++) {
            this->query_client[i] = this->get_client(replicas[i], query_transport[i]);
            fprintf(stderr, "Created query client %u.\n", i);
            this->mgmnt_client[i] = this->get_client(replicas[i], mgmnt_transport[i]);
            fprintf(stderr, "Created management client %u.\n", i);
            this->stats_client[i] = this->get_client(replicas[i], stats_transport[i]);
            fprintf(stderr, "Created stats client %u.\n", i);
            this->queue_length[i] = 0;
        }

        this->reqfile = reqfile;
        this->resfile = resfile;
        this->addfile = addfile;
        this->delfile = delfile;

        this->skew = skew;

        // generate_randoms();
        if(queryfile != "") {
            read_queries(queryfile);
        }

        if(randoms.empty() && queries.empty()) {
            fprintf(stderr, "Warning: No search or get queries loaded.\n");
        }

        parse_config_file(configfile);
    }

    static void send_requests(std::vector<AdaptiveQueryServiceClient *> query_client,
            std::vector<std::string> queries,
            std::vector<uint32_t> request_rates,
            std::vector<uint32_t> durations,
            std::atomic<uint64_t> *queue_length,
            std::atomic<double> *avg_qlens,
            std::string reqfile) {

        time_t cur_time;
        const time_t MEASURE_INTERVAL = 40000000;
        std::ofstream req_stream(reqfile);
        uint64_t num_requests = 0;
        DynamicLoadBalancer lb(query_client.size());

        time_t measure_start_time = get_timestamp();
        for(uint32_t stage = 0; stage < request_rates.size(); stage++) {
            time_t duration = ((uint64_t)durations[stage]) * 1000L * 1000L;   // Seconds to microseconds
            time_t sleep_time = (1000 * 1000) / request_rates[stage];
            uint64_t i = 0;
            fprintf(stderr, "Starting stage %u: request-rate = %u Ops/sec, duration = %llu us\n",
                    stage, request_rates[stage], duration);
            time_t stage_start_time = get_timestamp();
            while((cur_time = get_timestamp()) - stage_start_time <= duration) {
                time_t t0 = get_timestamp();
                uint32_t replica_id = lb.get_replica(avg_qlens);
                query_client[replica_id]->send_search(queries[i % queries.size()]);
                i++;
                num_requests++;
                queue_length[replica_id]++;
                while(get_timestamp() - t0 < sleep_time);
                if((cur_time = get_timestamp()) - measure_start_time >= MEASURE_INTERVAL) {
                    time_t diff = cur_time - measure_start_time;
                    double rr = ((double) num_requests * 1000 * 1000) / ((double)diff);
                    req_stream << cur_time << "\t" << rr;
                    std::vector<double> allocations;
                    lb.get_latest_allocations(allocations);
                    for(size_t i = 0; i < allocations.size(); i++) {
                        req_stream << "\t" << allocations[i];
                    }
                    req_stream << "\n";
                    req_stream.flush();
                    num_requests = 0;
                    measure_start_time = get_timestamp();
                }
            }
            fprintf(stderr, "Finished stage %u, spent %llu us.\n", stage, (cur_time  - stage_start_time));
        }
        time_t diff = cur_time - measure_start_time;
        double rr = ((double) num_requests * 1000 * 1000) / ((double)diff);
        req_stream << cur_time << "\t" << rr;
        std::vector<double> allocations;
        lb.get_latest_allocations(allocations);
        for(size_t i = 0; i < allocations.size(); i++) {
            req_stream << "\t" << allocations[i];
        }
        req_stream << "\n";
        req_stream.close();
    }

    static void measure_responses(AdaptiveQueryServiceClient *query_client,
            AdaptiveQueryServiceClient *stats_client,
            std::atomic<uint64_t> &queue_length, std::string resfile) {

        const time_t MEASURE_INTERVAL = 40000000;
        uint32_t num_responses = 0;
        time_t cur_time;
        std::ofstream res_stream(resfile);
        time_t measure_start_time = get_timestamp();
        while(true) {
            try {
                std::set<int64_t> res;
                query_client->recv_search(res);
                num_responses++;
                queue_length--;
                if((cur_time = get_timestamp()) - measure_start_time >= MEASURE_INTERVAL) {
                    time_t diff = cur_time - measure_start_time;
                    double thput = ((double) num_responses * 1000 * 1000) / ((double)diff);
                    res_stream << cur_time << "\t" << thput << "\t" << stats_client->storage_size()
                            << "\t" << queue_length << "\n";
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
        res_stream << cur_time << "\t" << thput << "\t" << stats_client->storage_size()
                                    << "\t" << stats_client->num_sampled_values() << "\n";
        res_stream.close();
    }

    static void manage_layers(AdaptiveQueryServiceClient *mgmt_client,
            bool is_active,
            std::vector<std::vector<uint32_t>> layers_to_create,
            std::vector<std::vector<uint32_t>> layers_to_delete,
            std::vector<uint32_t> durations,
            std::string addfile,
            std::string delfile) {

        std::ofstream add_stream(addfile);
        std::ofstream del_stream(delfile);
        time_t cur_time;

        for(uint32_t stage = 0; stage < layers_to_create.size(); stage++) {
            time_t duration = ((uint64_t)durations[stage]) * 1000L * 1000L;   // Seconds to microseconds
            time_t start_time = get_timestamp();
            if(stage == 0 || is_active) {
                for(size_t i = 0; i < layers_to_create[stage].size(); i++) {
                    try {
                        size_t add_size = mgmt_client->reconstruct_layer(layers_to_create[stage][i]);
                        fprintf(stderr, "Created layer with size = %zu\n", add_size);
                        add_stream << get_timestamp() << "\t" << i << "\t" << add_size << "\n";
                        add_stream.flush();
                    } catch(std::exception& e) {
                        fprintf(stderr, "Error: %s\n", e.what());
                        break;
                    }
                }

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
            }

            // Sleep if there is still time left
            if((cur_time = get_timestamp()) - start_time < duration) {
                fprintf(stderr, "Done with layer management for stage %u, took %llu us, sleeping for %llu us.\n",
                    stage, (cur_time - start_time), (duration - (cur_time - start_time)));
                usleep(duration - (cur_time - start_time));
            }
        }
    }

    static void monitor_queues(std::atomic<uint64_t> *queue_lengths, std::atomic<double> *avg_qlens, uint64_t num_replicas) {

        double alpha = 0.9;
        uint32_t delta = 5;

        sleep(delta);

        while(true) {
            fprintf(stderr, "[QM]");
            for(uint32_t i = 0; i < num_replicas; i++) {
                avg_qlens[i] = queue_lengths[i] * alpha + (1.0 - alpha) * avg_qlens[i];
                double val = avg_qlens[i];
                fprintf(stderr, "\t%f", val);
            }
            fprintf(stderr, "\n");
            sleep(delta);
        }
    }

    void run_benchmark() {

        std::atomic<double> *avg_qlens = new std::atomic<double>[replicas.size()];
        for(uint32_t i = 0; i < replicas.size(); i++) {
            avg_qlens[i] = 0.0;
        }

        std::thread queue_monitor_daemon(&DynamicLoadBalancerBenchmark::monitor_queues, queue_length, avg_qlens, replicas.size());

        std::thread req(&DynamicLoadBalancerBenchmark::send_requests,
                query_client, queries, request_rates, durations, queue_length,
                avg_qlens,
                reqfile);

        std::thread *res = new std::thread[query_client.size()];
        std::thread *lay = new std::thread[query_client.size()];
        for(uint32_t i = 0; i < query_client.size(); i++) {
            res[i] = std::thread(&DynamicLoadBalancerBenchmark::measure_responses, query_client[i],
                    stats_client[i], std::ref(queue_length[i]), resfile + "." + std::to_string(i));
            lay[i] = std::thread(&DynamicLoadBalancerBenchmark::manage_layers, mgmnt_client[i], (i < num_active_replicas),
                                layers_to_create, layers_to_delete, durations, addfile, delfile);

        }

        if(req.joinable()) {
            req.join();
            fprintf(stderr, "Request thread terminated.\n");
        }

        // Sleep for some time and let other threads finish
        fprintf(stderr, "Sleeping for 10 seconds...\n");
        sleep(10);
        fprintf(stderr, "Finished sending queries, attempting to close query sockets...\n");
        for(auto qt : query_transport)
            qt->close();
        fprintf(stderr, "Closed query sockets.\n");

        for(uint32_t i = 0; i < query_client.size(); i++) {
            if(res[i].joinable()) {
                res[i].join();
                fprintf(stderr, "Response %u thread terminated.\n", i);
            }
            if(lay[i].joinable()) {
                lay[i].join();
                fprintf(stderr, "Layer creation thread terminated.\n");
            }
        }

        for(auto mt : mgmnt_transport)
            mt->close();
        fprintf(stderr, "Closed management sockets.\n");

        for(auto st: stats_transport)
            st->close();
        fprintf(stderr, "Closed stats sockets.\n");
    }
};

#endif
