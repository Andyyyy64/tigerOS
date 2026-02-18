#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

#include "trap.h"

void sched_init(void);
void sched_bootstrap_test_tasks(void);
void sched_handle_timer_interrupt(const struct trap_frame *frame);
uint32_t sched_runnable_count(void);

#endif
