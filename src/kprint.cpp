// kprint.cpp - kernel debug output

#include "../h/kprint.hpp"
#include "../h/kconsole.hpp"

// Writes directly to the UART (by polling), bypassing the output buffer, so
// it works at any time: before threads exist, inside the trap handler and
// while reporting an exception.
void kprintChar(char c) {
    KConsole::writeSync(c);
}

void kprintString(const char* s) {
    while (*s != '\0') {
        kprintChar(*s);
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
        kprintChar(buffer[--count]);                // digits were stored in reverse order
    }
}

void kprintInt(long x) {
    if (x < 0) {
        kprintChar('-');
        kprintUInt((uint64)0 - (uint64)x);          // safe even for the minimum long value
    } else {
        kprintUInt((uint64)x);
    }
}
