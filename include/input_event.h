#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include <stdint.h>

typedef enum input_event_type {
  INPUT_EVENT_MOUSE_MOVE = 0,
  INPUT_EVENT_MOUSE_BUTTON_DOWN = 1,
  INPUT_EVENT_MOUSE_BUTTON_UP = 2,
} input_event_type_t;

enum {
  INPUT_MOUSE_BUTTON_LEFT = 1u,
};

typedef struct input_mouse_event {
  uint32_t x;
  uint32_t y;
  uint8_t buttons;
  uint8_t button;
} input_mouse_event_t;

typedef struct input_event {
  input_event_type_t type;
  input_mouse_event_t mouse;
} input_event_t;

#endif
