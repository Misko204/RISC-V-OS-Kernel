// ksemaphore.hpp - counting semaphore implemented in the kernel

#ifndef _ksemaphore_hpp_
#define _ksemaphore_hpp_

#include "../lib/hw.h"
#include "thread_queue.hpp"

class KSemaphore {
public:
    // Returns nullptr if the kernel is out of memory.
    static KSemaphore* create(unsigned initialValue);

    // Returns the semaphore behind a handle passed in from user code, or
    // nullptr if the handle is null or does not point to a live semaphore.
    static KSemaphore* fromHandle(void* handle);

    // Decrements the value; blocks the running thread if it becomes negative.
    // Returns 0, or ERR_SEMAPHORE_CLOSED if the semaphore was closed while
    // the thread was waiting.
    int wait();

    // Increments the value; unblocks the longest-waiting thread, if any.
    void signal();

    // Unblocks every waiting thread (their wait() fails) and destroys the semaphore.
    void close();

    int value() const { return val; }

    static void* operator new(size_t size) noexcept;
    static void operator delete(void* ptr) noexcept;

private:
    explicit KSemaphore(unsigned initialValue) noexcept
        : magic(MAGIC), val((int)initialValue), blocked() { }
    ~KSemaphore() { magic = 0; }

    static constexpr uint64 MAGIC = 0x5E4A9408E5E4A940UL;

    uint64      magic;      // MAGIC while the semaphore exists
    int         val;        // when negative, -val threads are waiting
    ThreadQueue blocked;    // waiting threads, in arrival order
};

#endif // _ksemaphore_hpp_
