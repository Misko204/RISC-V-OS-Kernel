// user_main.cpp - the user program: runs in user mode as the first user thread

#include "tests.hpp"
#include "user_print.hpp"

void userMain() {
    printString("\nuserMain started (user mode)\n");
    int failures = 0;
    failures += testThreads();
    failures += testSemaphores();
    failures += testTimer();
    printString(failures == 0 ? "userMain: all user-mode tests passed\n"
                              : "userMain: SOME USER-MODE TESTS FAILED\n");
}
