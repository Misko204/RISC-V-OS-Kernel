// tcb.hpp - thread control block: the kernel's representation of a thread

#ifndef _tcb_hpp_
#define _tcb_hpp_

#include "../lib/hw.h"

// Registers saved by contextSwitch: the return address, the stack pointer and
// the callee-saved registers. Everything else is already on the thread's
// kernel stack (TrapFrame), or is caller-saved by the calling convention.
struct Context {
    uint64 ra;
    uint64 sp;
    uint64 s[12];       // s0..s11
};

// Defined in src/context_switch.S. Saves the current registers into `current`
// and loads `next`; returns on the stack of the next thread.
extern "C" void contextSwitch(Context* current, Context* next);

class TCB {
public:
    using Body = void (*)(void*);

    enum class State { READY, RUNNING, FINISHED };

    // Wraps the code currently executing (main) into a thread and creates the
    // idle thread. Must be called once, after MemoryAllocator::init().
    static void init();

    // Creates a user-mode thread that runs body(arg) on the given user stack
    // (userStackTop points just past its last byte; the stack is
    // DEFAULT_STACK_SIZE bytes long and is freed when the thread finishes).
    // The thread is put into the ready queue. Returns nullptr if out of memory.
    static TCB* createUserThread(Body body, void* arg, void* userStackTop);

    // Creates a supervisor-mode thread that uses only its kernel stack.
    static TCB* createKernelThread(Body body, void* arg);

    // Gives the CPU to the next ready thread (possibly the same one).
    static void dispatch();

    // Terminates the running thread. Does not return.
    static void exit();

    static TCB* running() { return runningThread; }
    static size_t liveUserThreads() { return userThreadCount; }

    // Kernel objects are allocated directly from the kernel allocator,
    // never through the mem_alloc system call.
    static void* operator new(size_t size) noexcept;
    static void operator delete(void* ptr) noexcept;

private:
    TCB(Body body, void* arg, bool userMode, void* userStackTop) noexcept;
    ~TCB();

    bool hasStack() const { return kernelStack != nullptr; }
    static void reapFinished();
    static void threadWrapper(Body body, void* arg);

    static constexpr size_t KERNEL_STACK_SIZE = 2 * DEFAULT_STACK_SIZE;

    Context context;
    Body    body;
    void*   arg;
    uint64* kernelStack;    // bottom of the kernel stack (nullptr for the main thread)
    void*   userStack;      // bottom of the user stack (nullptr for kernel threads)
    bool    userMode;
    State   state;
    TCB*    next;           // link in the ready queue or in the list of finished threads

    static TCB* runningThread;
    static TCB* finishedThreads;
    static size_t userThreadCount;

    friend class Scheduler;
};

#endif // _tcb_hpp_
