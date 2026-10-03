// thread_test.cpp - self-tests for threads with synchronous context switching
// Runs in user mode (called from userMain), so it uses only the C API.

#include "tests.hpp"
#include "user_print.hpp"
#include "../h/syscall_c.hpp"
#include "../h/syscall_codes.hpp"

static int failures = 0;

static void check(const char* name, bool ok) {
    printString(ok ? "  [ OK ] " : "  [FAIL] ");
    printString(name);
    printString("\n");
    if (!ok) failures++;
}

// Waits by yielding until `counter` reaches `target`.
static void waitFor(volatile int& counter, int target) {
    while (counter < target) thread_dispatch();
}

// ---------------------------------------------------------------- 1. interleaving

static char trace[64];
static volatile int traceLength = 0;
static volatile int interleavingDone = 0;

static void letterBody(void* arg) {
    char letter = *(char*)arg;
    for (int i = 0; i < 3; i++) {
        trace[traceLength++] = letter;
        thread_dispatch();
    }
    interleavingDone++;
}

// ---------------------------------------------------------------- 2. arguments

struct Work {
    long input;
    long output;
};

static volatile int argumentsDone = 0;

static void squareBody(void* arg) {
    Work* work = (Work*)arg;
    work->output = work->input * work->input;
    argumentsDone++;
}

// ---------------------------------------------------------------- 3. thread_exit

static volatile int beforeExit = 0;
static volatile int afterExit = 0;

static void exitingBody(void*) {
    beforeExit = 1;
    thread_exit();
    afterExit = 1;          // must never run
}

// ---------------------------------------------------------------- 4. local state

static volatile int localsDone = 0;
static volatile int localsOk = 1;

// Keeps many values live across context switches; they must survive in
// registers and on the stack.
static void localsBody(void* arg) {
    long seed = (long)arg;
    long a = seed, b = seed * 2, c = seed * 3, d = seed * 5, e = seed * 7;
    for (int i = 0; i < 5; i++) {
        thread_dispatch();
        a += 1; b += 2; c += 3; d += 5; e += 7;
    }
    if (a != seed + 5 || b != seed * 2 + 10 || c != seed * 3 + 15 ||
        d != seed * 5 + 25 || e != seed * 7 + 35) {
        localsOk = 0;
    }
    localsDone++;
}

// ---------------------------------------------------------------- 5. many threads

static volatile int manyDone = 0;

static void counterBody(void*) {
    manyDone++;
}

// ----------------------------------------------------------------

int testThreads() {
    failures = 0;
    printString("Thread tests (user mode)\n");

    // 1. Three threads that yield after every step must interleave in FIFO order.
    static char letters[3] = { 'A', 'B', 'C' };
    thread_t a, b, c;
    bool created = thread_create(&a, letterBody, &letters[0]) == 0 &&
                   thread_create(&b, letterBody, &letters[1]) == 0 &&
                   thread_create(&c, letterBody, &letters[2]) == 0;
    check("thread_create returns 0", created);
    check("handles are distinct and non-null", a && b && c && a != b && b != c && a != c);
    waitFor(interleavingDone, 3);
    trace[traceLength] = '\0';
    bool fifo = traceLength == 9;
    const char* expected = "ABCABCABC";
    for (int i = 0; fifo && i < 9; i++) fifo = trace[i] == expected[i];
    printString("         trace: ");
    printString(trace);
    printString("\n");
    check("threads interleave in FIFO order", fifo);

    // 2. Each thread receives its own argument.
    Work work[4] = { {2, 0}, {3, 0}, {-4, 0}, {10, 0} };
    for (int i = 0; i < 4; i++) {
        thread_t handle;
        thread_create(&handle, squareBody, &work[i]);
    }
    waitFor(argumentsDone, 4);
    check("arguments are passed correctly",
          work[0].output == 4 && work[1].output == 9 &&
          work[2].output == 16 && work[3].output == 100);

    // 3. thread_exit terminates the thread immediately.
    thread_t exiting;
    thread_create(&exiting, exitingBody, nullptr);
    for (int i = 0; i < 5; i++) thread_dispatch();
    check("thread runs until thread_exit", beforeExit == 1);
    check("code after thread_exit never runs", afterExit == 0);

    // 4. Local variables survive context switches.
    for (long seed = 1; seed <= 3; seed++) {
        thread_t handle;
        thread_create(&handle, localsBody, (void*)(seed * 1000));
    }
    waitFor(localsDone, 3);
    check("local variables survive context switches", localsOk == 1);

    // 5. Many threads; all their memory is returned when they finish.
    thread_dispatch();                          // let earlier threads be freed
    size_t freeBefore = mem_get_free_space();
    const int N = 50;
    int createdCount = 0;
    for (int i = 0; i < N; i++) {
        thread_t handle;
        if (thread_create(&handle, counterBody, nullptr) == 0) createdCount++;
    }
    check("50 threads created", createdCount == N);
    waitFor(manyDone, N);
    thread_dispatch();                          // the last finished thread is freed here
    check("all 50 threads ran", manyDone == N);
    check("no memory leaked by finished threads", mem_get_free_space() == freeBefore);

    // 6. Invalid arguments are rejected and leak nothing.
    size_t freeBeforeErrors = mem_get_free_space();
    thread_t handle;
    check("thread_create(nullptr handle) fails",
          thread_create(nullptr, counterBody, nullptr) == ERR_INVALID_ARGUMENT);
    check("thread_create(nullptr body) fails",
          thread_create(&handle, nullptr, nullptr) == ERR_INVALID_ARGUMENT);
    check("failed thread_create leaks nothing", mem_get_free_space() == freeBeforeErrors);

    printString(failures == 0 ? "All thread tests passed\n" : "Some thread tests FAILED\n");
    return failures;
}
