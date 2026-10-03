// memory_allocator.cpp - kernel heap allocator

#include "../h/memory_allocator.hpp"

MemoryAllocator::FreeSegment* MemoryAllocator::freeHead = nullptr;
uint64 MemoryAllocator::heapStart = 0;
uint64 MemoryAllocator::heapEnd = 0;

void MemoryAllocator::init() {
    // HEAP_START_ADDR directly follows the kernel image and is not guaranteed
    // to be block-aligned, so round it up. Round the end down for symmetry.
    heapStart = ((uint64)HEAP_START_ADDR + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE * MEM_BLOCK_SIZE;
    heapEnd   = (uint64)HEAP_END_ADDR / MEM_BLOCK_SIZE * MEM_BLOCK_SIZE;

    if (heapEnd <= heapStart) {
        freeHead = nullptr;
        return;
    }

    // Initially the whole heap is one free segment.
    freeHead = (FreeSegment*)heapStart;
    freeHead->size = (heapEnd - heapStart) / MEM_BLOCK_SIZE;
    freeHead->next = nullptr;
    freeHead->prev = nullptr;
}

size_t MemoryAllocator::bytesToBlocks(size_t bytes) {
    return (bytes + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
}

void* MemoryAllocator::alloc(size_t blocks) {
    if (blocks == 0) return nullptr;

    size_t needed = blocks + HEADER_BLOCKS;
    if (needed < blocks) return nullptr;            // overflow for absurd requests

    // First fit: take the first free segment that is large enough.
    for (FreeSegment* seg = freeHead; seg != nullptr; seg = seg->next) {
        if (seg->size < needed) continue;

        size_t remaining = seg->size - needed;
        if (remaining > 0) {
            // Split: the tail stays free and takes this segment's place in the list.
            FreeSegment* rest = (FreeSegment*)(address(seg) + needed * MEM_BLOCK_SIZE);
            rest->size = remaining;
            rest->prev = seg->prev;
            rest->next = seg->next;
            if (rest->prev) rest->prev->next = rest;
            else freeHead = rest;
            if (rest->next) rest->next->prev = rest;
        } else {
            // Exact fit: the whole segment is used.
            removeFree(seg);
        }

        AllocHeader* header = (AllocHeader*)seg;
        header->size = needed;
        header->magic = MAGIC;

        return (void*)(address(seg) + HEADER_BLOCKS * MEM_BLOCK_SIZE);
    }

    return nullptr;                                 // no segment is large enough
}

int MemoryAllocator::free(void* ptr) {
    if (ptr == nullptr) return ERR_NULL_PTR;

    uint64 p = address(ptr);
    if (p < heapStart + HEADER_BLOCKS * MEM_BLOCK_SIZE || p >= heapEnd) return ERR_OUT_OF_HEAP;
    if ((p - heapStart) % MEM_BLOCK_SIZE != 0) return ERR_MISALIGNED;

    AllocHeader* header = (AllocHeader*)(p - HEADER_BLOCKS * MEM_BLOCK_SIZE);
    if (header->magic != MAGIC) return ERR_NOT_ALLOCATED;

    size_t size = header->size;
    header->magic = 0;                              // makes a second free() detectable

    // The header block becomes the free-list node of this segment.
    FreeSegment* seg = (FreeSegment*)header;
    seg->size = size;
    insertFree(seg);

    return 0;
}

void MemoryAllocator::insertFree(FreeSegment* seg) {
    // Find the neighbours: prev has a lower address, next a higher one.
    FreeSegment* prev = nullptr;
    FreeSegment* next = freeHead;
    while (next != nullptr && address(next) < address(seg)) {
        prev = next;
        next = next->next;
    }

    // Link seg between them.
    seg->prev = prev;
    seg->next = next;
    if (prev) prev->next = seg;
    else freeHead = seg;
    if (next) next->prev = seg;

    // Coalesce with the right neighbour first, then with the left one, so that
    // free | freed | free collapses into a single segment owned by prev.
    if (next != nullptr && endOf(seg) == address(next)) {
        seg->size += next->size;
        removeFree(next);
    }
    if (prev != nullptr && endOf(prev) == address(seg)) {
        prev->size += seg->size;
        removeFree(seg);
    }
}

void MemoryAllocator::removeFree(FreeSegment* seg) {
    if (seg->prev) seg->prev->next = seg->next;
    else freeHead = seg->next;
    if (seg->next) seg->next->prev = seg->prev;
    seg->next = seg->prev = nullptr;
}

size_t MemoryAllocator::freeSpace() {
    size_t total = 0;
    for (FreeSegment* seg = freeHead; seg != nullptr; seg = seg->next) {
        total += seg->size * MEM_BLOCK_SIZE;
    }
    return total;
}

size_t MemoryAllocator::largestFreeBlock() {
    size_t largest = 0;
    for (FreeSegment* seg = freeHead; seg != nullptr; seg = seg->next) {
        if (seg->size > largest) largest = seg->size;
    }
    return largest * MEM_BLOCK_SIZE;
}
