#include "thrift/AdaptiveQueryService.h"
#include "thrift/succinct_constants.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include <cstdio>
#include <fstream>
#include <cstdint>

#include "../../include/succinct/LayeredSuccinctShard.hpp"
#include "thrift/ports.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class AdaptiveQueryServiceHandler : virtual public AdaptiveQueryServiceIf {
public:
    AdaptiveQueryServiceHandler(std::string filename, bool construct, uint32_t sa_sampling_rate, uint32_t isa_sampling_rate) {
        this->fd = NULL;
        this->construct = construct;
        this->filename = filename;
        this->is_init = false;
        std::ifstream input(filename);
        this->num_keys = std::count(std::istreambuf_iterator<char>(input),
                        std::istreambuf_iterator<char>(), '\n');
        input.close();
        this->isa_sampling_rate = isa_sampling_rate;
        this->sa_sampling_rate = sa_sampling_rate;
        init(0);
    }

    int32_t init(int32_t id) {
        fprintf(stderr, "Received INIT signal, initializing data structures...\n");
        fprintf(stderr, "Construct is set to %d\n", construct);

        fd = new LayeredSuccinctShard(id, filename, construct, sa_sampling_rate, isa_sampling_rate);
        if(construct) {
            fprintf(stderr, "Constructing data structures for file %s\n", filename.c_str());
            std::ofstream s_file(filename + ".succinct", std::ofstream::binary);
            fd->serialize(s_file);
            s_file.close();
        } else {
            fprintf(stderr, "Read data structures from file %s\n", filename.c_str());
        }
        fprintf(stderr, "Succinct data structures with original size = %llu\n", fd->original_size());
        fprintf(stderr, "Done initializing...\n");
        fprintf(stderr, "Waiting for queries...\n");
        is_init = true;
        return 0;
    }

    void get(std::string& _return, const int64_t key) {
        fd->get(_return, key);
    }

    void access(std::string& _return, const int64_t key, const int32_t offset, const int32_t len) {
        fd->access(_return, key, offset, len);
    }

    void search(std::set<int64_t>& _return, const std::string& query) {
        fd->search(_return, query);
    }

    int64_t count(const std::string& query) {
        return fd->count(query);
    }

    int32_t get_num_keys() {
        return fd->num_keys();
    }

    int64_t remove_layer(const int32_t layer_id) {
        fprintf(stderr, "Received remove layer request for layer_id = %d\n", layer_id);
        int64_t del_size = fd->remove_layer(layer_id);
        fprintf(stderr, "Completed remove layer request for layer_id = %d\n", layer_id);
        return del_size;
    }

    typedef unsigned long long int time_t;

    static time_t get_timestamp() {
        struct timeval now;
        gettimeofday (&now, NULL);

        return  now.tv_usec + (time_t)now.tv_sec * 1000000;
    }

    int64_t reconstruct_layer(const int32_t layer_id) {
        fprintf(stderr, "Received create layer request for layer_id = %d\n", layer_id);
        time_t start_time = get_timestamp();
        int64_t add_size = fd->reconstruct_layer(layer_id);
        time_t end_time = get_timestamp();
        fprintf(stderr, "Completed create layer request for layer_id = %d, time = %lu\n", layer_id, end_time - start_time);
        return add_size;
    }

    int64_t storage_size() {
        return fd->storage_size();
    }

private:
    LayeredSuccinctShard *fd;
    bool construct;
    std::string filename;
    bool is_init;
    uint32_t num_keys;
    uint32_t sa_sampling_rate;
    uint32_t isa_sampling_rate;
};

void print_usage(char *exec) {
    fprintf(stderr, "Usage: %s [-m mode] [-p port] [-s sa_sampling_rate] [-i isa_sampling_rate] [file]\n", exec);
}

int main(int argc, char **argv) {

    if(argc < 2 || argc > 10) {
        print_usage(argv[0]);
        return -1;
    }

    fprintf(stderr, "Command line: ");
    for(int i = 0; i < argc; i++) {
        fprintf(stderr, "%s ", argv[i]);
    }
    fprintf(stderr, "\n");

    int c;
    uint32_t mode = 0, port = QUERY_SERVER_PORT, sa_sampling_rate = 32, isa_sampling_rate = 32;
    while((c = getopt(argc, argv, "m:p:s:i:")) != -1) {
        switch(c) {
        case 'm':
            mode = atoi(optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 's':
            sa_sampling_rate = atoi(optarg);
            break;
        case 'i':
            isa_sampling_rate = atoi(optarg);
            break;
        default:
            mode = 0;
            port = QUERY_SERVER_PORT;
            isa_sampling_rate = 32;
        }
    }

    if(optind == argc) {
        print_usage(argv[0]);
        return -1;
    }

    std::string filename = std::string(argv[optind]);
    bool construct = (mode == 0) ? true : false;

    shared_ptr<AdaptiveQueryServiceHandler> handler(new AdaptiveQueryServiceHandler(filename, construct, sa_sampling_rate, isa_sampling_rate));
    shared_ptr<TProcessor> processor(new AdaptiveQueryServiceProcessor(handler));

    try {
        shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
        shared_ptr<TBufferedTransportFactory> transport_factory(new TBufferedTransportFactory());
        shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());
        TThreadedServer server(processor,
                         server_transport,
                         transport_factory,
                         protocol_factory);
        server.serve();
    } catch(std::exception& e) {
        fprintf(stderr, "Exception at AdaptiveQueryServer:main(): %s\n", e.what());
    }
    return 0;
}
