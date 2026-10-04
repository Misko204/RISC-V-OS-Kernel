// timer_test.cpp - self-tests for preemption (time sharing) and time_sleep
// Runs in user mode (called from userMain), so it uses only the C API.
// None of the threads below ever calls thread_dispatch: every switch between
// them is caused by the timer or by blocking.

#include "tests.hpp"
#include "user_print.hpp"
#include "../h/syscall_c.hpp"

static int failures = 0;

static void check(const char* name, bool ok) {
    printString(ok ? "  [ OK ] " : "  [FAIL] ");
    printString(name);
    printString("\n");
    if (!ok) failures++;
}

static void startThread(void (*body)(void*), void* arg) {
    thread_t handle;
    thread_create(&handle, body, arg);
}

static sem_t finished;

static void waitForThreads(int count) {
    for (int i = 0; i < count; i++) sem_wait(finished);
}

// ---------------------------------------------------------------- preemption

static volatile int stopSpinning = 0;
static volatile unsigned long spins[3];

// An endless busy loop: without preemption, nothing else would ever run again.
static void spinner(void* arg) {
    int index = (int)(long)arg;
    while (!stopSpinning) spins[index]++;
    sem_signal(finished);
}

// ---------------------------------------------------------------- sleeping

struct Sleeper {
    char   name;
    time_t ticks;
};

static char wakeOrder[8];
static volatile int wakeCount = 0;

static void sleeper(void* arg) {
    Sleeper* s = (Sleeper*)arg;
    time_sleep(s->ticks);
    wakeOrder[wakeCount++] = s->name;
    sem_signal(finished);
}

// ---------------------------------------------------------------- mutex under preemption

static sem_t mutex;
static volatile long sharedCounter = 0;
static const int WORKERS = 3;
static const int INCREMENTS = 3000;

// The critical section is long enough that the timer often interrupts it.
static void worker(void*) {
    for (int i = 0; i < INCREMENTS; i++) {
        sem_wait(mutex);
        long value = sharedCounter;
        for (volatile int delay = 0; delay < 50; delay++) { }
        sharedCounter = value + 1;
        sem_signal(mutex);
    }
    sem_signal(finished);
}

// ----------------------------------------------------------------

static void resetWakeOrder() {
    wakeCount = 0;
    for (int i = 0; i < 8; i++) wakeOrder[i] = '\0';
}

static void printWakeOrder() {
    printString("         wake order: ");
    printString(wakeOrder);
    printString("\n");
}

int testTimer() {
    failures = 0;
    printString("Timer tests (user mode)\n");
    sem_open(&finished, 0);

    // 1. Busy threads that never yield still share the CPU, and this thread
    //    gets the CPU back after its sleep.
    for (long i = 0; i < 3; i++) startThread(spinner, (void*)i);
    time_sleep(10);                             // 1 s; the spinners run meanwhile
    stopSpinning = 1;
    waitForThreads(3);
    printString("         spins: ");
    for (int i = 0; i < 3; i++) { printInt((long)spins[i]); printString(i < 2 ? ", " : "\n"); }
    check("busy threads are preempted and all of them run",
          spins[0] > 0 && spins[1] > 0 && spins[2] > 0);
    check("a sleeping thread wakes up while others are busy", stopSpinning == 1);

    // 2. Sleepers wake up in order of their wake-up time, not of their start.
    static Sleeper ordered[3] = { {'A', 6}, {'B', 2}, {'C', 4} };
    resetWakeOrder();
    for (int i = 0; i < 3; i++) startThread(sleeper, &ordered[i]);
    waitForThreads(3);
    printWakeOrder();
    check("sleepers wake in order of wake-up time (BCA)",
          wakeOrder[0] == 'B' && wakeOrder[1] == 'C' && wakeOrder[2] == 'A');

    // 3. Sleepers with the same wake-up time wake in FIFO order.
    static Sleeper same[3] = { {'D', 3}, {'E', 3}, {'F', 3} };
    resetWakeOrder();
    for (int i = 0; i < 3; i++) startThread(sleeper, &same[i]);
    waitForThreads(3);
    printWakeOrder();
    check("equal sleeps wake in FIFO order (DEF)",
          wakeOrder[0] == 'D' && wakeOrder[1] == 'E' && wakeOrder[2] == 'F');

    // 4. Mixed: equal and different times inserted out of order.
    static Sleeper mixed[4] = { {'W', 5}, {'X', 1}, {'Y', 5}, {'Z', 3} };
    resetWakeOrder();
    for (int i = 0; i < 4; i++) startThread(sleeper, &mixed[i]);
    waitForThreads(4);
    printWakeOrder();
    check("mixed sleeps wake in order XZWY",
          wakeOrder[0] == 'X' && wakeOrder[1] == 'Z' && wakeOrder[2] == 'W' && wakeOrder[3] == 'Y');

    // 5. time_sleep(0) returns at once.
    check("time_sleep(0) returns 0", time_sleep(0) == 0);
    check("time_sleep(1) returns 0", time_sleep(1) == 0);

    // 6. A mutex keeps a long critical section correct under preemption.
    sem_open(&mutex, 1);
    sharedCounter = 0;
    for (int i = 0; i < WORKERS; i++) startThread(worker, nullptr);
    waitForThreads(WORKERS);
    check("mutex protects a critical section under preemption",
          sharedCounter == WORKERS * INCREMENTS);
    sem_close(mutex);

    sem_close(finished);
    printString(failures == 0 ? "All timer tests passed\n" : "Some timer tests FAILED\n");
    return failures;
}
