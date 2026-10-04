// kprint.cpp - kernel debug output

#include "../h/kprint.hpp"
#include "../h/riscv.hpp"
#include "../lib/console.h"

// Prints a single character with interrupts masked.
//
// When the UART's transmit buffer is full, __putc (console.lib) re-enables
// interrupts and waits for the console interrupt to drain it. The timer
// interrupt source is masked in sie for that time, so the wait can never be
// preempted by a timer tick. This matters when kprintChar runs inside the
// trap handler (e.g. for the putc system call): the kernel is not re-entrant.
void kprintChar(char c) {
    uint64 oldSstatus = Riscv::r_sstatus();
    Riscv::mc_sstatus(Riscv::SSTATUS_SIE);          // disable interrupts
    uint64 oldSie = Riscv::r_sie();
    Riscv::mc_sie(Riscv::SIE_SSIE);                 // keep the timer out while __putc waits

    __putc(c);

    if (oldSie & Riscv::SIE_SSIE) Riscv::ms_sie(Riscv::SIE_SSIE);
    if (oldSstatus & Riscv::SSTATUS_SIE) {          // re-enable only if they were enabled
        Riscv::ms_sstatus(Riscv::SSTATUS_SIE);
    } else {
        Riscv::mc_sstatus(Riscv::SSTATUS_SIE);      // __putc may have left them enabled
    }
}

static void kputc(char c) {
    kprintChar(c);
}

void kprintString(const char* s) {
    while (*s != '\0') {
        kputc(*s);
        s++;
    }
}

void kprintUInt(uint64 x, uint64 base) {
    static const char digits[] = "0123456789ABCDEF";
    char buffer[24];                                // 2^64 has at most 20 decimal digits
    int count = 0;

    do {
        buffer[count++] = digits[x % base];
        x /= base;
    } while (x != 0);

    if (base == 16) kprintString("0x");

    while (count > 0) {
        kputc(buffer[--count]);                     // digits were stored in reverse order
    }
}

void kprintInt(long x) {
    if (x < 0) {
        kputc('-');
        kprintUInt((uint64)0 - (uint64)x);          // safe even for the minimum long value
    } else {
        kprintUInt((uint64)x);
    }
}
