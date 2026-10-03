// scheduler.hpp - FIFO ready queue

#ifndef _scheduler_hpp_
#define _scheduler_hpp_

class TCB;

class Scheduler {
public:
    // Appends a thread to the ready queue. The idle thread is never queued.
    static void put(TCB* thread);

    // Removes and returns the first ready thread, or the idle thread if the
    // queue is empty. Never returns nullptr once the idle thread is set.
    static TCB* get();

    static void setIdleThread(TCB* thread) { idleThread = thread; }

private:
    static TCB* head;
    static TCB* tail;
    static TCB* idleThread;
};

#endif // _scheduler_hpp_
