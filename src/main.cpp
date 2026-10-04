// main.cpp - kernel entry point

#include "../h/riscv.hpp"
#include "../h/kprint.hpp"
#include "../h/memory_allocator.hpp"
#include "../h/trap.hpp"
#include "../h/tcb.hpp"
#include "../h/ksemaphore.hpp"
#include "../h/syscall_c.hpp"
#include "../test/tests.hpp"

// The user program's entry point.
void userMain();

// Runs in user mode as the first user thread.
static void userMainWrapper(void*) {
    userMain();
}

int main() {
    kprintString("Kernel started\n");

    MemoryAllocator::init();
    Trap::init();

    // Kernel-mode self-tests (run before any thread exists).
    int failures = 0;
    failures += testMemoryAllocator();
    failures += testSystemCalls();
    kprintString(failures == 0 ? "Kernel-mode tests passed\n" : "SOME KERNEL-MODE TESTS FAILED\n");

    // main becomes a kernel thread; start the user program as a user thread.
    TCB::init();
    KSemaphore* userThreadsDone = KSemaphore::create(0);
    TCB::setUserThreadsDoneSemaphore(userThreadsDone);

    thread_t userMainThread;
    if (userThreadsDone == nullptr ||
        thread_create(&userMainThread, userMainWrapper, nullptr) != 0) {
        kprintString("Failed to start userMain\n");
        Riscv::haltEmulator();
    }

    // Sleep until the last user thread finishes. The wait goes through the
    // system call so that main blocks inside a trap, like any other thread.
    sem_wait((sem_t)userThreadsDone);

    kprintString("\nAll user threads finished, shutting down\n");
    Riscv::haltEmulator();
    return 0;
}
