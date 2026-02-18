#include <stdint.h>

#include "event_queue.h"
#include "input_event.h"
#include "mouse.h"

typedef struct mouse_state {
  uint32_t x;
  uint32_t y;
  uint8_t buttons;
} mouse_state_t;

static mouse_state_t g_mouse_state;

static int mouse_emit(input_event_type_t type, uint32_t x, uint32_t y, uint8_t buttons, uint8_t button) {
  input_event_t event;

  event.type = type;
  event.mouse.x = x;
  event.mouse.y = y;
  event.mouse.buttons = buttons;
  event.mouse.button = button;
  return input_event_queue_push(&event);
}

void mouse_reset(void) {
  g_mouse_state.x = 0u;
  g_mouse_state.y = 0u;
  g_mouse_state.buttons = 0u;
  input_event_queue_reset();
}

int mouse_emit_move(uint32_t x, uint32_t y, uint8_t buttons) {
  g_mouse_state.x = x;
  g_mouse_state.y = y;
  g_mouse_state.buttons = buttons;
  return mouse_emit(INPUT_EVENT_MOUSE_MOVE, g_mouse_state.x, g_mouse_state.y, g_mouse_state.buttons, 0u);
}

int mouse_emit_button_down(uint32_t x, uint32_t y, uint8_t button_mask) {
  g_mouse_state.x = x;
  g_mouse_state.y = y;
  g_mouse_state.buttons = (uint8_t)(g_mouse_state.buttons | button_mask);
  return mouse_emit(INPUT_EVENT_MOUSE_BUTTON_DOWN, g_mouse_state.x, g_mouse_state.y, g_mouse_state.buttons,
                    button_mask);
}

int mouse_emit_button_up(uint32_t x, uint32_t y, uint8_t button_mask) {
  g_mouse_state.x = x;
  g_mouse_state.y = y;
  g_mouse_state.buttons = (uint8_t)(g_mouse_state.buttons & ((uint8_t)(~button_mask)));
  return mouse_emit(INPUT_EVENT_MOUSE_BUTTON_UP, g_mouse_state.x, g_mouse_state.y, g_mouse_state.buttons,
                    button_mask);
}
