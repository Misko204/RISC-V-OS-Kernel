// user_print.hpp - console output for code running in user mode
// (kprint cannot be used there: it accesses supervisor-only registers).

#ifndef _user_print_hpp_
#define _user_print_hpp_

#include "../lib/hw.h"

void printString(const char* s);
void printInt(long x);

#endif // _user_print_hpp_
