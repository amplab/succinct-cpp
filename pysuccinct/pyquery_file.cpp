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
 * Program that wraps succinct's query file functions for python use via boost
 */

struct QueryFile {
    QueryFile(std::string filename, uint32_t mode) {
        this->s_file_ = nullptr;
        if (mode == 0) {
            // If mode is set to 0, compress the input file.
            // Use default parameters.
            std::cout << "Constructing Succinct data structures...\n";
            this->s_file_ = new SuccinctFile(filename);
            std::cout << "Serializing Succinct data structures...\n";
            this->s_file_->Serialize(filename + ".succinct");
        } else {
            // If mode is set to 1, read the serialized data structures from disk.
            // The serialized data structures must exist at <filename>.succinct.
            std::cout << "De-serializing Succinct data structures...\n";
            this->s_file_ = new SuccinctFile(filename, SuccinctMode::LOAD_IN_MEMORY);
        }
    }

    //Wrapped search command
    std::vector<int64_t> Search(const std::string& arg) {
        std::vector<int64_t> results;
        s_file_->Search(results, arg);
        return results;
    }

    //Wrapped count command
    int64_t Count(const std::string& arg) {
        int64_t count = s_file_->Count(arg);
        return count;
    }

    //Wrapped extract command
    std::string Extract(uint64_t offset, uint64_t length) {
        std::string result;
        s_file_->Extract(result, offset, length);
        return result;
    }

    //QueryFile members
    SuccinctFile *s_file_;
};

//Boost Python module
BOOST_PYTHON_MODULE(pyquery_file){
    class_<QueryFile>("QueryFile", init<std::string, uint32_t>())
    .def("Search", &QueryFile::Search)
    .def("Count", &QueryFile::Count)
    .def("Extract", &QueryFile::Extract)
    ;

}