// tests.hpp - kernel self-tests

#ifndef _tests_hpp_
#define _tests_hpp_

// Each function prints its results and returns the number of failed checks.

// Kernel-mode tests, called from main().
int testMemoryAllocator();
int testSystemCalls();
int testConsoleBuffers();

// User-mode tests, called from userMain().
int testThreads();
int testSemaphores();
int testTimer();
int testConsole();
int testCppApi();
int testConsoleEcho();   // interactive: waits for a line from the keyboard

#endif // _tests_hpp_
