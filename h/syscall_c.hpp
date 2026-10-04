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

// ---------------------------------------------------------------- semaphores

class _sem;                 // opaque: the kernel's semaphore object
typedef _sem* sem_t;

// Creates a semaphore with the given initial value and stores its handle in *handle.
// Returns 0 on success, a negative error code otherwise.
int sem_open(sem_t* handle, unsigned init);

// Destroys the semaphore. Threads waiting on it are released and their sem_wait fails.
int sem_close(sem_t handle);

// Waits on the semaphore. Returns 0, or a negative error code (also when the
// semaphore is closed while the caller is waiting).
int sem_wait(sem_t id);

// Signals the semaphore. Returns 0, or a negative error code.
int sem_signal(sem_t id);

// ---------------------------------------------------------------- time

// Suspends the calling thread for `ticks` timer periods (10 ticks per second).
// Returns 0 on success, a negative error code otherwise.
int time_sleep(time_t ticks);

// ---------------------------------------------------------------- console

// Writes one character to the console.
void putc(char c);

#endif // _syscall_c_hpp_
