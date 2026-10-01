// main.cpp - FAZA 0: provera okruzenja

#include "../h/riscv.hpp"
#include "../h/kprint.hpp"

int main() {
    kprintString("Jezgro pokrenuto!\n");

    kprintString("sstatus = ");
    kprintUInt(Riscv::r_sstatus(), 16);
    kprintString("\n");

    uint64 heapStart = (uint64)HEAP_START_ADDR;
    uint64 heapEnd   = (uint64)HEAP_END_ADDR;
    uint64 heapSize  = heapEnd - heapStart;

    kprintString("HEAP: ");
    kprintUInt(heapStart, 16);
    kprintString(" - ");
    kprintUInt(heapEnd, 16);
    kprintString("\n");

    kprintString("Velicina heap-a: ");
    kprintUInt(heapSize);
    kprintString(" B, blokova: ");
    kprintUInt(heapSize / MEM_BLOCK_SIZE);
    kprintString("\n");

    kprintString("MEM_BLOCK_SIZE = ");     kprintUInt(MEM_BLOCK_SIZE);     kprintString("\n");
    kprintString("DEFAULT_STACK_SIZE = "); kprintUInt(DEFAULT_STACK_SIZE); kprintString("\n");
    kprintString("DEFAULT_TIME_SLICE = "); kprintUInt(DEFAULT_TIME_SLICE); kprintString("\n");

    Riscv::haltEmulator();
    return 0;
}