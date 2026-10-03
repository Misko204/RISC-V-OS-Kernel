// trap.hpp - trap entry/exit and dispatching of system calls, interrupts and exceptions

#ifndef _trap_hpp_
#define _trap_hpp_

#include "../lib/hw.h"

// Register state saved by the trap entry code in src/trap_entry.S.
// The layout must match the offsets used there.
struct TrapFrame {
    uint64 x[32];       // general-purpose registers; x[0] is unused, x[2] is sp before the trap
    uint64 sepc;        // address of the trapping instruction
    uint64 sstatus;     // sstatus at the time of the trap

    // Indices into x[] for the registers the kernel accesses by name.
    static constexpr int RA = 1;
    static constexpr int SP = 2;
    static constexpr int A0 = 10;
    static constexpr int A1 = 11;
    static constexpr int A2 = 12;
    static constexpr int A3 = 13;
    static constexpr int A4 = 14;
};

static_assert(sizeof(TrapFrame) == 34 * 8, "TrapFrame layout must match trap_entry.S");
static_assert(sizeof(TrapFrame) % 16 == 0, "sp must stay 16-byte aligned");

class Trap {
public:
    // Installs the trap vector. Must be called before the first ecall.
    static void init();

    // Called from trap_entry.S for every trap, with interrupts disabled.
    static void handle(TrapFrame* frame);

private:
    static void handleSystemCall(TrapFrame* frame);
    static void handleTimerInterrupt();
    static void handleExternalInterrupt();
    static void handleException(TrapFrame* frame, uint64 scause);
};

// Defined in src/trap_entry.S.
extern "C" void supervisorTrap();   // trap vector
extern "C" void trapReturn();       // restores a TrapFrame from sp and executes sret

#endif // _trap_hpp_
