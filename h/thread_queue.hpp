// thread_queue.hpp - FIFO queue of threads, linked through TCB::next

#ifndef _thread_queue_hpp_
#define _thread_queue_hpp_

class TCB;

// A thread can be in at most one ThreadQueue at a time (the ready queue or the
// wait queue of one semaphore), so a single link field in the TCB is enough
// and no memory is allocated when threads are queued.
class ThreadQueue {
public:
    // constexpr: queues that are static members are initialized at compile
    // time (there is no C runtime to run constructors of global objects).
    constexpr ThreadQueue() : head(nullptr), tail(nullptr) { }

    void put(TCB* thread);      // append at the tail
    TCB* get();                 // remove from the head; nullptr if empty
    bool isEmpty() const { return head == nullptr; }

private:
    TCB* head;
    TCB* tail;
};

#endif // _thread_queue_hpp_
