// trap.cpp - dispatching of system calls, interrupts and exceptions

#include "../h/trap.hpp"
#include "../h/riscv.hpp"
#include "../h/kprint.hpp"
#include "../h/syscall_handler.hpp"
#include "../h/timer.hpp"
#include "../h/kconsole.hpp"

// Called from trap_entry.S. extern "C" keeps the symbol name unmangled.
extern "C" void trapHandler(TrapFrame* frame) {
    Trap::handle(frame);
}

void Trap::init() {
    Riscv::w_sscratch(0);                           // we are running in S-mode
    Riscv::w_stvec((uint64)&supervisorTrap);        // direct mode: all traps go to one address
}

void Trap::handle(TrapFrame* frame) {
    uint64 scause = Riscv::r_scause();

    switch (scause) {
        case Riscv::SCAUSE_ECALL_USER:
        case Riscv::SCAUSE_ECALL_SUPER:
            handleSystemCall(frame);
            break;
        case Riscv::SCAUSE_TIMER:
            handleTimerInterrupt();
            break;
        case Riscv::SCAUSE_EXTERNAL:
            handleExternalInterrupt();
            break;
        default:
            handleException(frame, scause);
            break;
    }
}

void Trap::handleSystemCall(TrapFrame* frame) {
    frame->sepc += 4;                               // resume after the ecall instruction
    frame->x[TrapFrame::A0] = SyscallHandler::dispatch(frame);
}

void Trap::handleTimerInterrupt() {
    Riscv::mc_sip(Riscv::SIP_SSIP);     // acknowledge
    Timer::tick();                      // may switch to another thread
}

void Trap::handleExternalInterrupt() {
    // Ask the interrupt controller (PLIC) which device interrupted, handle it,
    // and tell the PLIC we are done so it can deliver the next one.
    int irq = plic_claim();
    if (irq == (int)CONSOLE_IRQ) KConsole::handleInterrupt();
    if (irq != 0) plic_complete(irq);
}

void Trap::handleException(TrapFrame* frame, uint64 scause) {
    KConsole::flushSync();              // print whatever user output is still buffered
    kprintString("\n*** Unhandled exception ***\n");
    kprintString("scause = ");  kprintUInt(scause, 16);         kprintString("\n");
    kprintString("sepc   = ");  kprintUInt(frame->sepc, 16);    kprintString("\n");
    kprintString("stval  = ");  kprintUInt(Riscv::r_stval(), 16); kprintString("\n");
    kprintString("mode   = ");
    kprintString(frame->sstatus & Riscv::SSTATUS_SPP ? "supervisor\n" : "user\n");
    Riscv::haltEmulator();
}
