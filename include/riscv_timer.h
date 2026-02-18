#ifndef RISCV_TIMER_H
#define RISCV_TIMER_H

#include <stdint.h>

uint64_t riscv_timer_now(void);
uint64_t riscv_timer_read_time(void);
void riscv_timer_set_deadline(uint64_t deadline);
void riscv_timer_enable_interrupts(void);

#endif
