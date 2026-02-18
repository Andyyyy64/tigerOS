#ifndef TRAP_H
#define TRAP_H

#include <stdbool.h>
#include <stdint.h>

enum {
  RISCV_GPR_COUNT = 32,
  RISCV_MCAUSE_INTERRUPT_BIT = 1ull << 63,
  RISCV_MCAUSE_CODE_MASK = ~RISCV_MCAUSE_INTERRUPT_BIT,
  RISCV_CAUSE_BREAKPOINT = 3u,
};

struct riscv_trap_frame {
  uint64_t x[RISCV_GPR_COUNT];
  uint64_t mstatus;
  uint64_t mepc;
  uint64_t mcause;
  uint64_t mtval;
};

_Static_assert(sizeof(struct riscv_trap_frame) == 288,
               "trap frame size must match arch/riscv/trap.S");

void trap_init(void);
void trap_dispatch(struct riscv_trap_frame *frame);
void trap_trigger_self_test(void);

#endif
