// timer.hpp - timer ticks: sleeping threads and time sharing

#ifndef _timer_hpp_
#define _timer_hpp_

#include "../lib/hw.h"

class TCB;

class Timer {
public:
    // Called on every timer interrupt (10 times per second): wakes the
    // threads whose sleep has ended and preempts the running thread when its
    // time slice is used up.
    static void tick();

    // Blocks the running thread for `ticks` timer periods. Returns 0.
    // time_sleep(0) returns immediately.
    static int sleep(time_t ticks);

    // Number of timer ticks since the timer was started.
    static time_t ticks() { return tickCount; }

private:
    static void wakeSleepers();

    // Sleeping threads, ordered by wake-up time and linked through TCB::next.
    // Each thread's sleepDelta is the number of ticks between the wake-up of
    // the previous thread in the list and its own (for the first thread: from
    // now). A tick therefore only decrements the first delta.
    static TCB* sleepHead;
    static time_t tickCount;
};

#endif // _timer_hpp_
