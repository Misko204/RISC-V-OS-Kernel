// main.cpp - kernel entry point

#include "../h/riscv.hpp"
#include "../h/kprint.hpp"
#include "../h/memory_allocator.hpp"
#include "../h/trap.hpp"
#include "../test/tests.hpp"

int main() {
    kprintString("Kernel started\n");

    MemoryAllocator::init();
    Trap::init();

    int failures = 0;
    failures += testMemoryAllocator();
    failures += testSystemCalls();

    kprintString(failures == 0 ? "\nAll tests passed\n" : "\nSOME TESTS FAILED\n");

    Riscv::haltEmulator();
    return 0;
}
