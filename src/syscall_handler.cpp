// syscall_handler.cpp - kernel side of the system call ABI

#include "../h/syscall_handler.hpp"
#include "../h/syscall_codes.hpp"
#include "../h/trap.hpp"
#include "../h/memory_allocator.hpp"

uint64 SyscallHandler::dispatch(TrapFrame* frame) {
    switch (frame->x[TrapFrame::A0]) {
        case SYS_MEM_ALLOC:                  return memAlloc(frame);
        case SYS_MEM_FREE:                   return memFree(frame);
        case SYS_MEM_GET_FREE_SPACE:         return memGetFreeSpace(frame);
        case SYS_MEM_GET_LARGEST_FREE_BLOCK: return memGetLargestFreeBlock(frame);
        default:                             return (uint64)ERR_UNKNOWN_SYSCALL;
    }
}

// a1 = size in blocks; returns the address or 0.
uint64 SyscallHandler::memAlloc(TrapFrame* frame) {
    size_t blocks = frame->x[TrapFrame::A1];
    return (uint64)MemoryAllocator::alloc(blocks);
}

// a1 = address returned by mem_alloc; returns 0 or a negative error code.
uint64 SyscallHandler::memFree(TrapFrame* frame) {
    void* ptr = (void*)frame->x[TrapFrame::A1];
    return fromInt(MemoryAllocator::free(ptr));
}

uint64 SyscallHandler::memGetFreeSpace(TrapFrame*) {
    return MemoryAllocator::freeSpace();
}

uint64 SyscallHandler::memGetLargestFreeBlock(TrapFrame*) {
    return MemoryAllocator::largestFreeBlock();
}
