// scheduler.cpp - FIFO ready queue, linked through TCB::next

#include "../h/scheduler.hpp"
#include "../h/tcb.hpp"

TCB* Scheduler::head = nullptr;
TCB* Scheduler::tail = nullptr;
TCB* Scheduler::idleThread = nullptr;

void Scheduler::put(TCB* thread) {
    if (thread == nullptr || thread == idleThread) return;
    thread->next = nullptr;
    if (tail != nullptr) tail->next = thread;
    else head = thread;
    tail = thread;
}

TCB* Scheduler::get() {
    if (head == nullptr) return idleThread;
    TCB* thread = head;
    head = head->next;
    if (head == nullptr) tail = nullptr;
    thread->next = nullptr;
    return thread;
}
