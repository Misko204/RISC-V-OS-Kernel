// main.cpp - kernel entry point

#include "../h/riscv.hpp"
#include "../h/kprint.hpp"
#include "../h/memory_allocator.hpp"
#include "../test/tests.hpp"

int main() {
    kprintString("Kernel started\n");

    MemoryAllocator::init();
    kprintString("Heap: ");
    kprintUInt(MemoryAllocator::freeSpace());
    kprintString(" B free\n");

    testMemoryAllocator();

    Riscv::haltEmulator();
    return 0;
}
