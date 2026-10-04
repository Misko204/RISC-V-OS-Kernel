// syscall_handler.hpp - kernel side of the system call ABI

#ifndef _syscall_handler_hpp_
#define _syscall_handler_hpp_

#include "../lib/hw.h"

struct TrapFrame;

class SyscallHandler {
public:
    // Reads the call number from a0 and the arguments from a1..a4 of the
    // saved frame, performs the call and returns the value for a0.
    static uint64 dispatch(TrapFrame* frame);

private:
    static uint64 memAlloc(TrapFrame* frame);
    static uint64 memFree(TrapFrame* frame);
    static uint64 memGetFreeSpace(TrapFrame* frame);
    static uint64 memGetLargestFreeBlock(TrapFrame* frame);

    static uint64 threadCreate(TrapFrame* frame);
    static uint64 threadExit(TrapFrame* frame);
    static uint64 threadDispatch(TrapFrame* frame);

    static uint64 semOpen(TrapFrame* frame);
    static uint64 semClose(TrapFrame* frame);
    static uint64 semWait(TrapFrame* frame);
    static uint64 semSignal(TrapFrame* frame);

    static uint64 timeSleep(TrapFrame* frame);

    static uint64 getc(TrapFrame* frame);
    static uint64 putc(TrapFrame* frame);

    // Sign-extends a (possibly negative) return value into a register.
    static uint64 fromInt(long value) { return (uint64)value; }
};

#endif // _syscall_handler_hpp_
