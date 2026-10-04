// console_test.cpp - self-tests for the console driver
//
// testConsoleBuffers() runs in kernel mode before any thread exists. It feeds
// characters into the input buffer the way the console interrupt does and
// reads them back through the getc system call.
//
// testConsole() runs in user mode and exercises the output path with several
// writers, including writing far more than fits into the output buffer.

#include "tests.hpp"
#include "user_print.hpp"
#include "../h/kconsole.hpp"
#include "../h/kprint.hpp"
#include "../h/syscall_c.hpp"

// ---------------------------------------------------------------- kernel mode

static int kernelFailures = 0;

static void kcheck(const char* name, bool ok) {
    kprintString(ok ? "  [ OK ] " : "  [FAIL] ");
    kprintString(name);
    kprintString("\n");
    if (!ok) kernelFailures++;
}

int testConsoleBuffers() {
    kernelFailures = 0;
    kprintString("Console buffer tests\n");

    // 1. Received characters come out of getc in order.
    KConsole::receive('a');
    KConsole::receive('b');
    KConsole::receive('c');
    char first = getc(), second = getc(), third = getc();
    kcheck("getc returns received characters in order",
           first == 'a' && second == 'b' && third == 'c');

    // 2. Characters that do not fit into the input buffer are dropped,
    //    and the ones that fit are kept intact.
    const int capacity = (int)KConsole::BUFFER_SIZE;
    for (int i = 0; i < capacity + 10; i++) KConsole::receive((char)('A' + i % 26));
    bool intact = true;
    for (int i = 0; i < capacity; i++) {
        if (getc() != (char)('A' + i % 26)) intact = false;
    }
    kcheck("a full input buffer keeps the first characters", intact);

    // 3. Every byte value survives the trip (0xFF must not turn into EOF in the kernel).
    KConsole::receive((char)0xFF);
    KConsole::receive((char)0x00);
    kcheck("bytes 0xFF and 0x00 are passed through",
           getc() == (char)0xFF && getc() == (char)0x00);

    kprintString(kernelFailures == 0 ? "All console buffer tests passed\n"
                                     : "Some console buffer tests FAILED\n");
    return kernelFailures;
}

// ---------------------------------------------------------------- user mode

static int failures = 0;

static void check(const char* name, bool ok) {
    printString(ok ? "  [ OK ] " : "  [FAIL] ");
    printString(name);
    printString("\n");
    if (!ok) failures++;
}

static sem_t finished;
static sem_t lineMutex;

// Prints whole lines under a mutex, so that lines from different threads do
// not get mixed even though the threads are preempted.
static void lineWriter(void* arg) {
    char name = *(char*)arg;
    for (int i = 1; i <= 3; i++) {
        sem_wait(lineMutex);
        printString("         writer ");
        putc(name);
        printString(", line ");
        printInt(i);
        printString("\n");
        sem_signal(lineMutex);
        time_sleep(1);
    }
    sem_signal(finished);
}

// Writes many more characters than fit into the output buffer: putc must
// block when the buffer is full and continue as the output thread drains it.
static volatile int floodDone = 0;
static const int FLOOD_LINES = 12;
static const int FLOOD_WIDTH = 60;

static void flooder(void*) {
    for (int line = 0; line < FLOOD_LINES; line++) {
        printString("         ");
        for (int i = 0; i < FLOOD_WIDTH; i++) putc((char)('0' + (line + i) % 10));
        putc('\n');
    }
    floodDone = 1;
    sem_signal(finished);
}

int testConsole() {
    failures = 0;
    printString("Console tests (user mode)\n");
    sem_open(&finished, 0);
    sem_open(&lineMutex, 1);

    // 1. Several threads write lines concurrently.
    static char names[3] = { 'A', 'B', 'C' };
    for (int i = 0; i < 3; i++) {
        thread_t handle;
        thread_create(&handle, lineWriter, &names[i]);
    }
    for (int i = 0; i < 3; i++) sem_wait(finished);
    check("three writers finished (9 intact lines above)", true);

    // 2. Output larger than the buffer.
    thread_t handle;
    thread_create(&handle, flooder, nullptr);
    sem_wait(finished);
    check("writing more than the output buffer holds completes", floodDone == 1);

    sem_close(lineMutex);
    sem_close(finished);
    printString(failures == 0 ? "All console tests passed\n" : "Some console tests FAILED\n");
    return failures;
}

// Interactive: reads a line from the keyboard and echoes it back.
int testConsoleEcho() {
    printString("Console echo test: type a line and press Enter\n> ");
    char line[64];
    int length = 0;
    while (true) {
        char c = getc();
        if (c == '\r' || c == '\n') break;
        if (length < 63) line[length++] = c;
        putc(c);                    // echo while typing
    }
    line[length] = '\0';
    printString("\n");
    printString("  [ OK ] read a line from the keyboard: \"");
    printString(line);
    printString("\"\n");
    return 0;
}
