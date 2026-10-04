// kprint.hpp - kernel debug output
// Currently uses __putc from console.lib; will be redirected to the kernel's
// own console driver later.

#ifndef _kprint_hpp_
#define _kprint_hpp_

#include "../lib/hw.h"

void kprintChar(char c);
void kprintString(const char* s);
void kprintUInt(uint64 x, uint64 base = 10);   // base: 10 or 16
void kprintInt(long x);

#endif // _kprint_hpp_
