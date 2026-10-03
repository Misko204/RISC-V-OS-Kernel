// syscall_c.hpp - C API of the kernel (user side)

#ifndef _syscall_c_hpp_
#define _syscall_c_hpp_

#include "../lib/hw.h"

// ---------------------------------------------------------------- memory

// Allocates at least `size` bytes, rounded up to whole MEM_BLOCK_SIZE blocks.
// Returns nullptr on failure.
void* mem_alloc(size_t size);

// Frees memory returned by mem_alloc. Returns 0 on success, a negative error code otherwise.
int mem_free(void* ptr);

// Total free memory and the largest free fragment, in bytes.
size_t mem_get_free_space();
size_t mem_get_largest_free_block();

// ---------------------------------------------------------------- threads

class _thread;              // opaque: the kernel's thread object
typedef _thread* thread_t;

// Starts a thread that runs start_routine(arg), with a DEFAULT_STACK_SIZE stack.
// On success stores the handle in *handle and returns 0; returns a negative error code otherwise.
int thread_create(thread_t* handle, void (*start_routine)(void*), void* arg);

// Terminates the calling thread. Returns only on failure, with a negative error code.
int thread_exit();

// Gives the CPU to another ready thread (or back to the caller if there is none).
void thread_dispatch();

// ---------------------------------------------------------------- console

// Writes one character to the console.
void putc(char c);

#endif // _syscall_c_hpp_
