// demo.hpp - demo user program

#ifndef _demo_hpp_
#define _demo_hpp_

// Dining philosophers with a periodic clock, then a keyboard echo.
// Uses only the C++ API (syscall_cpp.hpp).
void runDemo();

// Reads a line from the console, echoing it as it is typed. Stores at most
// capacity - 1 characters plus a terminating '\0' and returns the length.
int readLine(char* line, int capacity);

#endif // _demo_hpp_
