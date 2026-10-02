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
- The console is a UART behind the PLIC (IRQ 10).
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
