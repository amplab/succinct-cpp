#include "succinct/utils/SuccinctUtils.hpp"

// Returns the number of set bits in a 64 bit integer
uint64_t SuccinctUtils::popcount(uint64_t n) {
    // TODO: Add support for hardware instruction
    n -= (n >> 1) & m1;
    n = (n & m2) + ((n >> 2) & m2);
    n = (n + (n >> 4)) & m4;
    return (n * h01) >> 56;
}

// Returns integer logarithm to the base 2
uint32_t SuccinctUtils::int_log_2(uint64_t n) {
    // TODO: Add support for hardware instruction
    uint32_t l = ISPOWOF2(n) ? 0 : 1;
    while (n >>= 1) ++l;
    return l;
}

// Returns a modulo n
uint64_t SuccinctUtils::modulo(int64_t a, uint64_t n) {
    while (a < 0)
        a += n;
    return a % n;
}

// Memory map a file and return mapped buffer
void* SuccinctUtils::memory_map(std::string filename) {
    struct stat st;
    stat(filename.c_str(), &st);

    int fd = open(filename.c_str(), O_RDONLY, 0);
    assert(fd != -1);

    // Try mapping with huge-pages support
    void *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_HUGETLB | MAP_POPULATE, fd, 0);

    // Revert to mapping with huge page support in case mapping fails
    if(data == (void *)-1) {
        fprintf(stderr, "mmap with MAP_HUGETLB option failed; trying without MAP_HUGETLB flag...\n");
        data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    }
    madvise(data, st.st_size, POSIX_MADV_RANDOM);
    assert(data != (void *)-1);

    return data;
}
