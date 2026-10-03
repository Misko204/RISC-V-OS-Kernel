// main.cpp - kernel entry point

#include "../h/riscv.hpp"
#include "../h/kprint.hpp"
#include "../h/memory_allocator.hpp"
#include "../h/trap.hpp"
#include "../h/tcb.hpp"
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
    thread_t userMainThread;
    if (thread_create(&userMainThread, userMainWrapper, nullptr) != 0) {
        kprintString("Failed to start userMain\n");
        Riscv::haltEmulator();
    }

    // Keep yielding until every user thread has finished.
    while (TCB::liveUserThreads() > 0) {
        thread_dispatch();
    }

    kprintString("\nAll user threads finished, shutting down\n");
    Riscv::haltEmulator();
    return 0;
}
