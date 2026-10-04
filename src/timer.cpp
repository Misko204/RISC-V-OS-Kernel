// timer.cpp - timer ticks: sleeping threads and time sharing

#include "../h/timer.hpp"
#include "../h/tcb.hpp"

TCB* Timer::sleepHead = nullptr;
time_t Timer::tickCount = 0;

void Timer::tick() {
    tickCount++;
    wakeSleepers();         // first, so that woken threads are already ready...
    TCB::timerTick();       // ...when the running thread may be preempted
}

int Timer::sleep(time_t ticks) {
    if (ticks == 0) return 0;

    TCB* thread = TCB::running();

    // Walk past every sleeper that wakes up no later than this thread,
    // turning `ticks` into a delta relative to the thread before it.
    // Using <= keeps threads with the same wake-up time in FIFO order.
    TCB** link = &sleepHead;
    while (*link != nullptr && (*link)->sleepDelta <= ticks) {
        ticks -= (*link)->sleepDelta;
        link = &(*link)->next;
    }

    thread->sleepDelta = ticks;
    thread->next = *link;
    if (*link != nullptr) (*link)->sleepDelta -= ticks;   // the next one now counts from us
    *link = thread;

    return TCB::block();    // unblocked by wakeSleepers()
}

void Timer::wakeSleepers() {
    if (sleepHead == nullptr) return;

    if (sleepHead->sleepDelta > 0) sleepHead->sleepDelta--;

    while (sleepHead != nullptr && sleepHead->sleepDelta == 0) {
        TCB* thread = sleepHead;
        sleepHead = thread->next;
        thread->next = nullptr;
        TCB::unblock(thread, 0);
    }
}
