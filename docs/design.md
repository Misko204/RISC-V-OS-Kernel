# Design

This document describes the key architectural decisions of the kernel and the
reasoning behind them.

## Overview

A small multithreaded, time-sharing kernel for a single-core RV64IMA machine
(QEMU `virt`). The kernel and the user application are statically linked into a
single image and share one physical address space, as in an embedded system.

The kernel provides:

- a contiguous memory allocator,
- threads with preemptive time sharing,
- counting semaphores,
- timed sleep,
- buffered, interrupt-driven console I/O.

User code runs in U-mode and talks to the kernel through a layered interface:

```
user program                         U-mode
  C++ API   (Thread, Semaphore, PeriodicThread, Console, new/delete)
  C API     (mem_alloc, thread_create, sem_wait, ...)
  ABI       (ecall, arguments in a0..a7)
---------------------------------------------------------
  kernel                             S-mode
  hardware access layer (hw.lib)
```

## Decisions

### 1. Single-core, monolithic kernel

All kernel code runs in S-mode in one address space, and calls inside the kernel
are ordinary function calls. Because there is only one hart, no spin locks are
needed, there is a single ready queue, and there is no cross-CPU scheduling.

### 2. User threads are preemptible, the kernel is not

The timer and console interrupts can preempt user threads at any instruction.
The kernel itself is not preemptible:

1. On trap entry the CPU clears `sstatus.SIE` and saves the old value in `SPIE`.
2. The trap handler never re-enables interrupts.
3. `sret` restores `SIE` from `SPIE`.

The whole kernel is therefore a single critical section and needs no locks.

Kernel-internal threads (idle, console output) run in S-mode with interrupts
enabled, outside the trap handler. They must mask interrupts while touching
shared kernel data.

### 3. One kernel stack per thread

Every thread owns two stacks:

| Stack | Allocated by | Used for |
|---|---|---|
| user stack | C API `thread_create` (through `mem_alloc`) | the thread body |
| kernel stack | the kernel | trap handling for that thread |

On trap entry, `csrrw sp, sscratch, sp` switches to the current thread's kernel
stack. `sscratch` always holds the top of the running thread's kernel stack.

Why:

- **Safety.** A user thread that nearly overflows its own stack cannot corrupt
  kernel execution.
- **Blocking inside the kernel.** A thread can block deep inside a system call
  (`sem_wait`, `getc` on an empty buffer, `putc` on a full one), and its kernel
  stack is preserved until it resumes. This lets console I/O use kernel
  semaphores. A single shared kernel stack would force such calls to fail
  instead of block.

### 4. Context saving

Context is saved at two levels.

- **Trap entry/exit:** all of `x1`–`x31`, `sepc` and `sstatus` are saved on the
  kernel stack. All registers are needed because an asynchronous interrupt can
  arrive while temporaries are live.
- **Thread switch:** `contextSwitch(old, new)`, written in assembly, saves `ra`,
  `sp` and `s0`–`s11` into the TCB. `contextSwitch` is called like a normal
  function, so under the calling convention the caller-saved registers are
  already preserved by its caller. The callee-saved registers must be kept per
  thread; otherwise a caller holding values in `s` registers would observe
  another thread's values after the switch.

### 5. Scheduling

The ready queue is FIFO with time slicing. A thread receives
`DEFAULT_TIME_SLICE` timer ticks when it is scheduled. When the slice expires,
the thread goes to the back of the queue.

An idle thread spins in S-mode and is never placed in the ready queue. The
scheduler returns it only when the queue is empty, so there is always a running
thread and the switch path has no special case.

### 6. Allocating kernel objects

The global `operator new` is routed to the `mem_alloc` system call, as the user
API requires. If the kernel used it, it would issue an `ecall` to itself from
inside the trap handler.

Instead, kernel classes (`TCB`, `KSemaphore`) overload class-level
`operator new` and `operator delete`, which call `MemoryAllocator` directly.
Kernel stacks are allocated the same way. Objects are allocated dynamically, so
there is no fixed limit on the number of threads or semaphores.

### 7. Lists

Queues are intrusive: link pointers live inside the `TCB`, so no list nodes are
allocated. A thread is in at most one queue at a time (ready, a semaphore's
blocked queue, or the sleep list).

### 8. Naming

| Layer | Names |
|---|---|
| kernel | `Riscv`, `MemoryAllocator`, `TCB`, `Scheduler`, `KSemaphore`, `KConsole` |
| C++ API | `Thread`, `Semaphore`, `PeriodicThread`, `Console` |
| C API handles | `thread_t` (`_thread*`), `sem_t` (`_sem*`) |

API names are fixed, so kernel classes use distinct names (`TCB`, the `K`
prefix) to avoid clashes at link time.

## Platform notes

- `HEAP_START_ADDR` is defined by the boot code and follows the end of the
  kernel image, so it moves as the code grows. It is **not** guaranteed to be
  aligned to `MEM_BLOCK_SIZE` (observed: `0x80005690`), so the allocator aligns
  it up.
- `HEAP_END_ADDR` is `0x88000000` (128 MiB of RAM starting at `0x80000000`).
- The timer interrupt arrives as a supervisor software interrupt
  (`scause = 0x8000000000000001`) at 10 Hz.
- The console is an NS16550A UART at `0x10000000`, behind the PLIC (IRQ 10).
- Writing `0x5555` to `0x100000` powers off QEMU.

## Memory allocator

`MemoryAllocator` (`h/memory_allocator.hpp`) manages the heap between
`HEAP_START_ADDR` and `HEAP_END_ADDR` in units of `MEM_BLOCK_SIZE` (64 B)
blocks.

- **Algorithm:** first fit over a free list sorted by address. A free segment
  larger than needed is split; its tail stays in the list.
- **Free list:** doubly linked, with nodes `{size, next, prev}` stored inside
  the free segments themselves, so bookkeeping costs no extra memory.
- **Coalescing:** on every `free`, the segment is inserted by address and merged
  with its right and then its left neighbour if they are adjacent. A fully
  freed heap is always a single segment again.
- **Allocated segments:** the first block holds a header `{size, magic}`. The
  caller gets the address of the next block, so returned pointers are always
  block-aligned. This costs one block per allocation.
- **Validation in `free`:** null pointer, address outside the heap, misaligned
  address, and a missing magic value (not allocated, or freed twice) are each
  reported with a distinct negative error code. The magic value is cleared on
  `free`, so double frees are detected.
- **Statistics:** `freeSpace()` and `largestFreeBlock()` report raw free bytes.
  Because of the header block, the largest single allocation that can succeed
  is one block smaller than `largestFreeBlock()`.
- **Initialization:** `init()` must be called explicitly at boot. There is no
  C runtime, so constructors of global objects are never run; the allocator
  therefore uses static members and constant initializers only.

## Traps and system calls

All traps (system calls, exceptions and interrupts) enter the kernel through a
single routine, `supervisorTrap` in `src/trap_entry.S`, installed in `stvec` in
direct mode.

**Entry and exit.** `sscratch` tells the entry code where the trap came from:
it holds the top of the current thread's kernel stack while U-mode code runs,
and 0 while S-mode code runs.

1. `csrrw sp, sscratch, sp` swaps the two. A nonzero result means the trap
   came from U-mode and `sp` is now the kernel stack; otherwise the swap is
   undone and the current S-mode stack is used.
2. All registers `x1`–`x31`, `sepc` and `sstatus` are saved as a `TrapFrame`
   on the stack, and `sscratch` is set to 0.
3. `Trap::handle` dispatches on `scause`.
4. `sepc` and `sstatus` are restored from the frame. If `sstatus.SPP` says the
   trap returns to U-mode, `sscratch` is set to the top of the kernel stack
   again. Then all registers are restored and `sret` returns.

Saving `sepc` and `sstatus` in the frame, rather than leaving them in the CSRs,
lets the handler switch threads: each thread returns through its own frame.

**Dispatch.**

| `scause` | Meaning | Action |
|---|---|---|
| 8, 9 | `ecall` from U-mode / S-mode | `sepc += 4`, run the system call, result in `a0` |
| interrupt 1 | timer (forwarded as a software interrupt) | clear `sip.SSIP`, `Timer::tick()` |
| interrupt 9 | external (console via PLIC) | `plic_claim`, `KConsole::handleInterrupt()`, `plic_complete` |
| anything else | exception | print `scause`, `sepc`, `stval`, mode, and halt |

**ABI.** `a0` holds the call number, `a1`–`a4` the arguments, and the result
is returned in `a0`. Negative `int` results are sign-extended so they survive
the trip through a 64-bit register. Unknown call numbers return
`ERR_UNKNOWN_SYSCALL`. `mem_alloc` takes its size in blocks at this level.

**C API.** `src/syscall_c.cpp` wraps each call in a function. A single helper
binds the arguments to `a0`–`a4` with register variables and executes
`ecall`. `mem_alloc` converts bytes to blocks before the call.

## Threads

A thread is represented by a `TCB` (`h/tcb.hpp`). It holds the thread's saved
`Context`, its stacks, its mode (user or supervisor), its state
(`READY`, `RUNNING`, `BLOCKED`, `FINISHED`) and a `next` link used by whichever
list the thread is currently in.

**Kinds of threads.**

| Thread | Mode | Stacks |
|---|---|---|
| user threads (`thread_create`, including `userMain`) | U | user stack (allocated by the C API) + kernel stack |
| `main` | S | the boot stack; it becomes a thread in `TCB::init()` |
| idle | S | kernel stack only |
| console output | S | kernel stack only (see Console) |

**Switching.** `TCB::dispatch()` puts the running thread at the back of the
ready queue (unless it has finished), takes the first ready thread and calls
`contextSwitch(current, next)` (`src/context_switch.S`). That routine saves
`ra`, `sp` and `s0`–`s11` into the current TCB and loads them from the next
one, so `ret` continues on the next thread's kernel stack. Since `dispatch`
always runs inside a system call or interrupt, every other register of the
thread is already in its `TrapFrame`.

**Starting a thread.** A new thread has never trapped, so the kernel fakes it:
it builds a `TrapFrame` at the top of the new kernel stack with
`sepc = threadWrapper`, `a0 = body`, `a1 = arg`, `sp` = the thread's stack top,
and `sstatus.SPP`/`SPIE` set for the thread's mode with interrupts enabled.
The thread's saved context points to that frame with `ra = trapReturn`, the
exit half of the trap routine. The first switch to the thread therefore
"returns" from a trap straight into `threadWrapper`, which calls `body(arg)`
and then `thread_exit()`.

**Scheduling.** `Scheduler` is a FIFO queue linked through `TCB::next`. The
idle thread is never queued; `Scheduler::get()` returns it only when the queue
is empty.

**Termination.** `thread_exit` marks the thread `FINISHED` and switches away.
The thread cannot free its own kernel stack while running on it, so it is
moved to a list of finished threads; every `dispatch` frees the finished
threads other than the running one, together with their stacks.

**Startup and shutdown.** `main` initializes the allocator, the trap vector
and the console, runs the kernel-mode self-tests, calls `TCB::init()`, starts
the console output thread and starts `userMain` as a user thread. It then waits on a
kernel semaphore that `TCB::exit()` signals when the last user thread
finishes, and powers off the machine. While it waits, main is blocked and
uses no CPU time.

**System calls.**

| Call | ABI arguments | Notes |
|---|---|---|
| `thread_create` (0x11) | `a1` handle, `a2` body, `a3` arg, `a4` stack top | the C API allocates the `DEFAULT_STACK_SIZE` stack first and frees it if the call fails |
| `thread_exit` (0x12) | none | does not return |
| `thread_dispatch` (0x13) | none | |

## Semaphores

`KSemaphore` (`h/ksemaphore.hpp`) is a counting semaphore with a FIFO queue of
blocked threads.

- **`wait`:** decrements the value. If it becomes negative, the running thread
  is appended to the semaphore's queue and `TCB::block()` marks it `BLOCKED`
  and switches away. A blocked thread is in no ready queue and uses no CPU time.
- **`signal`:** increments the value. If it was negative, the first waiting
  thread is moved back to the ready queue with `TCB::unblock(thread, 0)`.
- **`close`:** unblocks every waiting thread with `ERR_SEMAPHORE_CLOSED`,
  which becomes the return value of their `sem_wait`, and frees the semaphore.

Whatever `unblock` passes is stored in the TCB and returned by `block()` in the
woken thread. That is how a wait learns whether it succeeded or the semaphore
was closed.

`ThreadQueue` (`h/thread_queue.hpp`) is the intrusive FIFO used both by the
scheduler's ready queue and by every semaphore. A thread is in at most one of
them at a time, so the single `TCB::next` link is enough. Its constructor is
`constexpr`, so the scheduler's static queue is initialized at compile time.

**Handles.** `sem_t` is the address of the `KSemaphore`. The object carries a
magic value that is cleared when it is destroyed. The kernel rejects null
handles and handles without the magic value with `ERR_INVALID_ARGUMENT`.

| Call | ABI arguments | Returns |
|---|---|---|
| `sem_open` (0x21) | `a1` handle pointer, `a2` initial value | 0, `ERR_INVALID_ARGUMENT`, `ERR_OUT_OF_MEMORY` |
| `sem_close` (0x22) | `a1` handle | 0, `ERR_INVALID_ARGUMENT` |
| `sem_wait` (0x23) | `a1` handle | 0, `ERR_SEMAPHORE_CLOSED`, `ERR_INVALID_ARGUMENT` |
| `sem_signal` (0x24) | `a1` handle | 0, `ERR_INVALID_ARGUMENT` |

## Time sharing and sleeping

The timer interrupt arrives 10 times per second. `Timer::tick()`
(`src/timer.cpp`) does two things, in this order:

1. **Wakes sleeping threads** whose time is up. They go to the ready queue.
2. **Charges the running thread** one tick (`TCB::timerTick()`). When a thread
   has used `DEFAULT_TIME_SLICE` ticks (2, i.e. 0.2 s) since it was scheduled,
   `dispatch()` preempts it and it goes to the back of the ready queue. Every
   `dispatch` starts a fresh slice for the thread it schedules.

Waking sleepers first means a thread whose sleep ends can be scheduled at the
same tick.

**Why preemption is safe here.** The interrupt is handled like any other trap:
the full register state of the interrupted thread is already in its
`TrapFrame`, so switching away inside the handler and resuming later is the
same as for a blocking system call. User code can be preempted anywhere. In
S-mode only the idle thread runs with interrupts enabled. The trap handler
itself always runs with interrupts disabled, so the kernel is never preempted.

**Sleeping.** `time_sleep(n)` puts the running thread into a sleep list
ordered by wake-up time and blocks it. The list is a *delta list*: each entry
stores the ticks between the previous entry's wake-up and its own. A tick only
decrements the first entry and wakes every leading entry that reached 0, so it
costs O(1) unless threads actually wake up. Inserting walks the list. Using
`<=` while walking keeps threads with equal wake-up times in FIFO order.
`time_sleep(0)` returns immediately.

```
sleep(A, 6), sleep(B, 2), sleep(C, 4)   ->   B:2 -> C:2 -> A:2
```

The list is linked through `TCB::next`. A sleeping thread is `BLOCKED` and
in no other queue, so the link is free to use.

| Call | ABI arguments | Returns |
|---|---|---|
| `time_sleep` (0x31) | `a1` ticks | 0 |

## Console

`KConsole` (`h/kconsole.hpp`) is the console driver. It decouples the threads
from the UART with two ring buffers (`CharBuffer`, 256 characters each), so
that no system call ever busy-waits for the hardware.

```
putc ──> [ output buffer ] ──> output thread ──> UART TX
                                    (polls the "ready to send" bit)

UART RX ──> console interrupt ──> [ input buffer ] ──> getc
```

**Output.** The `putc` system call waits on `outputSpace`, a semaphore that
counts free slots, appends the character and signals `outputItems`. It
returns at once unless the buffer is full; then the caller blocks until there
is room. A supervisor-mode kernel thread waits on `outputItems`. When the UART
can take a character, it moves one from the buffer to the transmit register
and signals `outputSpace`. While the UART is busy the thread yields instead
of spinning.

**Input.** On a console interrupt the trap handler asks the PLIC which device
interrupted (`plic_claim`). For the console it reads every character the UART
has received into the input buffer, signalling `inputItems` for each one, and
then acknowledges the PLIC (`plic_complete`). The interrupt cannot wait, so
characters that do not fit into a full buffer are dropped. The `getc` system
call waits on `inputItems` and takes the oldest character.

**Mutual exclusion.** The trap handler uses the buffers with interrupts
disabled. The output thread runs outside it, so it disables interrupts while
it takes a character and writes it to the UART. On a single core that is
enough.

**UART interrupts.** The boot code enables both the "data received" and
the "transmitter empty" UART interrupts. Output is driven by the thread, so
`KConsole::init()` leaves only the receive interrupt enabled. Otherwise an
idle transmitter would keep the interrupt line raised.

**Kernel output.** `kprint` does not go through the buffer. It writes
straight to the UART by polling (`KConsole::writeSync`), so it works before
any thread exists, inside the trap handler and while reporting an exception.
To keep the order of messages, `main` waits until the output buffer is empty
before its final message. The exception handler first writes out the buffer
by polling (`flushSync`).

**No borrowed drivers.** The kernel is linked only with `hw.lib`, which
provides the boot code, the timer and the PLIC helpers. The allocator and the
console driver are the kernel's own.

| Call | ABI arguments | Returns |
|---|---|---|
| `getc` (0x41) | none | the character (0–255), or `EOF` |
| `putc` (0x42) | `a1` character | 0 |

## C++ API

`h/syscall_cpp.hpp` is the object-oriented layer on top of the C API. Every
object stores the handle of the kernel object it represents and forwards its
operations to the matching C function, which performs the system call.

```
Thread::start()  ->  thread_create()  ->  ecall SYS_THREAD_CREATE  ->  TCB::createUserThread()
```

- **`new` / `delete`.** The global operators (also the array forms) call
  `mem_alloc` / `mem_free`. Kernel classes (`TCB`, `KSemaphore`) define their
  own class-level operators, so the kernel never reaches these.
- **`Thread`.** A thread runs either the function passed to the constructor,
  or, for classes that use the protected default constructor, the virtual
  `run()`. Nothing runs before `start()`. A run()-based thread is started
  through a static `runWrapper` that receives the object as its argument and
  calls `run()` on it. If a function was given to the constructor, `run()` is
  ignored even when a derived class overrides it. `start()` on an already
  started thread fails. The kernel frees the thread when it finishes, so the
  destructor releases nothing. A run()-based thread needs its object to stay
  alive while it runs.
- **`Semaphore`.** The constructor opens a kernel semaphore and the destructor
  closes it. Threads still waiting are released and their `wait()` fails.
- **`PeriodicThread`.** The constructor passes a static `periodicBody` and
  `this` to `Thread`'s function constructor, so `run()` stays free. The body
  calls `periodicActivation()` and sleeps `period` ticks, until `terminate()`.
  The class may not get extra data members, so `terminate()` marks the thread
  by setting `period` to 0. The thread sees it when it next wakes up, and
  finishes within one period.
- **`Console`.** Static wrappers around `getc` and `putc`.

The class layouts follow the API specification: no extra non-static data
members and no extra virtual functions. Only private static helpers were added.

**Limitation.** There is no C runtime, so constructors of global (static
storage) objects are never run. Create API objects with `new` or as local
variables.
