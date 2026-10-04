// cpp_api_test.cpp - self-tests for the C++ API (Thread, Semaphore,
// PeriodicThread, Console, new/delete). Runs in user mode, from userMain.

#include "tests.hpp"
#include "user_print.hpp"
#include "../h/syscall_cpp.hpp"

static int failures = 0;

static void check(const char* name, bool ok) {
    printString(ok ? "  [ OK ] " : "  [FAIL] ");
    printString(name);
    printString("\n");
    if (!ok) failures++;
}

// ---------------------------------------------------------------- helpers

// Signalled by every test thread when it is done.
static Semaphore* finished = nullptr;

// ---------------------------------------------------------------- function-based thread

static volatile int functionResult = 0;

static void functionBody(void* arg) {
    functionResult = *(int*)arg * 2;
    finished->signal();
}

// ---------------------------------------------------------------- run()-based thread

class Worker : public Thread {
public:
    Worker(int id) : Thread(), result(0), id(id) { }
    int result;
protected:
    void run() override {
        result = id * 10;
        finished->signal();
    }
private:
    int id;
};

// A class that overrides run() but also passes a function to the constructor:
// the function must win and run() must be ignored.
static volatile int whichRan = 0;      // 1 = function, 2 = run()

static void preferredBody(void*) {
    whichRan = 1;
    finished->signal();
}

class Mixed : public Thread {
public:
    Mixed() : Thread(preferredBody, nullptr) { }
protected:
    void run() override {
        whichRan = 2;
        finished->signal();
    }
};

// ---------------------------------------------------------------- Semaphore under preemption

class Incrementer : public Thread {
public:
    Incrementer(Semaphore* mutex, volatile long* counter) : mutex(mutex), counter(counter) { }
protected:
    void run() override {
        for (int i = 0; i < 2000; i++) {
            mutex->wait();
            long value = *counter;
            for (volatile int delay = 0; delay < 20; delay++) { }
            *counter = value + 1;
            mutex->signal();
        }
        finished->signal();
    }
private:
    Semaphore* mutex;
    volatile long* counter;
};

// ---------------------------------------------------------------- PeriodicThread

class Ticker : public PeriodicThread {
public:
    Ticker(time_t period) : PeriodicThread(period), activations(0) { }
    volatile int activations;
protected:
    void periodicActivation() override {
        activations++;
    }
};

// ---------------------------------------------------------------- the test

int testCppApi() {
    failures = 0;
    printString("C++ API tests (user mode)\n");

    // 1. Global new / delete go through mem_alloc / mem_free.
    size_t freeBefore = mem_get_free_space();
    int* numbers = new int[100];
    check("new[] allocates from the kernel heap", numbers != nullptr && mem_get_free_space() < freeBefore);
    for (int i = 0; i < 100; i++) numbers[i] = i * i;
    check("new[] memory is usable", numbers[99] == 99 * 99);
    delete[] numbers;
    check("delete[] returns the memory", mem_get_free_space() == freeBefore);

    finished = new Semaphore(0);
    check("new Semaphore", finished != nullptr);

    // 2. Thread with a function body.
    int input = 21;
    Thread* functionThread = new Thread(functionBody, &input);
    check("Thread(body, arg).start() returns 0", functionThread->start() == 0);
    finished->wait();
    check("function body runs with its argument", functionResult == 42);
    check("start() twice fails", functionThread->start() < 0);
    delete functionThread;

    // 3. Thread subclasses overriding run().
    Worker* workers[3];
    for (int i = 0; i < 3; i++) {
        workers[i] = new Worker(i + 1);
        workers[i]->start();
    }
    for (int i = 0; i < 3; i++) finished->wait();
    check("run() of each subclass object runs",
          workers[0]->result == 10 && workers[1]->result == 20 && workers[2]->result == 30);
    for (int i = 0; i < 3; i++) delete workers[i];

    // 4. A function given to the constructor takes precedence over run().
    Mixed* mixed = new Mixed();
    mixed->start();
    finished->wait();
    check("constructor function wins over run()", whichRan == 1);
    delete mixed;

    // 5. Semaphore as a mutex under preemption.
    Semaphore* mutex = new Semaphore(1);
    volatile long counter = 0;
    Incrementer* incrementers[3];
    for (int i = 0; i < 3; i++) {
        incrementers[i] = new Incrementer(mutex, &counter);
        incrementers[i]->start();
    }
    for (int i = 0; i < 3; i++) finished->wait();
    check("Semaphore protects a critical section (6000)", counter == 6000);
    for (int i = 0; i < 3; i++) delete incrementers[i];
    delete mutex;

    // 6. Thread::sleep and Thread::dispatch.
    check("Thread::sleep returns 0", Thread::sleep(2) == 0);
    Thread::dispatch();
    check("Thread::dispatch returns", true);

    // 7. PeriodicThread: activations every 2 ticks until terminate().
    Ticker* ticker = new Ticker(2);
    ticker->start();
    Thread::sleep(11);                      // about 6 activations (one at the start)
    ticker->terminate();
    int countAtTerminate = ticker->activations;
    Thread::sleep(6);                       // the thread notices and finishes
    printString("         periodic activations in 1.1 s: ");
    printInt(countAtTerminate);
    printString("\n");
    check("PeriodicThread is activated periodically", countAtTerminate >= 4 && countAtTerminate <= 8);
    check("terminate() stops the activations", ticker->activations == countAtTerminate);
    delete ticker;

    // 8. Console.
    const char* text = "         Console::putc works\n";
    for (const char* p = text; *p != '\0'; p++) Console::putc(*p);
    check("Console::putc", true);

    // 9. Objects created and destroyed through the API leak nothing.
    delete finished;
    Thread::sleep(1);                       // let the last finished threads be freed
    size_t freeBeforeObjects = mem_get_free_space();
    for (int i = 0; i < 50; i++) {
        Semaphore* s = new Semaphore(i);
        delete s;
    }
    check("50 x new/delete Semaphore leaks nothing", mem_get_free_space() == freeBeforeObjects);

    printString(failures == 0 ? "All C++ API tests passed\n" : "Some C++ API tests FAILED\n");
    return failures;
}
