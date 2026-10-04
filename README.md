# RISC-V OS Kernel

A small multithreaded, time-sharing operating system kernel for **RISC-V (RV64IMA)**,
written from scratch in C++ and assembly and running on QEMU.

User programs run in user mode and reach the kernel only through system calls.
The kernel schedules threads preemptively, synchronizes them with semaphores,
lets them sleep, and drives the console with its own interrupt-driven driver.

## Features

- [x] Contiguous memory allocator (first fit, block-granular, coalescing)
- [x] Trap handling and system call ABI (`ecall`)
- [x] Threads with context switching
- [x] Counting semaphores
- [x] Preemptive time sharing (timer interrupt)
- [x] Timed sleep
- [x] Buffered, interrupt-driven console I/O
- [x] Layered user API: ABI → C API → C++ API

## Architecture

```
user program  (app/)                                  U-mode
  C++ API   Thread, Semaphore, PeriodicThread, Console, new/delete
  C API     mem_alloc, thread_create, sem_wait, time_sleep, getc, putc, ...
  ABI       ecall: a0 = call number, a1..a4 = arguments, result in a0
-----------------------------------------------------------------------
  kernel  (src/)                                      S-mode
  Trap · SyscallHandler · MemoryAllocator · TCB · Scheduler
  KSemaphore · Timer · KConsole
-----------------------------------------------------------------------
  hw.lib    boot code, timer forwarding, PLIC helpers
```

Highlights:

- **One kernel stack per thread.** A thread can block deep inside a system
  call, for example in `getc` on an empty buffer, without blocking the kernel.
- **Single trap entry** (`src/trap_entry.S`). A new thread starts by
  "returning" from a fake trap frame built for it.
- **Preemption** on a 10 Hz timer with a 2-tick time slice. Sleeping threads
  are kept in a delta list.
- **Console driver** with input and output ring buffers, a kernel output
  thread and PLIC-routed keyboard interrupts.
- **No borrowed allocator or drivers.** Only the boot and hardware-access
  layer (`hw.lib`) is prebuilt.

The design decisions and how each part works are described in
[docs/design.md](docs/design.md).

## Building and running

Requirements: a `riscv64-linux-gnu-` (or `riscv64-unknown-elf-`) GCC
toolchain, `qemu-system-riscv64`, `make`, and optionally `gdb-multiarch`.

```bash
make qemu        # build and run
make qemu-gdb    # build and wait for a debugger
make clean
```

At start-up the kernel runs its kernel-mode self-tests and then starts
`userMain`, which asks whether to run the user-mode self-tests or the demo.
The machine powers off when the last user thread finishes.

## Demo

`app/demo.cpp` uses only the C++ API. First, five philosopher threads share
five forks (semaphores) while a periodic thread prints the time. Then the
keyboard is echoed back until `q` is entered.

```
Type t or d and press Enter: d

--- Dining philosophers: 5 threads, 5 forks, 3 meals each ---
  [clock 0.0 s]
  philosopher 0 eats    (meal 1/3)
  philosopher 3 eats    (meal 1/3)
  philosopher 1 eats    (meal 1/3)
  philosopher 4 eats    (meal 1/3)
  [clock 0.5 s]
  philosopher 2 eats    (meal 1/3)
  philosopher 0 eats    (meal 2/3)
  ...
  philosopher 2 is done
  all philosophers have eaten

--- Keyboard: type a line and press Enter (q ends the demo) ---
> hello world
  HELLO WORLD  (11 characters)
> q

Demo finished

All user threads finished, shutting down
```

## Tests

104 automated checks, plus one interactive keyboard test:

| Suite | Mode | Checks |
|---|---|---|
| Memory allocator | kernel | 29 |
| System calls and trap path | kernel | 17 |
| Console buffers | kernel | 3 |
| Threads | user | 13 |
| Semaphores | user | 16 |
| Timer, preemption and sleep | user | 8 |
| Console output | user | 2 |
| C++ API | user | 16 |
| Keyboard echo (interactive) | user | 1 |

The tests check behaviour as well as results. Examples: FIFO wake-up order,
updates lost without a mutex and none lost with one, busy threads that never
yield still sharing the CPU, no memory leaked after 50 threads or 100
semaphores, and `putc` blocking on a full buffer and resuming.

## Project layout

```
h/      kernel and API headers
src/    kernel and API sources (.cpp, .S)
app/    the user program: userMain and the demo
test/   self-tests (kernel mode and user mode)
lib/    hardware access layer and boot code (prebuilt hw.lib)
docs/   design documentation
```

## Toolchain note

The `Makefile` targets the toolchain this project was developed with (GCC and
binutils from the Ubuntu-based development VM). Newer GCC versions require the
CSR extension to be named explicitly: use `-march=rv64ima_zicsr_zifencei`, and
`-g` instead of `-ggdb` in `ASFLAGS`. Very recent binutils may also reject the
prebuilt `hw.lib` at link time.

## Acknowledgements

The boot code and hardware access layer in `lib/hw.lib` (machine-mode
startup, timer forwarding, PLIC helpers) are prebuilt from a stripped down
version of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) (MIT License,
see `LICENSE-xv6`). They were provided as course material for the Operating
Systems 1 course at the School of Electrical Engineering, University of
Belgrade. The API specification comes from the same course. Everything else,
including the memory allocator and the console driver, is my own work in
`h/`, `src/`, `app/` and `test/`.

## License

MIT, see [LICENSE](LICENSE). The contents of `lib/` are licensed separately,
see [LICENSE-xv6](LICENSE-xv6).
