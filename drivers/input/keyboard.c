#include <stdint.h>

#include "console.h"
#include "keyboard.h"

static keyboard_decoder_state_t g_decoder_state;
static uint8_t g_decoder_mode;

enum {
  KEYBOARD_DECODER_MODE_AUTO = 0u,
  KEYBOARD_DECODER_MODE_ASCII = 1u,
  KEYBOARD_DECODER_MODE_SCANCODE = 2u,
};

static int ascii_control_event_from_byte(uint8_t byte, keyboard_event_t *event) {
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

  return 0;
}

static int is_alpha_char(char ch) { return ch >= 'a' && ch <= 'z'; }

static char apply_ascii_shift(char ch, uint8_t shift_down, uint8_t caps_lock_on) {
  uint8_t apply_shift = shift_down;

  if (is_alpha_char(ch) != 0) {
    if ((shift_down ^ caps_lock_on) != 0u) {
      return (char)(ch - ('a' - 'A'));
    }
    return ch;
  }

  if (apply_shift == 0u) {
    return ch;
  }

  switch (ch) {
    case '1':
      return '!';
    case '2':
      return '@';
    case '3':
      return '#';
    case '4':
      return '$';
    case '5':
      return '%';
    case '6':
      return '^';
    case '7':
      return '&';
    case '8':
      return '*';
    case '9':
      return '(';
    case '0':
      return ')';
    case '-':
      return '_';
    case '=':
      return '+';
    case '[':
      return '{';
    case ']':
      return '}';
    case '\\':
      return '|';
    case ';':
      return ':';
    case '\'':
      return '"';
    case ',':
      return '<';
    case '.':
      return '>';
    case '/':
      return '?';
    case '`':
      return '~';
    default:
      return ch;
  }
}

static char scancode_to_ascii(uint8_t scancode) {
  switch (scancode) {
    case 0x02u:
      return '1';
    case 0x03u:
      return '2';
    case 0x04u:
      return '3';
    case 0x05u:
      return '4';
    case 0x06u:
      return '5';
    case 0x07u:
      return '6';
    case 0x08u:
      return '7';
    case 0x09u:
      return '8';
    case 0x0au:
      return '9';
    case 0x0bu:
      return '0';
    case 0x0cu:
      return '-';
    case 0x0du:
      return '=';
    case 0x10u:
      return 'q';
    case 0x11u:
      return 'w';
    case 0x12u:
      return 'e';
    case 0x13u:
      return 'r';
    case 0x14u:
      return 't';
    case 0x15u:
      return 'y';
    case 0x16u:
      return 'u';
    case 0x17u:
      return 'i';
    case 0x18u:
      return 'o';
    case 0x19u:
      return 'p';
    case 0x1au:
      return '[';
    case 0x1bu:
      return ']';
    case 0x1eu:
      return 'a';
    case 0x1fu:
      return 's';
    case 0x20u:
      return 'd';
    case 0x21u:
      return 'f';
    case 0x22u:
      return 'g';
    case 0x23u:
      return 'h';
    case 0x24u:
      return 'j';
    case 0x25u:
      return 'k';
    case 0x26u:
      return 'l';
    case 0x27u:
      return ';';
    case 0x28u:
      return '\'';
    case 0x29u:
      return '`';
    case 0x2bu:
      return '\\';
    case 0x2cu:
      return 'z';
    case 0x2du:
      return 'x';
    case 0x2eu:
      return 'c';
    case 0x2fu:
      return 'v';
    case 0x30u:
      return 'b';
    case 0x31u:
      return 'n';
    case 0x32u:
      return 'm';
    case 0x33u:
      return ',';
    case 0x34u:
      return '.';
    case 0x35u:
      return '/';
    case 0x39u:
      return ' ';
    default:
      return '\0';
  }
}

static int keyboard_is_probable_scancode(uint8_t byte) {
  if (byte >= 0x80u || byte == 0xe0u || byte == 0xe1u) {
    return 1;
  }

  if (byte < 0x20u && byte != (uint8_t)'\r' && byte != (uint8_t)'\n' && byte != (uint8_t)'\t' &&
      byte != (uint8_t)'\b' && byte != 0x1bu) {
    return 1;
  }

  if (byte == 0x2au || byte == 0x36u || byte == 0x3au || byte == 0xaau || byte == 0xb6u) {
    return 1;
  }

  return 0;
}

void keyboard_decoder_reset(keyboard_decoder_state_t *state) {
  if (state == (keyboard_decoder_state_t *)0) {
    return;
  }

  state->left_shift_down = 0u;
  state->right_shift_down = 0u;
  state->caps_lock_on = 0u;
  state->extended_prefix = 0u;
}

void keyboard_input_reset(void) {
  keyboard_decoder_reset(&g_decoder_state);
  g_decoder_mode = KEYBOARD_DECODER_MODE_AUTO;
}

int keyboard_event_from_scancode(uint8_t scancode, keyboard_decoder_state_t *state, keyboard_event_t *event) {
  uint8_t released;
  uint8_t key;
  uint8_t shift_down;
  char ch;

  if (state == (keyboard_decoder_state_t *)0 || event == (keyboard_event_t *)0) {
    return 0;
  }

  if (scancode == 0xe0u || scancode == 0xe1u) {
    state->extended_prefix = 1u;
    return 0;
  }

  if (state->extended_prefix != 0u) {
    state->extended_prefix = 0u;
    return 0;
  }

  released = (uint8_t)((scancode & 0x80u) != 0u ? 1u : 0u);
  key = (uint8_t)(scancode & 0x7fu);

  if (key == 0x2au) {
    state->left_shift_down = (uint8_t)(released == 0u ? 1u : 0u);
    return 0;
  }

  if (key == 0x36u) {
    state->right_shift_down = (uint8_t)(released == 0u ? 1u : 0u);
    return 0;
  }

  if (key == 0x3au) {
    if (released == 0u) {
      state->caps_lock_on ^= 1u;
    }
    return 0;
  }

  if (released != 0u) {
    return 0;
  }

  if (key == 0x0eu) {
    event->type = KEYBOARD_EVENT_BACKSPACE;
    event->text = '\0';
    return 1;
  }

  if (key == 0x1cu) {
    event->type = KEYBOARD_EVENT_ENTER;
    event->text = '\0';
    return 1;
  }

  if (key == 0x0fu) {
    event->type = KEYBOARD_EVENT_TAB;
    event->text = '\0';
    return 1;
  }

  if (key == 0x01u) {
    event->type = KEYBOARD_EVENT_ESCAPE;
    event->text = '\0';
    return 1;
  }

  ch = scancode_to_ascii(key);
  if (ch == '\0') {
    return 0;
  }

  shift_down = (uint8_t)((state->left_shift_down | state->right_shift_down) != 0u ? 1u : 0u);
  event->type = KEYBOARD_EVENT_TEXT;
  event->text = apply_ascii_shift(ch, shift_down, state->caps_lock_on);
  return 1;
}

int keyboard_event_from_byte(uint8_t byte, keyboard_event_t *event) {
  if (event == (keyboard_event_t *)0) {
    return 0;
  }

  if (g_decoder_mode == KEYBOARD_DECODER_MODE_SCANCODE) {
    return keyboard_event_from_scancode(byte, &g_decoder_state, event);
  }

  if (g_decoder_mode == KEYBOARD_DECODER_MODE_AUTO && keyboard_is_probable_scancode(byte) != 0) {
    keyboard_decoder_reset(&g_decoder_state);
    g_decoder_mode = KEYBOARD_DECODER_MODE_SCANCODE;
    return keyboard_event_from_scancode(byte, &g_decoder_state, event);
  }

  g_decoder_mode = KEYBOARD_DECODER_MODE_ASCII;

  if (ascii_control_event_from_byte(byte, event) != 0) {
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
