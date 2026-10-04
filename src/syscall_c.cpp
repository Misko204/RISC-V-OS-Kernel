// syscall_c.cpp - C API of the kernel (user side)
// Every function packs its arguments into registers and executes ecall.

#include "../h/syscall_c.hpp"
#include "../h/syscall_codes.hpp"

// Performs a system call: a0 = code, a1..a4 = arguments; the result comes back in a0.
static uint64 syscall(uint64 code, uint64 arg1 = 0, uint64 arg2 = 0,
                      uint64 arg3 = 0, uint64 arg4 = 0) {
    register uint64 a0 __asm__("a0") = code;
    register uint64 a1 __asm__("a1") = arg1;
    register uint64 a2 __asm__("a2") = arg2;
    register uint64 a3 __asm__("a3") = arg3;
    register uint64 a4 __asm__("a4") = arg4;
    __asm__ volatile ("ecall"
    : "+r"(a0)
    : "r"(a1), "r"(a2), "r"(a3), "r"(a4)
    : "memory");
    return a0;
}

// ---------------------------------------------------------------- memory

void* mem_alloc(size_t size) {
    // The ABI takes the size in blocks. Written this way to avoid overflow for huge sizes.
    size_t blocks = size / MEM_BLOCK_SIZE + (size % MEM_BLOCK_SIZE != 0 ? 1 : 0);
    return (void*)syscall(SYS_MEM_ALLOC, blocks);
}

int mem_free(void* ptr) {
    return (int)syscall(SYS_MEM_FREE, (uint64)ptr);
}

size_t mem_get_free_space() {
    return (size_t)syscall(SYS_MEM_GET_FREE_SPACE);
}

size_t mem_get_largest_free_block() {
    return (size_t)syscall(SYS_MEM_GET_LARGEST_FREE_BLOCK);
}

// ---------------------------------------------------------------- threads

int thread_create(thread_t* handle, void (*start_routine)(void*), void* arg) {
    // The ABI expects the caller to provide the stack: allocate it here and
    // pass a pointer just past its end (the stack grows downwards).
    void* stack = mem_alloc(DEFAULT_STACK_SIZE);
    if (stack == nullptr) return (int)ERR_OUT_OF_MEMORY;

    int result = (int)syscall(SYS_THREAD_CREATE, (uint64)handle, (uint64)start_routine,
                              (uint64)arg, (uint64)stack + DEFAULT_STACK_SIZE);
    if (result < 0) mem_free(stack);    // on success the kernel owns the stack
    return result;
}

int thread_exit() {
    return (int)syscall(SYS_THREAD_EXIT);
}

void thread_dispatch() {
    syscall(SYS_THREAD_DISPATCH);
}

// ---------------------------------------------------------------- semaphores

int sem_open(sem_t* handle, unsigned init) {
    return (int)syscall(SYS_SEM_OPEN, (uint64)handle, init);
}

int sem_close(sem_t handle) {
    return (int)syscall(SYS_SEM_CLOSE, (uint64)handle);
}

int sem_wait(sem_t id) {
    return (int)syscall(SYS_SEM_WAIT, (uint64)id);
}

int sem_signal(sem_t id) {
    return (int)syscall(SYS_SEM_SIGNAL, (uint64)id);
}

// ---------------------------------------------------------------- console

void putc(char c) {
    syscall(SYS_PUTC, (uint64)(unsigned char)c);
}
