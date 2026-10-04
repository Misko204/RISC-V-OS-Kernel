// ksemaphore.cpp - counting semaphore implemented in the kernel

#include "../h/ksemaphore.hpp"
#include "../h/tcb.hpp"
#include "../h/memory_allocator.hpp"
#include "../h/syscall_codes.hpp"

KSemaphore* KSemaphore::create(unsigned initialValue) {
    return new KSemaphore(initialValue);    // nullptr if out of memory
}

KSemaphore* KSemaphore::fromHandle(void* handle) {
    KSemaphore* sem = (KSemaphore*)handle;
    if (sem == nullptr || sem->magic != MAGIC) return nullptr;
    return sem;
}

int KSemaphore::wait() {
    if (--val >= 0) return 0;

    blocked.put(TCB::running());
    return TCB::block();        // returns when signal() or close() unblocks us
}

void KSemaphore::signal() {
    if (++val <= 0) {
        TCB::unblock(blocked.get(), 0);
    }
}

void KSemaphore::close() {
    while (!blocked.isEmpty()) {
        TCB::unblock(blocked.get(), (int)ERR_SEMAPHORE_CLOSED);
    }
    delete this;
}

void* KSemaphore::operator new(size_t size) noexcept {
    return MemoryAllocator::alloc(MemoryAllocator::bytesToBlocks(size));
}

void KSemaphore::operator delete(void* ptr) noexcept {
    MemoryAllocator::free(ptr);
}
