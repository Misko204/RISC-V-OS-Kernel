// char_buffer.hpp - fixed-size ring buffer of characters

#ifndef _char_buffer_hpp_
#define _char_buffer_hpp_

#include "../lib/hw.h"

// Not synchronized: the caller provides mutual exclusion (the console driver
// uses it only inside the trap handler or with interrupts disabled).
template <size_t CAPACITY>
class CharBuffer {
public:
    // constexpr: static buffers are initialized at compile time.
    constexpr CharBuffer() : data{}, head(0), tail(0), count(0) { }

    // Appends a character. Returns false (and drops it) if the buffer is full.
    bool put(char c) {
        if (count == CAPACITY) return false;
        data[tail] = c;
        tail = (tail + 1) % CAPACITY;
        count++;
        return true;
    }

    // Removes and returns the oldest character. The buffer must not be empty.
    char take() {
        char c = data[head];
        head = (head + 1) % CAPACITY;
        count--;
        return c;
    }

    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count == CAPACITY; }
    size_t size() const { return count; }
    static constexpr size_t capacity() { return CAPACITY; }

private:
    char   data[CAPACITY];
    size_t head;    // next character to take
    size_t tail;    // next free slot
    size_t count;
};

#endif // _char_buffer_hpp_
