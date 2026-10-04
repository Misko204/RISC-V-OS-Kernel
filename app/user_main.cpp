// user_main.cpp - the user program: runs in user mode as the first user thread

#include "demo.hpp"
#include "../test/tests.hpp"
#include "../h/syscall_cpp.hpp"

static void print(const char* s) {
    while (*s != '\0') Console::putc(*s++);
}

static int runTests() {
    int failures = 0;
    failures += testThreads();
    failures += testSemaphores();
    failures += testTimer();
    failures += testConsole();
    failures += testCppApi();
    failures += testConsoleEcho();      // interactive: waits for a line
    print(failures == 0 ? "userMain: all user-mode tests passed\n"
                        : "userMain: SOME USER-MODE TESTS FAILED\n");
    return failures;
}

void userMain() {
    print("\nuserMain started (user mode)\n");
    print("  [t] run the user-mode self-tests\n");
    print("  [d] run the demo\n");

    char line[16];
    while (true) {
        print("Type t or d and press Enter: ");
        readLine(line, sizeof(line));
        char choice = line[0];

        if (choice == 't' || choice == 'T') { runTests(); return; }
        if (choice == 'd' || choice == 'D') { runDemo();  return; }
    }
}
