// syscall_codes.hpp - system call numbers (ABI), shared by the kernel and the C API

#ifndef _syscall_codes_hpp_
#define _syscall_codes_hpp_

#include "../lib/hw.h"

enum SyscallCode : uint64 {
    SYS_MEM_ALLOC                  = 0x01,
    SYS_MEM_FREE                   = 0x02,
    SYS_MEM_GET_FREE_SPACE         = 0x03,
    SYS_MEM_GET_LARGEST_FREE_BLOCK = 0x04,

    SYS_THREAD_CREATE              = 0x11,
    SYS_THREAD_EXIT                = 0x12,
    SYS_THREAD_DISPATCH            = 0x13,

    SYS_SEM_OPEN                   = 0x21,
    SYS_SEM_CLOSE                  = 0x22,
    SYS_SEM_WAIT                   = 0x23,
    SYS_SEM_SIGNAL                 = 0x24,

    SYS_TIME_SLEEP                 = 0x31,

    SYS_GETC                       = 0x41,
    SYS_PUTC                       = 0x42,
};

// Returned in a0 for an unknown system call number.
static const long ERR_UNKNOWN_SYSCALL = -100;

#endif // _syscall_codes_hpp_
