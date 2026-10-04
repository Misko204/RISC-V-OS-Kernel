// syscall_handler.cpp - kernel side of the system call ABI

#include "../h/syscall_handler.hpp"
#include "../h/syscall_codes.hpp"
#include "../h/syscall_c.hpp"
#include "../h/trap.hpp"
#include "../h/memory_allocator.hpp"
#include "../h/tcb.hpp"
#include "../h/ksemaphore.hpp"
#include "../h/kprint.hpp"
#include "../h/timer.hpp"

uint64 SyscallHandler::dispatch(TrapFrame* frame) {
    switch (frame->x[TrapFrame::A0]) {
        case SYS_MEM_ALLOC:                  return memAlloc(frame);
        case SYS_MEM_FREE:                   return memFree(frame);
        case SYS_MEM_GET_FREE_SPACE:         return memGetFreeSpace(frame);
        case SYS_MEM_GET_LARGEST_FREE_BLOCK: return memGetLargestFreeBlock(frame);
        case SYS_THREAD_CREATE:              return threadCreate(frame);
        case SYS_THREAD_EXIT:                return threadExit(frame);
        case SYS_THREAD_DISPATCH:            return threadDispatch(frame);
        case SYS_SEM_OPEN:                   return semOpen(frame);
        case SYS_SEM_CLOSE:                  return semClose(frame);
        case SYS_SEM_WAIT:                   return semWait(frame);
        case SYS_SEM_SIGNAL:                 return semSignal(frame);
        case SYS_TIME_SLEEP:                 return timeSleep(frame);
        case SYS_PUTC:                       return putc(frame);
        default:                             return fromInt(ERR_UNKNOWN_SYSCALL);
    }
}

// ---------------------------------------------------------------- memory

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

// ---------------------------------------------------------------- threads

// a1 = thread_t* handle, a2 = start routine, a3 = argument,
// a4 = top of a DEFAULT_STACK_SIZE-byte user stack allocated by the caller.
uint64 SyscallHandler::threadCreate(TrapFrame* frame) {
    thread_t* handle  = (thread_t*)frame->x[TrapFrame::A1];
    TCB::Body body    = (TCB::Body)frame->x[TrapFrame::A2];
    void* arg         = (void*)frame->x[TrapFrame::A3];
    void* stackTop    = (void*)frame->x[TrapFrame::A4];

    if (handle == nullptr || body == nullptr || stackTop == nullptr) {
        return fromInt(ERR_INVALID_ARGUMENT);
    }

    TCB* thread = TCB::createUserThread(body, arg, stackTop);
    if (thread == nullptr) return fromInt(ERR_OUT_OF_MEMORY);

    *handle = (thread_t)thread;
    return 0;
}

uint64 SyscallHandler::threadExit(TrapFrame*) {
    TCB::exit();
    return 0;   // never reached: exit() does not return
}

uint64 SyscallHandler::threadDispatch(TrapFrame*) {
    TCB::dispatch();
    return 0;
}

// ---------------------------------------------------------------- semaphores

// a1 = sem_t* handle, a2 = initial value.
uint64 SyscallHandler::semOpen(TrapFrame* frame) {
    sem_t* handle     = (sem_t*)frame->x[TrapFrame::A1];
    unsigned initial  = (unsigned)frame->x[TrapFrame::A2];
    if (handle == nullptr) return fromInt(ERR_INVALID_ARGUMENT);

    KSemaphore* sem = KSemaphore::create(initial);
    if (sem == nullptr) return fromInt(ERR_OUT_OF_MEMORY);

    *handle = (sem_t)sem;
    return 0;
}

// a1 = sem_t. Waiting threads are released with an error.
uint64 SyscallHandler::semClose(TrapFrame* frame) {
    KSemaphore* sem = KSemaphore::fromHandle((void*)frame->x[TrapFrame::A1]);
    if (sem == nullptr) return fromInt(ERR_INVALID_ARGUMENT);
    sem->close();
    return 0;
}

// a1 = sem_t. May block; returns ERR_SEMAPHORE_CLOSED if closed while waiting.
uint64 SyscallHandler::semWait(TrapFrame* frame) {
    KSemaphore* sem = KSemaphore::fromHandle((void*)frame->x[TrapFrame::A1]);
    if (sem == nullptr) return fromInt(ERR_INVALID_ARGUMENT);
    return fromInt(sem->wait());
}

// a1 = sem_t.
uint64 SyscallHandler::semSignal(TrapFrame* frame) {
    KSemaphore* sem = KSemaphore::fromHandle((void*)frame->x[TrapFrame::A1]);
    if (sem == nullptr) return fromInt(ERR_INVALID_ARGUMENT);
    sem->signal();
    return 0;
}

// ---------------------------------------------------------------- time

// a1 = number of timer ticks to sleep.
uint64 SyscallHandler::timeSleep(TrapFrame* frame) {
    time_t ticks = (time_t)frame->x[TrapFrame::A1];
    return fromInt(Timer::sleep(ticks));
}

// ---------------------------------------------------------------- console

// a1 = character. Temporary: synchronous output through console.lib.
uint64 SyscallHandler::putc(TrapFrame* frame) {
    kprintChar((char)frame->x[TrapFrame::A1]);
    return 0;
}
