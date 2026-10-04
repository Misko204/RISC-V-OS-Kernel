// scheduler.cpp - FIFO ready queue

#include "../h/scheduler.hpp"
#include "../h/tcb.hpp"

ThreadQueue Scheduler::readyQueue;
TCB* Scheduler::idleThread = nullptr;

void Scheduler::put(TCB* thread) {
    if (thread == nullptr || thread == idleThread) return;
    readyQueue.put(thread);
}

TCB* Scheduler::get() {
    TCB* thread = readyQueue.get();
    return thread != nullptr ? thread : idleThread;
}
