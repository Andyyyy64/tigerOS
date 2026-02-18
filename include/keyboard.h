#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

typedef enum keyboard_event_type {
  KEYBOARD_EVENT_TEXT = 0,
  KEYBOARD_EVENT_BACKSPACE = 1,
  KEYBOARD_EVENT_ENTER = 2,
  KEYBOARD_EVENT_TAB = 3,
  KEYBOARD_EVENT_ESCAPE = 4,
} keyboard_event_type_t;

typedef struct keyboard_event {
  keyboard_event_type_t type;
  char text;
} keyboard_event_t;

int keyboard_event_from_byte(uint8_t byte, keyboard_event_t *event);
int keyboard_poll_event(keyboard_event_t *event);

#endif
