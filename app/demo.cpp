// demo.cpp - a small user program written only against the C++ API
//
// Part 1, dining philosophers: five threads share five forks (semaphores).
// A "seats" semaphore lets at most four of them reach for forks at the same
// time, which rules out deadlock. A periodic thread prints the time while
// they eat.
//
// Part 2, keyboard: lines typed on the console are echoed back until "q" is
// entered. Empty lines are ignored, so a stray Enter does not end the demo.

#include "demo.hpp"
#include "../h/syscall_cpp.hpp"

// ---------------------------------------------------------------- output

// putc is one system call per character and threads can be preempted at any
// time, so whole lines are printed under a lock to keep them intact.
static Semaphore* printLock = nullptr;

static void print(const char* s) {
    while (*s != '\0') Console::putc(*s++);
}

static void print(long x) {
    char digits[24];
    int count = 0;
    unsigned long value = x < 0 ? 0UL - (unsigned long)x : (unsigned long)x;
    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    if (x < 0) Console::putc('-');
    while (count > 0) Console::putc(digits[--count]);
}

// ---------------------------------------------------------------- dining philosophers

static const int PHILOSOPHERS = 5;
static const int MEALS = 3;

class Philosopher : public Thread {
public:
    Philosopher(int id, Semaphore* left, Semaphore* right, Semaphore* seats, Semaphore* done)
        : Thread(), id(id), left(left), right(right), seats(seats), done(done) { }

protected:
    void run() override {
        for (int meal = 1; meal <= MEALS; meal++) {
            Thread::sleep(1 + id % 3);              // think

            seats->wait();
            left->wait();
            right->wait();

            printLock->wait();
            print("  philosopher ");
            print((long)id);
            print(" eats    (meal ");
            print((long)meal);
            print("/");
            print((long)MEALS);
            print(")\n");
            printLock->signal();

            Thread::sleep(2);                       // eat

            right->signal();
            left->signal();
            seats->signal();
        }

        printLock->wait();
        print("  philosopher ");
        print((long)id);
        print(" is done\n");
        printLock->signal();

        done->signal();
    }

private:
    int id;
    Semaphore* left;
    Semaphore* right;
    Semaphore* seats;
    Semaphore* done;
};

// Prints the elapsed time every half second.
class Clock : public PeriodicThread {
public:
    Clock() : PeriodicThread(5), tenths(0) { }

protected:
    void periodicActivation() override {
        printLock->wait();
        print("  [clock ");
        print(tenths / 10);
        print(".");
        print(tenths % 10);
        print(" s]\n");
        printLock->signal();
        tenths += 5;
    }

private:
    long tenths;
};

static void diningPhilosophers() {
    print("\n--- Dining philosophers: 5 threads, 5 forks, 3 meals each ---\n");

    Semaphore* forks[PHILOSOPHERS];
    for (int i = 0; i < PHILOSOPHERS; i++) forks[i] = new Semaphore(1);
    Semaphore* seats = new Semaphore(PHILOSOPHERS - 1);
    Semaphore* done = new Semaphore(0);

    Clock* clock = new Clock();
    clock->start();

    Philosopher* philosophers[PHILOSOPHERS];
    for (int i = 0; i < PHILOSOPHERS; i++) {
        philosophers[i] = new Philosopher(i, forks[i], forks[(i + 1) % PHILOSOPHERS], seats, done);
        philosophers[i]->start();
    }

    for (int i = 0; i < PHILOSOPHERS; i++) done->wait();

    clock->terminate();
    Thread::sleep(6);                   // let the clock thread notice and finish

    printLock->wait();
    print("  all philosophers have eaten\n");
    printLock->signal();

    for (int i = 0; i < PHILOSOPHERS; i++) delete philosophers[i];
    delete clock;
    delete done;
    delete seats;
    for (int i = 0; i < PHILOSOPHERS; i++) delete forks[i];
}

// ---------------------------------------------------------------- keyboard

// Reads one line, echoing the characters as they are typed. Returns its length.
// Enter arrives as '\r' from a raw terminal and as '\n' from an IDE console;
// a "\r\n" pair counts as a single line ending.
int readLine(char* line, int capacity) {
    static bool lastWasCarriageReturn = false;
    int length = 0;
    while (true) {
        char c = Console::getc();
        if (c == '\n' && lastWasCarriageReturn && length == 0) {
            lastWasCarriageReturn = false;
            continue;                           // the second half of "\r\n"
        }
        lastWasCarriageReturn = (c == '\r');
        if (c == '\r' || c == '\n') break;
        if (length < capacity - 1) {
            line[length++] = c;
            Console::putc(c);
        }
    }
    line[length] = '\0';
    Console::putc('\n');
    return length;
}

static bool isQuit(const char* line) {
    if ((line[0] == 'q' || line[0] == 'Q') && line[1] == '\0') return true;
    const char* word = "quit";
    for (int i = 0; i < 4; i++) {
        char c = line[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c != word[i]) return false;
    }
    return line[4] == '\0';
}

static void keyboardEcho() {
    print("\n--- Keyboard: type a line and press Enter (q ends the demo) ---\n");
    char line[80];
    while (true) {
        print("> ");
        int length = readLine(line, sizeof(line));
        if (length == 0) continue;              // ignore empty lines
        if (isQuit(line)) break;

        for (int i = 0; i < length; i++) {
            if (line[i] >= 'a' && line[i] <= 'z') line[i] = (char)(line[i] - 'a' + 'A');
        }
        print("  ");
        print(line);
        print("  (");
        print((long)length);
        print(" characters)\n");
    }
}

// ----------------------------------------------------------------

void runDemo() {
    printLock = new Semaphore(1);
    diningPhilosophers();
    keyboardEcho();
    print("\nDemo finished\n");
    delete printLock;
}
