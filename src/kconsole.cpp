// kconsole.cpp - console driver: buffered, interrupt-driven input and output

#include "../h/kconsole.hpp"
#include "../h/ksemaphore.hpp"
#include "../h/tcb.hpp"
#include "../h/riscv.hpp"
#include "../h/syscall_c.hpp"

CharBuffer<KConsole::BUFFER_SIZE> KConsole::output;
CharBuffer<KConsole::BUFFER_SIZE> KConsole::input;
KSemaphore* KConsole::outputSpace = nullptr;
KSemaphore* KConsole::outputItems = nullptr;
KSemaphore* KConsole::inputItems  = nullptr;

// ---------------------------------------------------------------- UART access

// hw.h gives the addresses of the status, receive and transmit registers.
// The interrupt enable register is not exported; on the NS16550A it is the
// register right after the data register.
static const uint64 UART_IER_OFFSET = 1;
static const uint8  UART_IER_RX_ONLY = 0x01;   // "received data available" only

static inline uint8 readRegister(uint64 address) {
    return *(volatile uint8*)address;
}

static inline void writeRegister(uint64 address, uint8 value) {
    *(volatile uint8*)address = value;
}

bool KConsole::txReady() {
    return readRegister(CONSOLE_STATUS) & CONSOLE_TX_STATUS_BIT;
}

bool KConsole::rxReady() {
    return readRegister(CONSOLE_STATUS) & CONSOLE_RX_STATUS_BIT;
}

// ---------------------------------------------------------------- setup

void KConsole::init() {
    outputSpace = KSemaphore::create(BUFFER_SIZE);
    outputItems = KSemaphore::create(0);
    inputItems  = KSemaphore::create(0);

    // The boot code enables both the "received data" and the "transmitter
    // empty" interrupts. Output is done by the output thread, so keep only the
    // receive interrupt; otherwise an idle transmitter would keep the
    // interrupt line raised.
    writeRegister(CONSOLE_TX_DATA + UART_IER_OFFSET, UART_IER_RX_ONLY);
}

void KConsole::startOutputThread() {
    TCB::createKernelThread(outputThreadBody, nullptr);
}

// ---------------------------------------------------------------- output

void KConsole::putc(char c) {
    outputSpace->wait();            // blocks while the buffer is full
    output.put(c);
    outputItems->signal();
}

// Runs as a supervisor-mode kernel thread, outside the trap handler, with
// interrupts enabled. It waits through the normal system calls.
void KConsole::outputThreadBody(void*) {
    while (true) {
        sem_wait((sem_t)outputItems);       // until there is something to send
        while (!txReady()) thread_dispatch();
        transmitOne();
        sem_signal((sem_t)outputSpace);
    }
}

// Moves one character from the buffer to the UART. The buffer is shared with
// the trap handler, so interrupts are disabled while it is touched.
void KConsole::transmitOne() {
    uint64 sstatus = Riscv::r_sstatus();
    Riscv::mc_sstatus(Riscv::SSTATUS_SIE);

    while (!txReady()) { }                  // writeSync may have just used the UART
    writeRegister(CONSOLE_TX_DATA, (uint8)output.take());

    if (sstatus & Riscv::SSTATUS_SIE) Riscv::ms_sstatus(Riscv::SSTATUS_SIE);
}

void KConsole::writeSync(char c) {
    uint64 sstatus = Riscv::r_sstatus();
    Riscv::mc_sstatus(Riscv::SSTATUS_SIE);

    while (!txReady()) { }
    writeRegister(CONSOLE_TX_DATA, (uint8)c);

    if (sstatus & Riscv::SSTATUS_SIE) Riscv::ms_sstatus(Riscv::SSTATUS_SIE);
}

void KConsole::flushSync() {
    uint64 sstatus = Riscv::r_sstatus();
    Riscv::mc_sstatus(Riscv::SSTATUS_SIE);

    while (!output.isEmpty()) {
        while (!txReady()) { }
        writeRegister(CONSOLE_TX_DATA, (uint8)output.take());
    }

    if (sstatus & Riscv::SSTATUS_SIE) Riscv::ms_sstatus(Riscv::SSTATUS_SIE);
}

// ---------------------------------------------------------------- input

int KConsole::getc() {
    if (inputItems->wait() != 0) return EOF;
    return (unsigned char)input.take();
}

void KConsole::handleInterrupt() {
    while (rxReady()) {
        receive((char)readRegister(CONSOLE_RX_DATA));
    }
}

void KConsole::receive(char c) {
    // The interrupt cannot wait for space, so characters that do not fit are dropped.
    if (input.put(c)) inputItems->signal();
}
