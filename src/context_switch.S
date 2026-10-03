# context_switch.S - switching between threads
#
# void contextSwitch(Context* current, Context* next)
#   a0 = current: where to save ra, sp and s0..s11 of the running thread
#   a1 = next:    where to load them from for the thread that runs next
#
# It is called like a normal function, so the caller has already saved every
# caller-saved register it needs. After the loads, `ret` jumps to next->ra on
# the next thread's stack: either back into that thread's earlier call to
# contextSwitch, or, for a new thread, into trapReturn.

.text
.align 4
.global contextSwitch
.type contextSwitch, @function
contextSwitch:
    sd ra,   0 * 8(a0)
    sd sp,   1 * 8(a0)
    sd s0,   2 * 8(a0)
    sd s1,   3 * 8(a0)
    sd s2,   4 * 8(a0)
    sd s3,   5 * 8(a0)
    sd s4,   6 * 8(a0)
    sd s5,   7 * 8(a0)
    sd s6,   8 * 8(a0)
    sd s7,   9 * 8(a0)
    sd s8,  10 * 8(a0)
    sd s9,  11 * 8(a0)
    sd s10, 12 * 8(a0)
    sd s11, 13 * 8(a0)

    ld ra,   0 * 8(a1)
    ld sp,   1 * 8(a1)
    ld s0,   2 * 8(a1)
    ld s1,   3 * 8(a1)
    ld s2,   4 * 8(a1)
    ld s3,   5 * 8(a1)
    ld s4,   6 * 8(a1)
    ld s5,   7 * 8(a1)
    ld s6,   8 * 8(a1)
    ld s7,   9 * 8(a1)
    ld s8,  10 * 8(a1)
    ld s9,  11 * 8(a1)
    ld s10, 12 * 8(a1)
    ld s11, 13 * 8(a1)

    ret
