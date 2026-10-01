// riscv.hpp - pristup sistemskim (CSR) registrima RISC-V procesora

#ifndef _riscv_hpp_
#define _riscv_hpp_

#include "../lib/hw.h"

class Riscv {
public:
    // ---- Biti registra sstatus ----
    enum BitMaskSstatus : uint64 {
        SSTATUS_SIE  = (1UL << 1),   // dozvola prekida u S rezimu
        SSTATUS_SPIE = (1UL << 5),   // prethodna vrednost SIE
        SSTATUS_SPP  = (1UL << 8),   // prethodni rezim: 0 = U, 1 = S
    };

    // ---- Biti registra sip ----
    enum BitMaskSip : uint64 {
        SIP_SSIP = (1UL << 1),       // softverski prekid (tajmer)
        SIP_SEIP = (1UL << 9),       // spoljasnji prekid (konzola)
    };

    // ---- Vrednosti registra scause ----
    static constexpr uint64 SCAUSE_TIMER         = 0x8000000000000001UL;
    static constexpr uint64 SCAUSE_EXTERNAL      = 0x8000000000000009UL;
    static constexpr uint64 SCAUSE_ECALL_USER    = 0x0000000000000008UL;
    static constexpr uint64 SCAUSE_ECALL_SUPER   = 0x0000000000000009UL;
    static constexpr uint64 SCAUSE_ILLEGAL_INSTR = 0x0000000000000002UL;
    static constexpr uint64 SCAUSE_LOAD_FAULT    = 0x0000000000000005UL;
    static constexpr uint64 SCAUSE_STORE_FAULT   = 0x0000000000000007UL;

    // ---- scause, sepc, stval, stvec, sscratch ----
    static inline uint64 r_scause();
    static inline void   w_scause(uint64 value);
    static inline uint64 r_sepc();
    static inline void   w_sepc(uint64 value);
    static inline uint64 r_stval();
    static inline uint64 r_stvec();
    static inline void   w_stvec(uint64 value);
    static inline uint64 r_sscratch();
    static inline void   w_sscratch(uint64 value);

    // ---- sstatus ----
    static inline uint64 r_sstatus();
    static inline void   w_sstatus(uint64 value);
    static inline void   ms_sstatus(uint64 mask);   // postavi bite
    static inline void   mc_sstatus(uint64 mask);   // obrisi bite

    // ---- sip ----
    static inline uint64 r_sip();
    static inline void   w_sip(uint64 value);
    static inline void   ms_sip(uint64 mask);
    static inline void   mc_sip(uint64 mask);

    // Zaustavlja emulator
    static inline void haltEmulator();
};

// ======================= implementacija =======================

inline uint64 Riscv::r_scause() {
    uint64 volatile value;
    __asm__ volatile ("csrr %[value], scause" : [value] "=r"(value));
    return value;
}
inline void Riscv::w_scause(uint64 value) {
    __asm__ volatile ("csrw scause, %[value]" : : [value] "r"(value));
}

inline uint64 Riscv::r_sepc() {
    uint64 volatile value;
    __asm__ volatile ("csrr %[value], sepc" : [value] "=r"(value));
    return value;
}
inline void Riscv::w_sepc(uint64 value) {
    __asm__ volatile ("csrw sepc, %[value]" : : [value] "r"(value));
}

inline uint64 Riscv::r_stval() {
    uint64 volatile value;
    __asm__ volatile ("csrr %[value], stval" : [value] "=r"(value));
    return value;
}

inline uint64 Riscv::r_stvec() {
    uint64 volatile value;
    __asm__ volatile ("csrr %[value], stvec" : [value] "=r"(value));
    return value;
}
inline void Riscv::w_stvec(uint64 value) {
    __asm__ volatile ("csrw stvec, %[value]" : : [value] "r"(value));
}

inline uint64 Riscv::r_sscratch() {
    uint64 volatile value;
    __asm__ volatile ("csrr %[value], sscratch" : [value] "=r"(value));
    return value;
}
inline void Riscv::w_sscratch(uint64 value) {
    __asm__ volatile ("csrw sscratch, %[value]" : : [value] "r"(value));
}

inline uint64 Riscv::r_sstatus() {
    uint64 volatile value;
    __asm__ volatile ("csrr %[value], sstatus" : [value] "=r"(value));
    return value;
}
inline void Riscv::w_sstatus(uint64 value) {
    __asm__ volatile ("csrw sstatus, %[value]" : : [value] "r"(value));
}
inline void Riscv::ms_sstatus(uint64 mask) {
    __asm__ volatile ("csrs sstatus, %[mask]" : : [mask] "r"(mask));
}
inline void Riscv::mc_sstatus(uint64 mask) {
    __asm__ volatile ("csrc sstatus, %[mask]" : : [mask] "r"(mask));
}

inline uint64 Riscv::r_sip() {
    uint64 volatile value;
    __asm__ volatile ("csrr %[value], sip" : [value] "=r"(value));
    return value;
}
inline void Riscv::w_sip(uint64 value) {
    __asm__ volatile ("csrw sip, %[value]" : : [value] "r"(value));
}
inline void Riscv::ms_sip(uint64 mask) {
    __asm__ volatile ("csrs sip, %[mask]" : : [mask] "r"(mask));
}
inline void Riscv::mc_sip(uint64 mask) {
    __asm__ volatile ("csrc sip, %[mask]" : : [mask] "r"(mask));
}

inline void Riscv::haltEmulator() {
    *(volatile unsigned int*) 0x100000 = 0x5555;
    while (true) { }
}

#endif // _riscv_hpp_