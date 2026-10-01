// kprint.cpp - implementacija debag ispisa

#include "../h/kprint.hpp"
#include "../h/riscv.hpp"
#include "../lib/console.h"

// Ispis jednog znaka sa maskiranim prekidima.
static void kputc(char c) {
    uint64 oldSstatus = Riscv::r_sstatus();
    Riscv::mc_sstatus(Riscv::SSTATUS_SIE);          // zabrani prekide
    __putc(c);
    if (oldSstatus & Riscv::SSTATUS_SIE) {          // vrati SIE samo ako je bio ukljucen
        Riscv::ms_sstatus(Riscv::SSTATUS_SIE);
    }
}

void kprintString(const char* s) {
    while (*s != '\0') {
        kputc(*s);
        s++;
    }
}

void kprintUInt(uint64 x, uint64 base) {
    static const char digits[] = "0123456789ABCDEF";
    char buffer[24];                                // 2^64 ima najvise 20 decimalnih cifara
    int count = 0;

    do {
        buffer[count++] = digits[x % base];
        x /= base;
    } while (x != 0);

    if (base == 16) kprintString("0x");

    while (count > 0) {
        kputc(buffer[--count]);                     // cifre su upisane unazad
    }
}

void kprintInt(long x) {
    if (x < 0) {
        kputc('-');
        kprintUInt((uint64)0 - (uint64)x);          // bezbedno i za najmanji long
    } else {
        kprintUInt((uint64)x);
    }
}