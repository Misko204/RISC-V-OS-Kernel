// memory_allocator.hpp - kernel heap allocator
//
// Contiguous allocation with first fit, in units of MEM_BLOCK_SIZE blocks.
// Free segments form a doubly linked list sorted by address; the list nodes
// live inside the free segments themselves. Adjacent free segments are
// coalesced on every free.
//
// Every allocated segment starts with a one-block header that stores its size
// and a magic value. The caller receives the address of the block right after
// the header, so returned pointers are always MEM_BLOCK_SIZE-aligned.

#ifndef _memory_allocator_hpp_
#define _memory_allocator_hpp_

#include "../lib/hw.h"

class MemoryAllocator {
public:
    // Must be called once, before any other function.
    static void init();

    // Allocates at least `blocks` usable blocks. Returns nullptr on failure.
    static void* alloc(size_t blocks);

    // Frees a segment returned by alloc(). Returns 0 on success, or a
    // negative error code if the pointer is recognized as invalid.
    static int free(void* ptr);

    // Total free memory and largest free segment, in bytes.
    // These are raw sizes: one block of every segment goes to the header,
    // so the largest possible single allocation is one block smaller.
    static size_t freeSpace();
    static size_t largestFreeBlock();

    // Number of blocks needed to hold `bytes` bytes (rounded up).
    static size_t bytesToBlocks(size_t bytes);

    // Error codes returned by free().
    static constexpr int ERR_NULL_PTR      = -1;
    static constexpr int ERR_OUT_OF_HEAP   = -2;
    static constexpr int ERR_MISALIGNED    = -3;
    static constexpr int ERR_NOT_ALLOCATED = -4;   // not from alloc() or already freed

private:
    struct FreeSegment {
        size_t       size;     // in blocks, including the block this node occupies
        FreeSegment* next;     // next free segment (higher address)
        FreeSegment* prev;     // previous free segment (lower address)
    };

    struct AllocHeader {
        size_t size;           // in blocks, including the header block
        uint64 magic;          // MAGIC while the segment is allocated
    };

    static constexpr uint64 MAGIC = 0xA110CA7EDB10C5A1UL;
    static constexpr size_t HEADER_BLOCKS = 1;

    static FreeSegment* freeHead;   // free segment with the lowest address
    static uint64 heapStart;        // first usable address, block-aligned
    static uint64 heapEnd;          // one past the last usable address, block-aligned

    static void insertFree(FreeSegment* seg);   // insert sorted by address and coalesce
    static void removeFree(FreeSegment* seg);   // unlink from the free list

    static uint64 address(const void* p) { return (uint64)p; }
    static uint64 endOf(const FreeSegment* seg) {
        return address(seg) + seg->size * MEM_BLOCK_SIZE;
    }
};

#endif // _memory_allocator_hpp_