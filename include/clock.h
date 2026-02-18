#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

void clock_init(void);
void clock_handle_timer_interrupt(void);
uint64_t clock_ticks(void);

#endif
