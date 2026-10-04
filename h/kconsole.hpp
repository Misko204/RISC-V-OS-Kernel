// kconsole.hpp - console driver: buffered, interrupt-driven input and output

#ifndef _kconsole_hpp_
#define _kconsole_hpp_

#include "../lib/hw.h"
#include "char_buffer.hpp"

class KSemaphore;

// The console is a UART (NS16550A in QEMU's virt machine).
//
// Output: putc() puts characters into a buffer and returns at once (it blocks
// only while the buffer is full). A kernel thread takes them out and writes
// them to the UART when it is ready to accept them.
//
// Input: the console interrupt moves received characters into a second
// buffer. getc() takes them out, blocking while the buffer is empty.
class KConsole {
public:
    static constexpr size_t BUFFER_SIZE = 256;

    // Creates the semaphores and configures the UART. Call after the
    // allocator is initialized, before interrupts are enabled.
    static void init();

    // Starts the output kernel thread. Call after TCB::init().
    static void startOutputThread();

    // ---- system calls (run inside the trap handler) ----

    // Queues a character for output; blocks while the output buffer is full.
    static void putc(char c);

    // Returns the next received character (0..255), blocking while there is
    // none, or EOF on error.
    static int getc();

    // ---- interrupt ----

    // Handles the console interrupt: reads every character the UART has received.
    static void handleInterrupt();

    // Puts a received character into the input buffer (dropped if it is full).
    static void receive(char c);

    // ---- direct output for the kernel itself ----

    // Writes a character to the UART immediately, by polling. Used by kprint,
    // so it works before threads exist and inside the trap handler.
    static void writeSync(char c);

    // Writes out everything still in the output buffer, by polling.
    // Used before the machine is halted.
    static void flushSync();

    static bool outputEmpty() { return output.isEmpty(); }

private:
    static void outputThreadBody(void*);
    static void transmitOne();

    static bool txReady();
    static bool rxReady();

    static CharBuffer<BUFFER_SIZE> output;
    static CharBuffer<BUFFER_SIZE> input;

    static KSemaphore* outputSpace;     // free slots in the output buffer
    static KSemaphore* outputItems;     // characters waiting in the output buffer
    static KSemaphore* inputItems;      // characters waiting in the input buffer
};

#endif // _kconsole_hpp_
