#ifndef SUCCINCT_ALLOCATOR_H
#define SUCCINCT_ALLOCATOR_H

#include <cstddef>
#include <cstdlib>

class SuccinctAllocator {

private:
    bool s_use_hugepages;

public:
    /*
     * Constructor
     *
     */
    SuccinctAllocator(bool s_use_hugepages = false);

    /*
     * Enable the use of huge pages.
     *
     */
    bool use_hugepages();

    /*
     * Allocates a block of size bytes of memory, returning a pointer to the
     * beginning of the block.
     *
     */
    void* s_malloc(size_t size);

    /*
     * Allocates a block of memory for an array of num elements, each of them
     * size bytes long, and initializes all its bits to zero.
     *
     */
    void* s_calloc(size_t num, size_t size);

    /*
     * Changes the size of the memory block pointed to by ptr.
     *
     */
    void* s_realloc(void* ptr, size_t size);

    /*
     * A block of memory previously allocated by a call to s_malloc, s_calloc or
     * s_realloc is deallocated, making it available again for further
     * allocations.
     *
     */
    void s_free(void* ptr);

};
#endif


