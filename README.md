# RISC-V OS Kernel

A small multithreaded, time-sharing operating system kernel for **RISC-V (RV64IMA)**,
written from scratch in C++ and assembly and running on QEMU.

## Features

- [ ] Contiguous memory allocator (first fit, block-granular, coalescing)
- [ ] Trap handling and system call ABI (`ecall`)
- [ ] Threads with context switching
- [ ] Counting semaphores
- [ ] Preemptive time sharing (timer interrupt)
- [ ] Timed sleep
- [ ] Buffered, interrupt-driven console I/O
- [ ] Layered user API: ABI → C API → C++ API

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
lib/    hardware access layer and boot code (prebuilt)
docs/   design documentation
```

## Acknowledgements

The boot code and hardware access layer in `lib/` are prebuilt from a stripped
down version of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) (MIT
License, see `LICENSE-xv6`). They were provided as course material for the
Operating Systems 1 course at the School of Electrical Engineering, University
of Belgrade. Everything in `h/` and `src/` is my own work.

## License

MIT, see [LICENSE](LICENSE). The contents of `lib/` are licensed separately,
see [LICENSE-xv6](LICENSE-xv6).