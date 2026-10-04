// user_main.cpp - the user program: runs in user mode as the first user thread

#include "tests.hpp"
#include "user_print.hpp"

// Set to false to skip the test that waits for keyboard input.
static const bool INTERACTIVE_TESTS = true;

void userMain() {
    printString("\nuserMain started (user mode)\n");
    int failures = 0;
    failures += testThreads();
    failures += testSemaphores();
    failures += testTimer();
    failures += testConsole();
    failures += testCppApi();
    if (INTERACTIVE_TESTS) failures += testConsoleEcho();
    printString(failures == 0 ? "userMain: all user-mode tests passed\n"
                              : "userMain: SOME USER-MODE TESTS FAILED\n");
}
