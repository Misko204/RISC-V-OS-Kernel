// syscall_test.cpp - self-tests for the trap path and the memory system calls
// Runs in S-mode, so every ecall goes through the "trap from S-mode" path.

#include "tests.hpp"
#include "../h/syscall_c.hpp"
#include "../h/syscall_codes.hpp"
#include "../h/memory_allocator.hpp"
#include "../h/riscv.hpp"
#include "../h/kprint.hpp"

static int failures = 0;

static void check(const char* name, bool ok) {
    kprintString(ok ? "  [ OK ] " : "  [FAIL] ");
    kprintString(name);
    kprintString("\n");
    if (!ok) failures++;
}

// Issues ecall with an arbitrary code; used to test unknown system calls.
static long rawSyscall(uint64 code) {
    register uint64 a0 __asm__("a0") = code;
    __asm__ volatile ("ecall" : "+r"(a0) : : "memory");
    return (long)a0;
}

int testSystemCalls() {
    failures = 0;
    kprintString("System call tests\n");
    const size_t B = MEM_BLOCK_SIZE;

    // 1. The trap path returns, and the kernel and the C API agree on the heap state.
    size_t initialFree = mem_get_free_space();
    check("mem_get_free_space matches the allocator", initialFree == MemoryAllocator::freeSpace());
    check("mem_get_largest_free_block matches the allocator",
          mem_get_largest_free_block() == MemoryAllocator::largestFreeBlock());

    // 2. mem_alloc rounds bytes up to blocks: 100 B -> 2 blocks + 1 header block.
    char* p = (char*)mem_alloc(100);
    check("mem_alloc(100) succeeds", p != nullptr);
    check("pointer is block-aligned", (uint64)p % B == 0);
    check("mem_alloc(100) uses 3 blocks", mem_get_free_space() == initialFree - 3 * B);

    for (int i = 0; i < 100; i++) p[i] = (char)i;
    bool intact = true;
    for (int i = 0; i < 100; i++) if (p[i] != (char)i) intact = false;
    check("allocated memory is writable", intact);

    // 3. mem_free and its error codes (negative values must survive the trip through a0).
    check("mem_free returns 0", mem_free(p) == 0);
    check("free space restored", mem_get_free_space() == initialFree);
    check("double mem_free -> ERR_NOT_ALLOCATED", mem_free(p) == MemoryAllocator::ERR_NOT_ALLOCATED);
    check("mem_free(nullptr) -> ERR_NULL_PTR", mem_free(nullptr) == MemoryAllocator::ERR_NULL_PTR);

    // 4. Edge cases of mem_alloc.
    check("mem_alloc(0) -> nullptr", mem_alloc(0) == nullptr);
    check("mem_alloc(huge) -> nullptr", mem_alloc((size_t)-1) == nullptr);
    void* q = mem_alloc(B);
    check("mem_alloc(64) uses 2 blocks", mem_get_free_space() == initialFree - 2 * B);
    mem_free(q);

    // 5. Unknown system call numbers are rejected.
    check("unknown syscall -> ERR_UNKNOWN_SYSCALL", rawSyscall(0xFF) == ERR_UNKNOWN_SYSCALL);

    // 6. The trap saves and restores registers the compiler would not expect to change.
    uint64 t1Value, t2Value, a7Value;
    __asm__ volatile (
    "li t1, 0x1234\n"
    "li t2, 0x5678\n"
    "li a7, 0x9abc\n"
    "li a0, %[code]\n"
    "ecall\n"
    "mv %[t1], t1\n"
    "mv %[t2], t2\n"
    "mv %[a7], a7\n"
    : [t1] "=r"(t1Value), [t2] "=r"(t2Value), [a7] "=r"(a7Value)
    : [code] "i"(SYS_MEM_GET_FREE_SPACE)
    : "t1", "t2", "a7", "a0", "memory");
    check("registers preserved across ecall",
          t1Value == 0x1234 && t2Value == 0x5678 && a7Value == 0x9abc);

    // 7. The trap restores sstatus (interrupt enable bit unchanged).
    uint64 sieBefore = Riscv::r_sstatus() & Riscv::SSTATUS_SIE;
    mem_get_free_space();
    check("sstatus.SIE preserved across ecall", (Riscv::r_sstatus() & Riscv::SSTATUS_SIE) == sieBefore);

    check("heap intact after all tests", mem_get_free_space() == initialFree);

    kprintString(failures == 0 ? "All system call tests passed\n"
                               : "Some system call tests FAILED\n");
    return failures;
}
