// syscall_cpp.hpp - object-oriented C++ API of the kernel (user side)
//
// A thin layer over the C API (syscall_c.hpp): every object holds the handle
// of the kernel object it represents and forwards its operations to the
// corresponding C function.
//
// The public and protected interface follows the kernel's API specification.

#ifndef _syscall_cpp
#define _syscall_cpp

#include "syscall_c.hpp"

// Global new and delete allocate through the mem_alloc / mem_free system calls.
void* operator new (size_t size);
void  operator delete (void* ptr) noexcept;
void* operator new[] (size_t size);
void  operator delete[] (void* ptr) noexcept;

// A thread runs either the function given to the constructor, or (if the
// default constructor was used) the run() method of a derived class.
// It starts only when start() is called. The Thread object must stay alive
// while a run()-based thread is running.
class Thread {
public:
    Thread (void (*body)(void*), void* arg);
    virtual ~Thread ();

    int start ();

    static void dispatch ();
    static int sleep (time_t);

protected:
    Thread ();
    virtual void run () {}

private:
    static void runWrapper (void* thread);

    thread_t myHandle;
    void (*body)(void*); void* arg;
};


class Semaphore {
public:

    Semaphore (unsigned init = 1);
    virtual ~Semaphore ();

    int wait ();
    int signal ();

private:
    sem_t myHandle;

};


// A thread that calls periodicActivation() every `period` timer ticks until
// terminate() is called. Derive from it and override periodicActivation().
class PeriodicThread : public Thread {
public:
    void terminate ();

protected:
    PeriodicThread (time_t period);
    virtual void periodicActivation () {}

private:
    static void periodicBody (void* thread);

    time_t period;      // 0 once the thread has been terminated
};


class Console {
public:
    static char getc ();
    static void putc (char);
};

#endif // _syscall_cpp
