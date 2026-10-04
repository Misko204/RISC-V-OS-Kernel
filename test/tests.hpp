// tests.hpp - kernel self-tests

#ifndef _tests_hpp_
#define _tests_hpp_

// Each function prints its results and returns the number of failed checks.

// Kernel-mode tests, called from main().
int testMemoryAllocator();
int testSystemCalls();

// User-mode tests, called from userMain().
int testThreads();
int testSemaphores();

#endif // _tests_hpp_
