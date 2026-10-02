// memory_allocator_test.cpp - self-tests for MemoryAllocator
// Assumes MemoryAllocator::init() was called and nothing is allocated yet.

#include "tests.hpp"
#include "../h/memory_allocator.hpp"
#include "../h/kprint.hpp"

static int failures = 0;

static void check(const char* name, bool ok) {
    kprintString(ok ? "  [ OK ] " : "  [FAIL] ");
    kprintString(name);
    kprintString("\n");
    if (!ok) failures++;
}

static bool isAligned(void* p) {
    return (uint64)p % MEM_BLOCK_SIZE == 0;
}

int testMemoryAllocator() {
    failures = 0;
    kprintString("MemoryAllocator tests\n");

    const size_t initialFree = MemoryAllocator::freeSpace();
    const size_t initialLargest = MemoryAllocator::largestFreeBlock();
    const size_t B = MEM_BLOCK_SIZE;

    // 1. Initial state: the heap is a single free segment.
    check("initial heap is one segment", initialFree == initialLargest && initialFree > 0);

    // 2. bytesToBlocks rounds up.
    check("bytesToBlocks", MemoryAllocator::bytesToBlocks(0) == 0 &&
                           MemoryAllocator::bytesToBlocks(1) == 1 &&
                           MemoryAllocator::bytesToBlocks(B) == 1 &&
                           MemoryAllocator::bytesToBlocks(B + 1) == 2);

    // 3. A single allocation costs the requested blocks plus one header block.
    void* p = MemoryAllocator::alloc(1);
    check("alloc(1) succeeds", p != nullptr);
    check("returned pointer is block-aligned", isAligned(p));
    check("alloc(1) uses 2 blocks", MemoryAllocator::freeSpace() == initialFree - 2 * B);

    // 4. The memory is usable.
    char* bytes = (char*)p;
    for (size_t i = 0; i < B; i++) bytes[i] = (char)i;
    bool intact = true;
    for (size_t i = 0; i < B; i++) if (bytes[i] != (char)i) intact = false;
    check("allocated memory is writable", intact);

    // 5. Freeing restores the heap.
    check("free returns 0", MemoryAllocator::free(p) == 0);
    check("free restores free space", MemoryAllocator::freeSpace() == initialFree);
    check("free restores largest block", MemoryAllocator::largestFreeBlock() == initialLargest);

    // 6. First fit reuses the lowest address.
    void* q = MemoryAllocator::alloc(1);
    check("first fit reuses freed segment", q == p);
    MemoryAllocator::free(q);

    // 7. Coalescing: free | freed | free must merge into one segment.
    void* a = MemoryAllocator::alloc(2);
    void* b = MemoryAllocator::alloc(3);
    void* c = MemoryAllocator::alloc(4);
    void* guard = MemoryAllocator::alloc(1);    // keeps c away from the big tail segment
    MemoryAllocator::free(a);
    MemoryAllocator::free(c);
    check("two holes are not merged", MemoryAllocator::largestFreeBlock() < initialLargest);
    MemoryAllocator::free(b);
    MemoryAllocator::free(guard);
    check("all segments coalesce back into one",
          MemoryAllocator::freeSpace() == initialFree &&
          MemoryAllocator::largestFreeBlock() == initialLargest);

    // 8. Error handling in free().
    void* r = MemoryAllocator::alloc(1);
    check("free(nullptr) -> ERR_NULL_PTR",
          MemoryAllocator::free(nullptr) == MemoryAllocator::ERR_NULL_PTR);
    check("free(outside heap) -> ERR_OUT_OF_HEAP",
          MemoryAllocator::free((void*)0x1000) == MemoryAllocator::ERR_OUT_OF_HEAP);
    check("free(misaligned) -> ERR_MISALIGNED",
          MemoryAllocator::free((char*)r + 8) == MemoryAllocator::ERR_MISALIGNED);
    check("free(r) succeeds", MemoryAllocator::free(r) == 0);
    check("double free -> ERR_NOT_ALLOCATED",
          MemoryAllocator::free(r) == MemoryAllocator::ERR_NOT_ALLOCATED);
    check("heap intact after bad frees", MemoryAllocator::freeSpace() == initialFree);

    // 9. Requests that cannot be satisfied.
    size_t heapBlocks = initialLargest / B;
    check("alloc(0) -> nullptr", MemoryAllocator::alloc(0) == nullptr);
    check("alloc(whole heap) -> nullptr (no room for header)",
          MemoryAllocator::alloc(heapBlocks) == nullptr);
    check("alloc(huge) -> nullptr", MemoryAllocator::alloc((size_t)-1) == nullptr);
    check("heap intact after failed allocs", MemoryAllocator::freeSpace() == initialFree);

    // 10. Exact fit of the whole heap, then nothing is left.
    void* all = MemoryAllocator::alloc(heapBlocks - 1);
    check("alloc(heap - header) succeeds", all != nullptr);
    check("heap is now full", MemoryAllocator::freeSpace() == 0);
    check("alloc on full heap -> nullptr", MemoryAllocator::alloc(1) == nullptr);
    MemoryAllocator::free(all);
    check("heap restored after full alloc", MemoryAllocator::freeSpace() == initialFree);

    // 11. Stress: many allocations of different sizes, freed in a different order.
    const int N = 100;
    char* ptrs[N];
    bool allOk = true;
    for (int i = 0; i < N; i++) {
        size_t blocks = (size_t)(i % 7) + 1;
        ptrs[i] = (char*)MemoryAllocator::alloc(blocks);
        if (ptrs[i] == nullptr || !isAligned(ptrs[i])) { allOk = false; continue; }
        ptrs[i][0] = (char)i;                           // first byte
        ptrs[i][blocks * B - 1] = (char)i;              // last byte
    }
    check("stress: all allocations succeed", allOk);

    bool noOverlap = true;
    for (int i = 0; i < N; i++) {
        size_t blocks = (size_t)(i % 7) + 1;
        if (ptrs[i] == nullptr) continue;
        if (ptrs[i][0] != (char)i || ptrs[i][blocks * B - 1] != (char)i) noOverlap = false;
    }
    check("stress: allocations do not overlap", noOverlap);

    for (int i = 0; i < N; i += 2) MemoryAllocator::free(ptrs[i]);   // even first
    for (int i = 1; i < N; i += 2) MemoryAllocator::free(ptrs[i]);   // then odd
    check("stress: heap fully coalesced",
          MemoryAllocator::freeSpace() == initialFree &&
          MemoryAllocator::largestFreeBlock() == initialLargest);

    kprintString(failures == 0 ? "All MemoryAllocator tests passed\n"
                               : "Some MemoryAllocator tests FAILED\n");
    return failures;
}