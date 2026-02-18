#include <stdint.h>

#include "event_queue.h"

typedef struct input_event_queue {
  input_event_t events[INPUT_EVENT_QUEUE_CAPACITY];
  uint32_t read_index;
  uint32_t write_index;
  uint32_t count;
} input_event_queue_t;

static input_event_queue_t g_input_event_queue;

void input_event_queue_reset(void) {
  g_input_event_queue.read_index = 0u;
  g_input_event_queue.write_index = 0u;
  g_input_event_queue.count = 0u;
}

int input_event_queue_push(const input_event_t *event) {
  if (event == (const input_event_t *)0) {
    return -1;
  }

  if (g_input_event_queue.count >= INPUT_EVENT_QUEUE_CAPACITY) {
    return -1;
  }

  g_input_event_queue.events[g_input_event_queue.write_index] = *event;
  g_input_event_queue.write_index += 1u;
  if (g_input_event_queue.write_index >= INPUT_EVENT_QUEUE_CAPACITY) {
    g_input_event_queue.write_index = 0u;
  }

  g_input_event_queue.count += 1u;
  return 0;
}

int input_event_queue_pop(input_event_t *out_event) {
  if (out_event == (input_event_t *)0) {
    return -1;
  }

  if (g_input_event_queue.count == 0u) {
    return -1;
  }

  *out_event = g_input_event_queue.events[g_input_event_queue.read_index];
  g_input_event_queue.read_index += 1u;
  if (g_input_event_queue.read_index >= INPUT_EVENT_QUEUE_CAPACITY) {
    g_input_event_queue.read_index = 0u;
  }

  g_input_event_queue.count -= 1u;
  return 0;
}

uint32_t input_event_queue_count(void) { return g_input_event_queue.count; }
