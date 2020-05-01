#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <sstream>
#include <sys/time.h>

#include "succinct_file.h"

#include <boost/python.hpp>
using namespace boost::python;

/**
 * Prints usage.
 */
void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-m mode] [file]\n", exec);
}

void print_valid_cmds() {
  std::cerr
      << "Command must be one of: search [query], count [query], extract [offset] [length]\n";
}

typedef unsigned long long int timestamp_t;

static timestamp_t get_timestamp() {
  struct timeval now{};
  gettimeofday(&now, nullptr);

  return (now.tv_usec + (time_t) now.tv_sec * 1000000);
}

struct QueryFile {
    QueryFile(std::string filename, uint32_t mode){
        this->s_file = nullptr;
        if (mode == 0) {
            // If mode is set to 0, compress the input file.
            // Use default parameters.
            std::cout << "Constructing Succinct data structures...\n";
            this->s_file = new SuccinctFile(filename);
            std::cout << "Serializing Succinct data structures...\n";
            this->s_file->Serialize(filename + ".succinct");
        } else {
            // If mode is set to 1, read the serialized data structures from disk.
            // The serialized data structures must exist at <filename>.succinct.
            std::cout << "De-serializing Succinct data structures...\n";
            this->s_file = new SuccinctFile(filename, SuccinctMode::LOAD_IN_MEMORY);
        }
        std::cout << "Done. Starting Succinct Shell...\n";
        print_valid_cmds();
    }

    //QueryFile members
    SuccinctFile *s_file;

    //Wrapped search command
    void search(std::string arg){
        std::vector<int64_t> results;
        timestamp_t start = get_timestamp();
        s_file->Search(results, arg);
        timestamp_t tot_time = get_timestamp() - start;
        std::cout << "Found " << results.size() << " results in " << tot_time
                    << "us:\n";
        for (auto res : results) {
            std::cout << res << ", ";
        }
        std::cout << std::endl;
    }

    //Wrapped count command
    void count(std::string arg){
        timestamp_t start = get_timestamp();
        int64_t count = s_file->Count(arg);
        timestamp_t tot_time = get_timestamp() - start;
        std::cout << "Count = " << count << "; Time taken: " << tot_time
                    << "us\n";
    }

    //Wrapped extract command
    void extract(uint64_t offset, uint64_t length){
        timestamp_t start = get_timestamp();
        std::string result;
        s_file->Extract(result, offset, length);
        timestamp_t tot_time = get_timestamp() - start;
        std::cout << "Extracted string = " << result << "; Time taken: "
                    << tot_time << "us\n";
    }
};

BOOST_PYTHON_MODULE(pyquery_file){
    class_<QueryFile>("QueryFile", init<std::string, uint32_t>())
    .def("search", &QueryFile::search)
    .def("count", &QueryFile::count)
    .def("extract", &QueryFile::extract)
    ;

}