#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <sys/time.h>

#include "succinct_semistructured_shard.h"

#include <boost/python.hpp>
using namespace boost::python;

/**
 * Prints usage.
 */
void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-m mode] [file]\n", exec);
}

void print_valid_cmds() {
  std::cerr << "Command must be one of:\n"
            << "\t\tsearch [attr_key] [attr_val]\n"
            << "\t\tcount [attr_key] [attr_val]\n"
            << "\t\tget [key] [attr_key]\n";
}

typedef unsigned long long int timestamp_t;

static timestamp_t get_timestamp() {
  struct timeval now{};
  gettimeofday(&now, nullptr);

  return (now.tv_usec + (time_t) now.tv_sec * 1000000);
}

struct QuerySemistructured{
    QuerySemistructured(std::string filename, uint32_t mode){
        this->s_file = nullptr;
        if (mode == 0) {
            // If mode is set to 0, compress the input file.
            // Use default parameters.
            std::cout << "Constructing Succinct data structures...\n";
            s_file = new SuccinctSemistructuredShard(filename);

            std::cout << "Serializing Succinct data structures...\n";
            s_file->Serialize(filename + ".succinct");
        } else {
            // If mode is set to 1, read the serialized data structures from disk.
            // The serialized data structures must exist at <filename>.succinct.
            std::cout << "De-serializing Succinct data structures...\n";
            s_file = new SuccinctSemistructuredShard(filename,
                                                    SuccinctMode::LOAD_IN_MEMORY);
        }
        std::cout << "Done. Starting Succinct Shell...\n";
        print_valid_cmds();
    }

    //QuerySemistructured members
    SuccinctSemistructuredShard *s_file;

    //Wrapped search command
    void search(const std::string attr_key, const std::string attr_val){
        std::set<int64_t> results;
        timestamp_t start = get_timestamp();
        s_file->SearchAttribute(results, attr_key, attr_val);
        timestamp_t tot_time = get_timestamp() - start;
        std::cout << "Found " << results.size() << " records in " << tot_time
                    << "us; Matching keys:\n";
        for (auto res : results) {
            std::cout << res << ", ";
        }
        std::cout << std::endl;
    }

    //Wrapped count command
    void count(const std::string attr_key, const std::string attr_val){
      timestamp_t start = get_timestamp();
      int64_t count = s_file->CountAttribute(attr_key, attr_val);
      timestamp_t tot_time = get_timestamp() - start;
      std::cout << "Number of matching records = " << count << "; Time taken: "
                << tot_time << "us\n";
    }

    //Wrapped get command
    void get(int64_t key, std::string attr_key){
      timestamp_t start = get_timestamp();
      std::string result;
      s_file->Get(result, key, attr_key);
      timestamp_t tot_time = get_timestamp() - start;
      std::cout << "Value = " << result << "; Time taken: " << tot_time
                << "us\n";
    }

};

BOOST_PYTHON_MODULE(pyquery_semistructured){
    class_<QuerySemistructured>("QuerySemistructured", init<std::string, uint32_t>())
    .def("search", &QuerySemistructured::search)
    .def("count", &QuerySemistructured::count)
    .def("get", &QuerySemistructured::get)
    ;

}


