#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <sys/time.h>

#include "succinct_semistructured_shard.h"

#include <boost/python.hpp>
using namespace boost::python;

/**
 * Program that wraps succinct's query semistructured functions for python use via boost
 */

struct QuerySemistructured{
    QuerySemistructured(std::string filename, uint32_t mode) {
        this->s_file_ = nullptr;
        if (mode == 0) {
            // If mode is set to 0, compress the input file.
            // Use default parameters.
            std::cout << "Constructing Succinct data structures...\n";
            s_file_ = new SuccinctSemistructuredShard(filename);

            std::cout << "Serializing Succinct data structures...\n";
            s_file_->Serialize(filename + ".succinct");
        } else {
            // If mode is set to 1, read the serialized data structures from disk.
            // The serialized data structures must exist at <filename>.succinct.
            std::cout << "De-serializing Succinct data structures...\n";
            s_file_ = new SuccinctSemistructuredShard(filename,
                                                    SuccinctMode::LOAD_IN_MEMORY);
        }
    }

    //Wrapped search command
    std::set<int64_t> Search(const std::string &attr_key, const std::string &attr_val) {
        std::set<int64_t> results;
        s_file_->SearchAttribute(results, attr_key, attr_val);
        return results;
    }

    //Wrapped count command
    int64_t Count(const std::string &attr_key, const std::string &attr_val) {
      int64_t count = s_file_->CountAttribute(attr_key, attr_val);
      return count;
    }

    //Wrapped get command
    std::string Get(int64_t key, std::string attr_key) {
      std::string result;
      s_file_->Get(result, key, attr_key);
      return result;
    }

    //QuerySemistructured members
    SuccinctSemistructuredShard *s_file_;

};

//Boost Python module
BOOST_PYTHON_MODULE(pyquery_semistructured){
    class_<QuerySemistructured>("QuerySemistructured", init<std::string, uint32_t>())
    .def("Search", &QuerySemistructured::Search)
    .def("Count", &QuerySemistructured::Count)
    .def("Get", &QuerySemistructured::Get)
    ;

}


