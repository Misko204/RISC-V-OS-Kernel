// syscall_cpp.cpp - object-oriented C++ API of the kernel (user side)

#include "../h/syscall_cpp.hpp"
#include "../h/syscall_codes.hpp"

// ---------------------------------------------------------------- new / delete

void* operator new (size_t size) {
    return mem_alloc(size);
}

void operator delete (void* ptr) noexcept {
    if (ptr != nullptr) mem_free(ptr);
}

void* operator new[] (size_t size) {
    return mem_alloc(size);
}

void operator delete[] (void* ptr) noexcept {
    if (ptr != nullptr) mem_free(ptr);
}

// ---------------------------------------------------------------- Thread

Thread::Thread (void (*body)(void*), void* arg)
        : myHandle(nullptr), body(body), arg(arg) { }

// Used by derived classes that override run().
Thread::Thread ()
        : myHandle(nullptr), body(nullptr), arg(nullptr) { }

// The kernel thread frees its own resources when it finishes, so there is
// nothing to release here.
Thread::~Thread () { }

int Thread::start () {
    if (myHandle != nullptr) return (int)ERR_INVALID_ARGUMENT;   // already started

    // A function given to the constructor takes precedence over run().
    if (body != nullptr) return thread_create(&myHandle, body, arg);
    return thread_create(&myHandle, &Thread::runWrapper, this);
}

// Entry point of run()-based threads: calls the (virtual) run() of the object.
void Thread::runWrapper (void* thread) {
    ((Thread*)thread)->run();
}

void Thread::dispatch () {
    thread_dispatch();
}

int Thread::sleep (time_t ticks) {
    return time_sleep(ticks);
}

// ---------------------------------------------------------------- Semaphore

Semaphore::Semaphore (unsigned init) : myHandle(nullptr) {
    if (sem_open(&myHandle, init) != 0) myHandle = nullptr;
}

// Threads still waiting on the semaphore are released, and their wait() fails.
Semaphore::~Semaphore () {
    if (myHandle != nullptr) sem_close(myHandle);
}

int Semaphore::wait () {
    return sem_wait(myHandle);
}

int Semaphore::signal () {
    return sem_signal(myHandle);
}

// ---------------------------------------------------------------- PeriodicThread

// Runs as an ordinary function-based thread whose argument is the object
// itself; this keeps run() free for classes that derive from Thread directly.
PeriodicThread::PeriodicThread (time_t period)
        : Thread(&PeriodicThread::periodicBody, this), period(period) { }

// The interface has no room for an extra "terminated" flag, so a period of 0
// marks a terminated thread. The thread notices it the next time it wakes
// up and finishes.
void PeriodicThread::terminate () {
    period = 0;
}

void PeriodicThread::periodicBody (void* thread) {
    PeriodicThread* self = (PeriodicThread*)thread;
    while (self->period != 0) {
        self->periodicActivation();
        if (self->period == 0) break;
        time_sleep(self->period);
    }
}

// ---------------------------------------------------------------- Console

char Console::getc () {
    return ::getc();
}

void Console::putc (char c) {
    ::putc(c);
}
