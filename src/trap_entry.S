# trap_entry.S - the single entry point for all traps: system calls, exceptions and interrupts
#
# Stack convention (sscratch):
#   - while U-mode code runs, sscratch holds the top of the current thread's kernel stack;
#   - while S-mode code runs, sscratch is 0.
# A trap from U-mode therefore switches to the kernel stack, while a trap from
# S-mode keeps using the current stack.
#
# The saved registers form a TrapFrame (h/trap.hpp) on the stack.

.equ FRAME_SIZE,  34 * 8
.equ SEPC_OFF,    32 * 8
.equ SSTATUS_OFF, 33 * 8
.equ SSTATUS_SPP, 1 << 8

.extern trapHandler

.text
.align 4
.global supervisorTrap
.type supervisorTrap, @function
supervisorTrap:
    # Swap sp and sscratch. Nonzero means we came from U-mode and sp is now the kernel stack.
    csrrw sp, sscratch, sp
    bnez sp, 1f
    # We came from S-mode: undo the swap, so sp is unchanged and sscratch is 0 again.
    csrrw sp, sscratch, sp
1:
    addi sp, sp, -FRAME_SIZE

    # Save x1 and x3..x31. x2 (sp) is saved separately below.
    sd x1, 1 * 8(sp)
    .irp i, 3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    sd x\i, \i * 8(sp)
    .endr

    # sp before the trap: the user sp (now in sscratch), or sp + FRAME_SIZE when coming from S-mode.
    csrr t0, sscratch
    bnez t0, 2f
    addi t0, sp, FRAME_SIZE
2:
    sd t0, 2 * 8(sp)
    csrw sscratch, zero             # we are in the kernel now

    csrr t0, sepc
    sd t0, SEPC_OFF(sp)
    csrr t0, sstatus
    sd t0, SSTATUS_OFF(sp)

    mv a0, sp                       # trapHandler(TrapFrame* frame)
    call trapHandler

    ld t0, SEPC_OFF(sp)
    csrw sepc, t0
    ld t0, SSTATUS_OFF(sp)
    csrw sstatus, t0

    # Returning to U-mode (SPP == 0): sscratch must point to the top of this kernel stack.
    andi t1, t0, SSTATUS_SPP
    bnez t1, 3f
    addi t1, sp, FRAME_SIZE
    csrw sscratch, t1
3:
    ld x1, 1 * 8(sp)
    .irp i, 3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    ld x\i, \i * 8(sp)
    .endr
    ld sp, 2 * 8(sp)                # restore sp last
    sret
