#ifndef SUCCINCT_H
#define SUCCINCT_H

#include <cstdint>
#include <string>
#include <cstring>
#include <vector>

#include "SuccinctCore.hpp"

class SuccinctFile : public SuccinctCore {
private:
    std::string input_filename;
    std::string succinct_filename;

public:
    SuccinctFile(std::string filename, SuccinctMode s_mode = SuccinctMode::CONSTRUCT_IN_MEMORY);

    std::string name();
    /*
     * Random access into the Succinct file with the specified offset
     * and length
     */
    void extract(std::string& result, uint64_t offset, uint64_t len);

    /*
     * Extract character at specified position
     */
    char char_at(uint64_t pos);

    /*
     * Get the count of a string in the Succinct file
     */
    uint64_t count(std::string str);

    /*
     * Get the offsets of all the occurrences
     * of a string in the Succinct file
     */
    void search(std::vector<int64_t>& result, std::string str);

    /*
     * Get the offsets of all the occurrences of a
     * wild-card string in the Succinct file
     */
    void wildcard_search(std::vector<int64_t>& result, std::string pattern,
                                            uint64_t max_sep);

private:
    std::pair<int64_t, int64_t> get_range_slow(const char *str, uint64_t len);
    std::pair<int64_t, int64_t> get_range(const char *str, uint64_t len);

    uint64_t compute_context_value(const char *str, uint64_t pos);

};

#endif
