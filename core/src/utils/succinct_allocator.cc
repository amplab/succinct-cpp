#include "utils/succinct_allocator.h"

/*
 * Constructor
 *
 */
SuccinctAllocator::SuccinctAllocator(bool s_use_hugepages) {
  this->use_hugepages_ = s_use_hugepages;
}

/*
 * Enable the use of huge pages.
 *
 */
bool SuccinctAllocator::UseHugePages() {
  use_hugepages_ = false;
  return use_hugepages_;
}

/*
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.
 *
 */
void* SuccinctAllocator::s_malloc(size_t size) {
  if (use_hugepages_) {
    return 0;
  }
  return malloc(size);
}

/*
 * Allocates a block of memory for an array of num elements, each of them
 * size bytes long, and initializes all its bits to zero.
 *
 */
void* SuccinctAllocator::s_calloc(size_t num, size_t size) {
  if (use_hugepages_) {
    return 0;
  }
  return calloc(num, size);
}

/*
 * Changes the size of the memory block pointed to by ptr.
 *
 */
void* SuccinctAllocator::s_realloc(void* ptr, size_t size) {
  if (use_hugepages_) {
    return 0;
  }
  return realloc(ptr, size);
}

/*
 * A block of memory previously allocated by a call to s_malloc, s_calloc or
 * s_realloc is deallocated, making it available again for further
 * allocations.
 *
 */
void SuccinctAllocator::s_free(void* ptr) {
  if (use_hugepages_) {
    return;
  }
  free(ptr);
}

/*
 * Sets the first num bytes of the block of memory pointed to by ptr to the
 * specified value (interpreted as unsigned char).
 *
 */
void *SuccinctAllocator::s_memset(void *ptr, int value, size_t num) {
  if (use_hugepages_) {
    return 0;
  }
  return memset(ptr, value, num);
}
