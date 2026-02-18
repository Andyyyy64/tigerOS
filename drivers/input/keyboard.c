#include <stdint.h>

#include "keyboard.h"

typedef struct keyboard_state {
  keyboard_event_t events[KEYBOARD_EVENT_QUEUE_CAPACITY];
  uint32_t read_index;
  uint32_t write_index;
  uint32_t count;
  uint8_t shift_mask;
  uint8_t saw_extended_prefix;
} keyboard_state_t;

enum {
  KEYBOARD_SCANCODE_EXTENDED_PREFIX = 0xe0u,
  KEYBOARD_SCANCODE_LEFT_SHIFT = 0x2au,
  KEYBOARD_SCANCODE_RIGHT_SHIFT = 0x36u,
  KEYBOARD_SCANCODE_BREAK_BIT = 0x80u,
};

static keyboard_state_t g_keyboard_state;

static int keyboard_queue_push(const keyboard_event_t *event) {
  if (event == (const keyboard_event_t *)0) {
    return -1;
  }

  if (g_keyboard_state.count >= KEYBOARD_EVENT_QUEUE_CAPACITY) {
    return -1;
  }

  g_keyboard_state.events[g_keyboard_state.write_index] = *event;
  g_keyboard_state.write_index += 1u;
  if (g_keyboard_state.write_index >= KEYBOARD_EVENT_QUEUE_CAPACITY) {
    g_keyboard_state.write_index = 0u;
  }

  g_keyboard_state.count += 1u;
  return 0;
}

static uint8_t keyboard_shift_down(void) { return (uint8_t)(g_keyboard_state.shift_mask != 0u); }

static uint8_t keyboard_control_for_scancode(uint8_t scancode) {
  switch (scancode) {
  case 0x01u:
    return KEYBOARD_CONTROL_ESCAPE;
  case 0x0eu:
    return KEYBOARD_CONTROL_BACKSPACE;
  case 0x0fu:
    return KEYBOARD_CONTROL_TAB;
  case 0x1cu:
    return KEYBOARD_CONTROL_ENTER;
  default:
    return 0u;
  }
}

static char keyboard_printable_for_scancode(uint8_t scancode, uint8_t shift_down) {
  switch (scancode) {
  case 0x02u:
    return shift_down ? '!' : '1';
  case 0x03u:
    return shift_down ? '@' : '2';
  case 0x04u:
    return shift_down ? '#' : '3';
  case 0x05u:
    return shift_down ? '$' : '4';
  case 0x06u:
    return shift_down ? '%' : '5';
  case 0x07u:
    return shift_down ? '^' : '6';
  case 0x08u:
    return shift_down ? '&' : '7';
  case 0x09u:
    return shift_down ? '*' : '8';
  case 0x0au:
    return shift_down ? '(' : '9';
  case 0x0bu:
    return shift_down ? ')' : '0';
  case 0x0cu:
    return shift_down ? '_' : '-';
  case 0x0du:
    return shift_down ? '+' : '=';
  case 0x10u:
    return shift_down ? 'Q' : 'q';
  case 0x11u:
    return shift_down ? 'W' : 'w';
  case 0x12u:
    return shift_down ? 'E' : 'e';
  case 0x13u:
    return shift_down ? 'R' : 'r';
  case 0x14u:
    return shift_down ? 'T' : 't';
  case 0x15u:
    return shift_down ? 'Y' : 'y';
  case 0x16u:
    return shift_down ? 'U' : 'u';
  case 0x17u:
    return shift_down ? 'I' : 'i';
  case 0x18u:
    return shift_down ? 'O' : 'o';
  case 0x19u:
    return shift_down ? 'P' : 'p';
  case 0x1au:
    return shift_down ? '{' : '[';
  case 0x1bu:
    return shift_down ? '}' : ']';
  case 0x1eu:
    return shift_down ? 'A' : 'a';
  case 0x1fu:
    return shift_down ? 'S' : 's';
  case 0x20u:
    return shift_down ? 'D' : 'd';
  case 0x21u:
    return shift_down ? 'F' : 'f';
  case 0x22u:
    return shift_down ? 'G' : 'g';
  case 0x23u:
    return shift_down ? 'H' : 'h';
  case 0x24u:
    return shift_down ? 'J' : 'j';
  case 0x25u:
    return shift_down ? 'K' : 'k';
  case 0x26u:
    return shift_down ? 'L' : 'l';
  case 0x27u:
    return shift_down ? ':' : ';';
  case 0x28u:
    return shift_down ? '"' : '\'';
  case 0x29u:
    return shift_down ? '~' : '`';
  case 0x2bu:
    return shift_down ? '|' : '\\';
  case 0x2cu:
    return shift_down ? 'Z' : 'z';
  case 0x2du:
    return shift_down ? 'X' : 'x';
  case 0x2eu:
    return shift_down ? 'C' : 'c';
  case 0x2fu:
    return shift_down ? 'V' : 'v';
  case 0x30u:
    return shift_down ? 'B' : 'b';
  case 0x31u:
    return shift_down ? 'N' : 'n';
  case 0x32u:
    return shift_down ? 'M' : 'm';
  case 0x33u:
    return shift_down ? '<' : ',';
  case 0x34u:
    return shift_down ? '>' : '.';
  case 0x35u:
    return shift_down ? '?' : '/';
  case 0x39u:
    return ' ';
  default:
    return '\0';
  }
}

void keyboard_reset(void) {
  g_keyboard_state.read_index = 0u;
  g_keyboard_state.write_index = 0u;
  g_keyboard_state.count = 0u;
  g_keyboard_state.shift_mask = 0u;
  g_keyboard_state.saw_extended_prefix = 0u;
}

int keyboard_handle_scancode(uint8_t scancode) {
  uint8_t is_break;
  uint8_t keycode;
  keyboard_event_t event;
  uint8_t control;
  char printable;

  if (scancode == KEYBOARD_SCANCODE_EXTENDED_PREFIX) {
    g_keyboard_state.saw_extended_prefix = 1u;
    return 0;
  }

  if (g_keyboard_state.saw_extended_prefix != 0u) {
    g_keyboard_state.saw_extended_prefix = 0u;
    return 0;
  }

  is_break = (uint8_t)((scancode & KEYBOARD_SCANCODE_BREAK_BIT) != 0u);
  keycode = (uint8_t)(scancode & (uint8_t)(~KEYBOARD_SCANCODE_BREAK_BIT));

  if (keycode == KEYBOARD_SCANCODE_LEFT_SHIFT) {
    if (is_break != 0u) {
      g_keyboard_state.shift_mask = (uint8_t)(g_keyboard_state.shift_mask & (uint8_t)(~0x01u));
    } else {
      g_keyboard_state.shift_mask = (uint8_t)(g_keyboard_state.shift_mask | 0x01u);
    }
    return 0;
  }

  if (keycode == KEYBOARD_SCANCODE_RIGHT_SHIFT) {
    if (is_break != 0u) {
      g_keyboard_state.shift_mask = (uint8_t)(g_keyboard_state.shift_mask & (uint8_t)(~0x02u));
    } else {
      g_keyboard_state.shift_mask = (uint8_t)(g_keyboard_state.shift_mask | 0x02u);
    }
    return 0;
  }

  if (is_break != 0u) {
    return 0;
  }

  event.scancode = keycode;
  event.pressed = 1u;
  event.text = '\0';
  event.control = 0u;

  control = keyboard_control_for_scancode(keycode);
  if (control != 0u) {
    event.type = KEYBOARD_EVENT_CONTROL;
    event.control = control;
    return keyboard_queue_push(&event);
  }

  printable = keyboard_printable_for_scancode(keycode, keyboard_shift_down());
  if (printable == '\0') {
    return 0;
  }

  event.type = KEYBOARD_EVENT_TEXT;
  event.text = printable;
  return keyboard_queue_push(&event);
}

int keyboard_pop_event(keyboard_event_t *out_event) {
  if (out_event == (keyboard_event_t *)0) {
    return -1;
  }

  if (g_keyboard_state.count == 0u) {
    return -1;
  }

  *out_event = g_keyboard_state.events[g_keyboard_state.read_index];
  g_keyboard_state.read_index += 1u;
  if (g_keyboard_state.read_index >= KEYBOARD_EVENT_QUEUE_CAPACITY) {
    g_keyboard_state.read_index = 0u;
  }

  g_keyboard_state.count -= 1u;
  return 0;
}

uint32_t keyboard_pending_count(void) { return g_keyboard_state.count; }
