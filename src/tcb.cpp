// tcb.cpp - thread creation, switching and termination

#include "../h/tcb.hpp"
#include "../h/scheduler.hpp"
#include "../h/memory_allocator.hpp"
#include "../h/trap.hpp"
#include "../h/riscv.hpp"
#include "../h/syscall_c.hpp"

TCB* TCB::runningThread = nullptr;
TCB* TCB::finishedThreads = nullptr;
size_t TCB::userThreadCount = 0;

static void idleBody(void*) {
    while (true) { }
}

void TCB::init() {
    // The code that is running now (main) becomes a thread. Its context is
    // filled in by the first contextSwitch away from it.
    TCB* mainThread = new TCB(nullptr, nullptr, false, nullptr);
    mainThread->state = State::RUNNING;
    runningThread = mainThread;

    // The idle thread is never put into the ready queue: the scheduler hands
    // it out only when the queue is empty.
    TCB* idleThread = new TCB(idleBody, nullptr, false, nullptr);
    Scheduler::setIdleThread(idleThread);
}

TCB* TCB::createUserThread(Body body, void* arg, void* userStackTop) {
    TCB* thread = new TCB(body, arg, true, userStackTop);
    if (thread == nullptr) return nullptr;
    if (!thread->hasStack()) { delete thread; return nullptr; }
    userThreadCount++;
    Scheduler::put(thread);
    return thread;
}

TCB* TCB::createKernelThread(Body body, void* arg) {
    TCB* thread = new TCB(body, arg, false, nullptr);
    if (thread == nullptr) return nullptr;
    if (!thread->hasStack()) { delete thread; return nullptr; }
    Scheduler::put(thread);
    return thread;
}

TCB::TCB(Body body, void* arg, bool userMode, void* userStackTop) noexcept
: context(), body(body), arg(arg), kernelStack(nullptr), userStack(nullptr),
userMode(userMode), state(State::READY), next(nullptr) {

    if (body == nullptr) return;    // the main thread: already running, no stacks

    kernelStack = (uint64*)MemoryAllocator::alloc(MemoryAllocator::bytesToBlocks(KERNEL_STACK_SIZE));
    if (kernelStack == nullptr) return;
    uint64 kernelStackTop = (uint64)kernelStack + KERNEL_STACK_SIZE;
    if (userMode) userStack = (char*)userStackTop - DEFAULT_STACK_SIZE;

    // Build a TrapFrame at the top of the kernel stack, as if the thread had
    // trapped into the kernel just before the first instruction of
    // threadWrapper. The first switch to this thread "returns" into
    // trapReturn, which restores this frame and executes sret.
    TrapFrame* frame = (TrapFrame*)(kernelStackTop - sizeof(TrapFrame));
    for (int i = 0; i < 32; i++) frame->x[i] = 0;
    frame->x[TrapFrame::SP] = userMode ? (uint64)userStackTop : kernelStackTop;
    frame->x[TrapFrame::A0] = (uint64)body;
    frame->x[TrapFrame::A1] = (uint64)arg;
    frame->sepc = (uint64)&threadWrapper;

    // Interrupts enabled after sret (SPIE); SPP selects user or supervisor mode.
    uint64 sstatus = Riscv::r_sstatus();
    sstatus &= ~(uint64)(Riscv::SSTATUS_SPP | Riscv::SSTATUS_SPIE | Riscv::SSTATUS_SIE);
    sstatus |= Riscv::SSTATUS_SPIE;
    if (!userMode) sstatus |= Riscv::SSTATUS_SPP;
    frame->sstatus = sstatus;

    context.ra = (uint64)&trapReturn;
    context.sp = (uint64)frame;
}

TCB::~TCB() {
    if (kernelStack != nullptr) MemoryAllocator::free(kernelStack);
    if (userStack != nullptr) MemoryAllocator::free(userStack);
}

// Runs in the thread's own mode (user or supervisor). Never returns.
void TCB::threadWrapper(Body body, void* arg) {
    body(arg);
    thread_exit();
}

void TCB::dispatch() {
    reapFinished();

    TCB* current = runningThread;
    if (current->state == State::RUNNING) {
        current->state = State::READY;
        Scheduler::put(current);
    }

    TCB* nextThread = Scheduler::get();
    nextThread->state = State::RUNNING;
    runningThread = nextThread;

    if (nextThread != current) {
        contextSwitch(&current->context, &nextThread->context);
    }
}

void TCB::exit() {
    TCB* current = runningThread;
    current->state = State::FINISHED;
    if (current->userMode) userThreadCount--;

    // The thread is still running on its kernel stack, so it cannot be freed
    // here. It is freed by a later dispatch, from another thread's stack.
    current->next = finishedThreads;
    finishedThreads = current;

    dispatch();
}

// Frees finished threads, except the one that is still running.
void TCB::reapFinished() {
    TCB** link = &finishedThreads;
    while (*link != nullptr) {
        TCB* thread = *link;
        if (thread == runningThread) {
            link = &thread->next;
        } else {
            *link = thread->next;
            delete thread;
        }
    }
}

void* TCB::operator new(size_t size) noexcept {
return MemoryAllocator::alloc(MemoryAllocator::bytesToBlocks(size));
}

void TCB::operator delete(void* ptr) noexcept {
MemoryAllocator::free(ptr);
}
