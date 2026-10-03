// user_print.cpp - console output for code running in user mode

#include "user_print.hpp"
#include "../h/syscall_c.hpp"

void printString(const char* s) {
    while (*s != '\0') putc(*s++);
}

void printInt(long x) {
    char buffer[24];
    int count = 0;
    uint64 value = x < 0 ? (uint64)0 - (uint64)x : (uint64)x;
    do {
        buffer[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    if (x < 0) putc('-');
    while (count > 0) putc(buffer[--count]);
}
