# RISC-V OS Kernel

A small multithreaded, time-sharing operating system kernel for **RISC-V (RV64IMA)**,
written from scratch in C++ and assembly and running on QEMU.

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
user program                         U-mode
  C++ API   (Thread, Semaphore, PeriodicThread, Console)
  C API     (mem_alloc, thread_create, sem_wait, ...)
  ABI       (ecall)
---------------------------------------------------------
  kernel                             S-mode
  hardware access layer
```

See [docs/design.md](docs/design.md) for the design decisions.

## Building and running

Requirements: `riscv64-linux-gnu-` (or `riscv64-unknown-elf-`) GCC toolchain,
`qemu-system-riscv64`, `make`, and optionally `gdb-multiarch`.

```bash
make qemu        # build and run
make qemu-gdb    # build and wait for a debugger
make clean
```

## Project layout

```
h/      kernel and API headers
src/    kernel and API sources (.cpp, .S)
test/   kernel self-tests
lib/    hardware access layer and boot code (prebuilt hw.lib)
docs/   design documentation
```

## Acknowledgements

The boot code and hardware access layer in `lib/hw.lib` (machine-mode
startup, timer forwarding, PLIC helpers) are prebuilt from a stripped down
version of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) (MIT License,
see `LICENSE-xv6`). They were provided as course material for the Operating
Systems 1 course at the School of Electrical Engineering, University of
Belgrade. Everything else, including the memory allocator and the console
driver, is my own work in `h/`, `src/` and `test/`.

## License

MIT, see [LICENSE](LICENSE). The contents of `lib/` are licensed separately,
see [LICENSE-xv6](LICENSE-xv6).
