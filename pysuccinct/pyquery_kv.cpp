#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <sys/time.h>
#include <sstream>

#include "succinct_shard.h"

#include <boost/python.hpp>
using namespace boost::python;

/**
 * Program that wraps succinct's query kv functions for python use via boost
 */

struct QueryKv{
    QueryKv(std::string filename, uint32_t mode) {
        this->s_file_ = nullptr;
        if (mode == 0) {
            // If mode is set to 0, compress the input file.
            // Use default parameters.
            std::cout << "Constructing Succinct data structures...\n";
            s_file_ = new SuccinctShard(0, filename);

            std::cout << "Serializing Succinct data structures...\n";
            s_file_->Serialize(filename + ".succinct");
        } else {
            // If mode is set to 1, read the serialized data structures from disk.
            // The serialized data structures must exist at <filename>.succinct.
            std::cout << "De-serializing Succinct data structures...\n";
            s_file_ = new SuccinctShard(0, filename, SuccinctMode::LOAD_IN_MEMORY);
        }
    }

    //Wrapped search command
    std::set<int64_t> Search(const std::string &arg) {
        std::set<int64_t> results;
        s_file_->Search(results, arg);
        return results;
    }

    //Wrapped count command
    int64_t Count(const std::string &arg) {
        int64_t count = s_file_->Count(arg);
        return count;
    }

    //Wrapped get command
    std::string Get(uint64_t key) {
        std::string result;
        s_file_->Get(result, key);
        return result;
    }

    //QueryKv members
    SuccinctShard *s_file_;

};

//Boost Python module
BOOST_PYTHON_MODULE(pyquery_kv){
    class_<QueryKv>("QueryKv", init<std::string, uint32_t>())
    .def("Search", &QueryKv::Search)
    .def("Count", &QueryKv::Count)
    .def("Get", &QueryKv::Get)
    ;

}


