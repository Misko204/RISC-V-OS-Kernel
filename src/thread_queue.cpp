// thread_queue.cpp - FIFO queue of threads, linked through TCB::next

#include "../h/thread_queue.hpp"
#include "../h/tcb.hpp"

void ThreadQueue::put(TCB* thread) {
    thread->next = nullptr;
    if (tail != nullptr) tail->next = thread;
    else head = thread;
    tail = thread;
}

TCB* ThreadQueue::get() {
    if (head == nullptr) return nullptr;
    TCB* thread = head;
    head = head->next;
    if (head == nullptr) tail = nullptr;
    thread->next = nullptr;
    return thread;
}
