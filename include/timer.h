#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

uint64_t timer_now(void);
void timer_set_deadline(uint64_t deadline);

#endif
