#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>

#include "input_event.h"

enum {
  INPUT_EVENT_QUEUE_CAPACITY = 64u,
};

void input_event_queue_reset(void);
int input_event_queue_push(const input_event_t *event);
int input_event_queue_pop(input_event_t *out_event);
uint32_t input_event_queue_count(void);

#endif
