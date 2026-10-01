// kprint.hpp - pomocni ispis za debagovanje jezgra
// Za sada koristi __putc iz console.lib; u fazi 5 se preusmerava na nasu konzolu.

#ifndef _kprint_hpp_
#define _kprint_hpp_

#include "../lib/hw.h"

void kprintString(const char* s);
void kprintUInt(uint64 x, uint64 base = 10);   // base: 10 ili 16
void kprintInt(long x);

#endif // _kprint_hpp_