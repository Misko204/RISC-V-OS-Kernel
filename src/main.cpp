// main.cpp - kernel entry point (currently a smoke test of the environment)

#include "../h/riscv.hpp"
#include "../h/kprint.hpp"

int main() {
    kprintString("Kernel started\n");

    kprintString("sstatus = ");
    kprintUInt(Riscv::r_sstatus(), 16);
    kprintString("\n");

    uint64 heapStart = (uint64)HEAP_START_ADDR;
    uint64 heapEnd   = (uint64)HEAP_END_ADDR;
    uint64 heapSize  = heapEnd - heapStart;

    kprintString("Heap: ");
    kprintUInt(heapStart, 16);
    kprintString(" - ");
    kprintUInt(heapEnd, 16);
    kprintString("\n");

    kprintString("Heap size: ");
    kprintUInt(heapSize);
    kprintString(" B, blocks: ");
    kprintUInt(heapSize / MEM_BLOCK_SIZE);
    kprintString("\n");

    kprintString("MEM_BLOCK_SIZE = ");     kprintUInt(MEM_BLOCK_SIZE);     kprintString("\n");
    kprintString("DEFAULT_STACK_SIZE = "); kprintUInt(DEFAULT_STACK_SIZE); kprintString("\n");
    kprintString("DEFAULT_TIME_SLICE = "); kprintUInt(DEFAULT_TIME_SLICE); kprintString("\n");

    Riscv::haltEmulator();
    return 0;
}