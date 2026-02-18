#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

typedef enum keyboard_event_type {
  KEYBOARD_EVENT_TEXT = 0,
  KEYBOARD_EVENT_CONTROL = 1,
} keyboard_event_type_t;

typedef enum keyboard_control_code {
  KEYBOARD_CONTROL_ENTER = 1,
  KEYBOARD_CONTROL_BACKSPACE = 2,
  KEYBOARD_CONTROL_TAB = 3,
  KEYBOARD_CONTROL_ESCAPE = 4,
} keyboard_control_code_t;

typedef struct keyboard_event {
  keyboard_event_type_t type;
  uint8_t scancode;
  uint8_t pressed;
  char text;
  uint8_t control;
} keyboard_event_t;

enum {
  KEYBOARD_EVENT_QUEUE_CAPACITY = 64u,
};

void keyboard_reset(void);
int keyboard_handle_scancode(uint8_t scancode);
int keyboard_pop_event(keyboard_event_t *out_event);
uint32_t keyboard_pending_count(void);

#endif
