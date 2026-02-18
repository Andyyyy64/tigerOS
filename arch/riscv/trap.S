#include "trap.h"

.section .text
.align 2
.globl trap_vector
.type trap_vector, @function

trap_vector:
	addi sp, sp, -TRAP_FRAME_SIZE

	sd ra, TRAP_FRAME_RA(sp)
	sd gp, TRAP_FRAME_GP(sp)
	sd tp, TRAP_FRAME_TP(sp)
	sd t0, TRAP_FRAME_T0(sp)
	sd t1, TRAP_FRAME_T1(sp)
	sd t2, TRAP_FRAME_T2(sp)
	sd s0, TRAP_FRAME_S0(sp)
	sd s1, TRAP_FRAME_S1(sp)
	sd a0, TRAP_FRAME_A0(sp)
	sd a1, TRAP_FRAME_A1(sp)
	sd a2, TRAP_FRAME_A2(sp)
	sd a3, TRAP_FRAME_A3(sp)
	sd a4, TRAP_FRAME_A4(sp)
	sd a5, TRAP_FRAME_A5(sp)
	sd a6, TRAP_FRAME_A6(sp)
	sd a7, TRAP_FRAME_A7(sp)
	sd s2, TRAP_FRAME_S2(sp)
	sd s3, TRAP_FRAME_S3(sp)
	sd s4, TRAP_FRAME_S4(sp)
	sd s5, TRAP_FRAME_S5(sp)
	sd s6, TRAP_FRAME_S6(sp)
	sd s7, TRAP_FRAME_S7(sp)
	sd s8, TRAP_FRAME_S8(sp)
	sd s9, TRAP_FRAME_S9(sp)
	sd s10, TRAP_FRAME_S10(sp)
	sd s11, TRAP_FRAME_S11(sp)
	sd t3, TRAP_FRAME_T3(sp)
	sd t4, TRAP_FRAME_T4(sp)
	sd t5, TRAP_FRAME_T5(sp)
	sd t6, TRAP_FRAME_T6(sp)

	addi t0, sp, TRAP_FRAME_SIZE
	sd t0, TRAP_FRAME_SP(sp)

	csrr t0, sstatus
	sd t0, TRAP_FRAME_MSTATUS(sp)
	csrr t0, sepc
	sd t0, TRAP_FRAME_MEPC(sp)
	csrr t0, scause
	sd t0, TRAP_FRAME_MCAUSE(sp)
	csrr t0, stval
	sd t0, TRAP_FRAME_MTVAL(sp)

	mv a0, sp
	call trap_handle

	ld t0, TRAP_FRAME_MEPC(sp)
	csrw sepc, t0
	ld t0, TRAP_FRAME_MSTATUS(sp)
	csrw sstatus, t0

	ld ra, TRAP_FRAME_RA(sp)
	ld gp, TRAP_FRAME_GP(sp)
	ld tp, TRAP_FRAME_TP(sp)
	ld t0, TRAP_FRAME_T0(sp)
	ld t1, TRAP_FRAME_T1(sp)
	ld t2, TRAP_FRAME_T2(sp)
	ld s0, TRAP_FRAME_S0(sp)
	ld s1, TRAP_FRAME_S1(sp)
	ld a0, TRAP_FRAME_A0(sp)
	ld a1, TRAP_FRAME_A1(sp)
	ld a2, TRAP_FRAME_A2(sp)
	ld a3, TRAP_FRAME_A3(sp)
	ld a4, TRAP_FRAME_A4(sp)
	ld a5, TRAP_FRAME_A5(sp)
	ld a6, TRAP_FRAME_A6(sp)
	ld a7, TRAP_FRAME_A7(sp)
	ld s2, TRAP_FRAME_S2(sp)
	ld s3, TRAP_FRAME_S3(sp)
	ld s4, TRAP_FRAME_S4(sp)
	ld s5, TRAP_FRAME_S5(sp)
	ld s6, TRAP_FRAME_S6(sp)
	ld s7, TRAP_FRAME_S7(sp)
	ld s8, TRAP_FRAME_S8(sp)
	ld s9, TRAP_FRAME_S9(sp)
	ld s10, TRAP_FRAME_S10(sp)
	ld s11, TRAP_FRAME_S11(sp)
	ld t3, TRAP_FRAME_T3(sp)
	ld t4, TRAP_FRAME_T4(sp)
	ld t5, TRAP_FRAME_T5(sp)
	ld t6, TRAP_FRAME_T6(sp)
	ld sp, TRAP_FRAME_SP(sp)

	sret
.size trap_vector, . - trap_vector
