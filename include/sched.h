#ifndef SCHED_H
#define SCHED_H

#include "trap.h"

void sched_init(void);
void sched_handle_timer_tick(struct trap_frame *frame);

#endif
