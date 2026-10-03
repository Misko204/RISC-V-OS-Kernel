// syscall_c.hpp - C API of the kernel (user side)

#ifndef _syscall_c_hpp_
#define _syscall_c_hpp_

#include "../lib/hw.h"

// Allocates at least `size` bytes, rounded up to whole MEM_BLOCK_SIZE blocks.
// Returns nullptr on failure.
void* mem_alloc(size_t size);

// Frees memory returned by mem_alloc. Returns 0 on success, a negative error code otherwise.
int mem_free(void* ptr);

// Total free memory and the largest free fragment, in bytes.
size_t mem_get_free_space();
size_t mem_get_largest_free_block();

#endif // _syscall_c_hpp_
