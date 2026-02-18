#include <stdint.h>

#include "console.h"
#include "keyboard.h"

int keyboard_event_from_byte(uint8_t byte, keyboard_event_t *event) {
  if (event == (keyboard_event_t *)0) {
    return 0;
  }

  if (byte == (uint8_t)'\r' || byte == (uint8_t)'\n') {
    event->type = KEYBOARD_EVENT_ENTER;
    event->text = '\0';
    return 1;
  }

  if (byte == (uint8_t)'\b' || byte == 0x7fu) {
    event->type = KEYBOARD_EVENT_BACKSPACE;
    event->text = '\0';
    return 1;
  }

  if (byte == (uint8_t)'\t') {
    event->type = KEYBOARD_EVENT_TAB;
    event->text = '\0';
    return 1;
  }

  if (byte == 0x1bu) {
    event->type = KEYBOARD_EVENT_ESCAPE;
    event->text = '\0';
    return 1;
  }

  if (byte >= 0x20u && byte <= 0x7eu) {
    event->type = KEYBOARD_EVENT_TEXT;
    event->text = (char)byte;
    return 1;
  }

  return 0;
}

int keyboard_poll_event(keyboard_event_t *event) {
  int byte;

  if (event == (keyboard_event_t *)0) {
    return 0;
  }

  for (;;) {
    byte = console_getc_nonblocking();
    if (byte < 0) {
      return 0;
    }

    if (keyboard_event_from_byte((uint8_t)byte, event) != 0) {
      return 1;
    }
  }
}
