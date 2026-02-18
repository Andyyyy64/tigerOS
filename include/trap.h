#ifndef TRAP_H
#define TRAP_H

#define TRAP_FRAME_RA 0
#define TRAP_FRAME_SP 8
#define TRAP_FRAME_GP 16
#define TRAP_FRAME_TP 24
#define TRAP_FRAME_T0 32
#define TRAP_FRAME_T1 40
#define TRAP_FRAME_T2 48
#define TRAP_FRAME_S0 56
#define TRAP_FRAME_S1 64
#define TRAP_FRAME_A0 72
#define TRAP_FRAME_A1 80
#define TRAP_FRAME_A2 88
#define TRAP_FRAME_A3 96
#define TRAP_FRAME_A4 104
#define TRAP_FRAME_A5 112
#define TRAP_FRAME_A6 120
#define TRAP_FRAME_A7 128
#define TRAP_FRAME_S2 136
#define TRAP_FRAME_S3 144
#define TRAP_FRAME_S4 152
#define TRAP_FRAME_S5 160
#define TRAP_FRAME_S6 168
#define TRAP_FRAME_S7 176
#define TRAP_FRAME_S8 184
#define TRAP_FRAME_S9 192
#define TRAP_FRAME_S10 200
#define TRAP_FRAME_S11 208
#define TRAP_FRAME_T3 216
#define TRAP_FRAME_T4 224
#define TRAP_FRAME_T5 232
#define TRAP_FRAME_T6 240
#define TRAP_FRAME_MSTATUS 248
#define TRAP_FRAME_MEPC 256
#define TRAP_FRAME_MCAUSE 264
#define TRAP_FRAME_MTVAL 272
#define TRAP_FRAME_RESERVED 280
#define TRAP_FRAME_SIZE 288

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <stdint.h>

struct trap_frame {
  uint64_t ra;
  uint64_t sp;
  uint64_t gp;
  uint64_t tp;
  uint64_t t0;
  uint64_t t1;
  uint64_t t2;
  uint64_t s0;
  uint64_t s1;
  uint64_t a0;
  uint64_t a1;
  uint64_t a2;
  uint64_t a3;
  uint64_t a4;
  uint64_t a5;
  uint64_t a6;
  uint64_t a7;
  uint64_t s2;
  uint64_t s3;
  uint64_t s4;
  uint64_t s5;
  uint64_t s6;
  uint64_t s7;
  uint64_t s8;
  uint64_t s9;
  uint64_t s10;
  uint64_t s11;
  uint64_t t3;
  uint64_t t4;
  uint64_t t5;
  uint64_t t6;
  uint64_t mstatus;
  uint64_t mepc;
  uint64_t mcause;
  uint64_t mtval;
  uint64_t reserved;
};

_Static_assert(offsetof(struct trap_frame, ra) == TRAP_FRAME_RA, "trap frame ra");
_Static_assert(offsetof(struct trap_frame, sp) == TRAP_FRAME_SP, "trap frame sp");
_Static_assert(offsetof(struct trap_frame, mepc) == TRAP_FRAME_MEPC, "trap frame mepc");
_Static_assert(offsetof(struct trap_frame, mcause) == TRAP_FRAME_MCAUSE, "trap frame mcause");
_Static_assert(sizeof(struct trap_frame) == TRAP_FRAME_SIZE, "trap frame size");

void trap_init(void);
void trap_test_trigger(void);
void trap_handle(struct trap_frame *frame);

#endif

#endif
