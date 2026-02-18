#ifndef RISCV_TIMER_H
#define RISCV_TIMER_H

#include <stdint.h>

uint64_t riscv_timer_now(void);
void riscv_timer_set_deadline(uint64_t deadline);

#endif
