// semaphore_test.cpp - self-tests for semaphores
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

static void startThread(void (*body)(void*), void* arg) {
    thread_t handle;
    thread_create(&handle, body, arg);
}

// Every test thread signals this when it finishes; the test waits on it.
static sem_t finished;

static void waitForThreads(int count) {
    for (int i = 0; i < count; i++) sem_wait(finished);
}

// ---------------------------------------------------------------- mutual exclusion

static sem_t mutex;
static volatile long sharedCounter = 0;
static const int INCREMENTERS = 4;
static const int INCREMENTS = 5;

// Read-modify-write with a context switch in the middle: without mutual
// exclusion, updates made by other threads in between are lost.
static void incrementer(void* useMutex) {
    for (int i = 0; i < INCREMENTS; i++) {
        if (useMutex) sem_wait(mutex);
        long value = sharedCounter;
        thread_dispatch();
        sharedCounter = value + 1;
        if (useMutex) sem_signal(mutex);
    }
    sem_signal(finished);
}

// ---------------------------------------------------------------- blocking and wake-up order

static sem_t gate;
static char wakeOrder[8];
static volatile int wakeCount = 0;
static volatile int passedGate = 0;

static void gateWaiter(void* arg) {
    sem_wait(gate);
    wakeOrder[wakeCount++] = *(char*)arg;
    passedGate++;
    sem_signal(finished);
}

// ---------------------------------------------------------------- producer / consumer

static const int BUFFER_SIZE = 5;
static const int ITEMS = 50;
static int buffer[BUFFER_SIZE];
static int putIndex = 0, takeIndex = 0;
static sem_t spaceAvailable, itemsAvailable, bufferMutex;
static volatile long consumedSum = 0;
static volatile int maxFill = 0, fill = 0;

static void producer(void*) {
    for (int item = 1; item <= ITEMS; item++) {
        sem_wait(spaceAvailable);
        sem_wait(bufferMutex);
        buffer[putIndex] = item;
        putIndex = (putIndex + 1) % BUFFER_SIZE;
        if (++fill > maxFill) maxFill = fill;
        sem_signal(bufferMutex);
        sem_signal(itemsAvailable);
    }
    sem_signal(finished);
}

static void consumer(void*) {
    for (int i = 0; i < ITEMS; i++) {
        sem_wait(itemsAvailable);
        sem_wait(bufferMutex);
        consumedSum += buffer[takeIndex];
        takeIndex = (takeIndex + 1) % BUFFER_SIZE;
        fill--;
        sem_signal(bufferMutex);
        sem_signal(spaceAvailable);
    }
    sem_signal(finished);
}

// ---------------------------------------------------------------- closing

static sem_t closing;
static volatile int closeResults[2] = { 1, 1 };

static void closedWaiter(void* arg) {
    int index = (int)(long)arg;
    closeResults[index] = sem_wait(closing);
    sem_signal(finished);
}

// ----------------------------------------------------------------

int testSemaphores() {
    failures = 0;
    printString("Semaphore tests (user mode)\n");

    // 1. Creating semaphores.
    check("sem_open returns 0", sem_open(&finished, 0) == 0 && finished != nullptr);
    check("sem_open(nullptr) fails", sem_open(nullptr, 0) == ERR_INVALID_ARGUMENT);

    // 2. Without a mutex the read-modify-write race loses updates...
    sharedCounter = 0;
    for (int i = 0; i < INCREMENTERS; i++) startThread(incrementer, nullptr);
    waitForThreads(INCREMENTERS);
    check("without a mutex, updates are lost", sharedCounter < INCREMENTERS * INCREMENTS);

    // ...and with a mutex none are.
    sem_open(&mutex, 1);
    sharedCounter = 0;
    for (int i = 0; i < INCREMENTERS; i++) startThread(incrementer, (void*)1);
    waitForThreads(INCREMENTERS);
    check("with a mutex, no update is lost", sharedCounter == INCREMENTERS * INCREMENTS);

    // 3. Threads block on a closed gate and are released one per signal, in FIFO order.
    static char names[3] = { 'A', 'B', 'C' };
    sem_open(&gate, 0);
    for (int i = 0; i < 3; i++) startThread(gateWaiter, &names[i]);
    for (int i = 0; i < 5; i++) thread_dispatch();
    check("waiters block on a zero semaphore", passedGate == 0);
    sem_signal(gate);
    for (int i = 0; i < 5; i++) thread_dispatch();
    check("one signal releases exactly one waiter", passedGate == 1);
    sem_signal(gate);
    sem_signal(gate);
    waitForThreads(3);
    wakeOrder[wakeCount] = '\0';
    printString("         wake order: ");
    printString(wakeOrder);
    printString("\n");
    check("waiters are released in FIFO order",
          wakeOrder[0] == 'A' && wakeOrder[1] == 'B' && wakeOrder[2] == 'C');

    // 4. Signals without waiters are remembered (counting semaphore).
    sem_t counting;
    sem_open(&counting, 0);
    sem_signal(counting);
    sem_signal(counting);
    bool counted = sem_wait(counting) == 0 && sem_wait(counting) == 0;
    check("signals are counted when no one waits", counted);
    sem_close(counting);

    // 5. Bounded buffer: producer and consumer synchronized by three semaphores.
    sem_open(&spaceAvailable, BUFFER_SIZE);
    sem_open(&itemsAvailable, 0);
    sem_open(&bufferMutex, 1);
    startThread(consumer, nullptr);
    startThread(producer, nullptr);
    waitForThreads(2);
    check("consumer received every item (sum 1..50)", consumedSum == ITEMS * (ITEMS + 1) / 2);
    check("buffer never overflowed", maxFill <= BUFFER_SIZE);

    // 6. Closing a semaphore releases its waiters with an error.
    sem_open(&closing, 0);
    startThread(closedWaiter, (void*)0);
    startThread(closedWaiter, (void*)1);
    for (int i = 0; i < 5; i++) thread_dispatch();
    check("sem_close returns 0", sem_close(closing) == 0);
    waitForThreads(2);
    check("waiters of a closed semaphore get ERR_SEMAPHORE_CLOSED",
          closeResults[0] == ERR_SEMAPHORE_CLOSED && closeResults[1] == ERR_SEMAPHORE_CLOSED);

    // 7. Invalid handles are rejected.
    check("sem_wait(nullptr) fails", sem_wait(nullptr) == ERR_INVALID_ARGUMENT);
    check("sem_signal(nullptr) fails", sem_signal(nullptr) == ERR_INVALID_ARGUMENT);
    check("sem_close(nullptr) fails", sem_close(nullptr) == ERR_INVALID_ARGUMENT);

    // 8. Opening and closing semaphores leaks no memory.
    thread_dispatch();                          // free threads that finished above
    size_t freeBefore = mem_get_free_space();
    for (int i = 0; i < 100; i++) {
        sem_t temp;
        sem_open(&temp, (unsigned)i);
        sem_close(temp);
    }
    check("100 x sem_open/sem_close leaks nothing", mem_get_free_space() == freeBefore);

    sem_close(mutex);
    sem_close(gate);
    sem_close(spaceAvailable);
    sem_close(itemsAvailable);
    sem_close(bufferMutex);
    sem_close(finished);

    printString(failures == 0 ? "All semaphore tests passed\n" : "Some semaphore tests FAILED\n");
    return failures;
}
