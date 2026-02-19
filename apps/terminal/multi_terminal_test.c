#include <stddef.h>
#include <stdint.h>

#include "apps/multi_terminal_test.h"
#include "keyboard.h"
#include "keyboard_dispatch.h"
#include "wm_compositor.h"
#include "wm_terminal_window.h"

static int mt_str_eq(const char *a, const char *b) {
  size_t i = 0u;

  if (a == NULL || b == NULL) {
    return 0;
  }

  while (a[i] != '\0' && b[i] != '\0') {
    if (a[i] != b[i]) {
      return 0;
    }
    ++i;
  }

  return a[i] == '\0' && b[i] == '\0';
}

static uint32_t mt_hash_u32(uint32_t hash, uint32_t value) {
  hash ^= (value & 0xffu);
  hash *= 16777619u;
  hash ^= ((value >> 8u) & 0xffu);
  hash *= 16777619u;
  hash ^= ((value >> 16u) & 0xffu);
  hash *= 16777619u;
  hash ^= ((value >> 24u) & 0xffu);
  hash *= 16777619u;
  return hash;
}

static uint8_t scancode_for_char(char ch) {
  switch (ch) {
  case 'a':
    return 0x1eu;
  case 'b':
    return 0x30u;
  case 'c':
    return 0x2eu;
  case 'd':
    return 0x20u;
  case 'e':
    return 0x12u;
  case 'f':
    return 0x21u;
  case 'g':
    return 0x22u;
  case 'h':
    return 0x23u;
  case 'i':
    return 0x17u;
  case 'j':
    return 0x24u;
  case 'k':
    return 0x25u;
  case 'l':
    return 0x26u;
  case 'm':
    return 0x32u;
  case 'n':
    return 0x31u;
  case 'o':
    return 0x18u;
  case 'p':
    return 0x19u;
  case 'q':
    return 0x10u;
  case 'r':
    return 0x13u;
  case 's':
    return 0x1fu;
  case 't':
    return 0x14u;
  case 'u':
    return 0x16u;
  case 'v':
    return 0x2fu;
  case 'w':
    return 0x11u;
  case 'x':
    return 0x2du;
  case 'y':
    return 0x15u;
  case 'z':
    return 0x2cu;
  case '/':
    return 0x35u;
  case ' ':
    return 0x39u;
  default:
    return 0u;
  }
}

static int emit_scancode(uint8_t scancode) {
  if (scancode == 0u) {
    return -1;
  }
  if (keyboard_handle_scancode(scancode) != 0) {
    return -1;
  }
  (void)keyboard_dispatch_pending();
  return 0;
}

static int emit_text(const char *text) {
  size_t i = 0u;

  if (text == NULL) {
    return -1;
  }

  while (text[i] != '\0') {
    if (emit_scancode(scancode_for_char(text[i])) != 0) {
      return -1;
    }
    ++i;
  }

  return 0;
}

static int emit_enter(void) { return emit_scancode(0x1cu); }

int multi_terminal_test_run(uint32_t *out_marker) {
  wm_terminal_window_t left_terminal;
  wm_terminal_window_t right_terminal;
  uint32_t marker = 2166136261u;
  int ok = 1;

  if (out_marker == (uint32_t *)0) {
    return -1;
  }
  *out_marker = 0u;

  keyboard_reset();
  keyboard_dispatch_reset();
  wm_terminal_window_dispatch_reset();
  keyboard_dispatch_set_sink(wm_terminal_window_dispatch_event);
  wm_compositor_reset(0x0011171fu);

  wm_terminal_window_init(&left_terminal, "term-left", 28u, 28u, 236u, 156u, 1u, 101u);
  wm_terminal_window_init(&right_terminal, "term-right", 108u, 76u, 236u, 156u, 2u, 202u);

  if (wm_terminal_window_attach(&left_terminal) != 0 || wm_terminal_window_attach(&right_terminal) != 0) {
    ok = 0;
    goto finish;
  }

  if (wm_compositor_activate_window(wm_terminal_window_native(&left_terminal)) != 0) {
    ok = 0;
    goto finish;
  }
  if (emit_text("cd /t") != 0) {
    ok = 0;
    goto finish;
  }
  if (wm_terminal_window_input_len(&left_terminal) != 5u || wm_terminal_window_input_len(&right_terminal) != 0u) {
    ok = 0;
    goto finish;
  }

  if (wm_compositor_activate_window(wm_terminal_window_native(&right_terminal)) != 0) {
    ok = 0;
    goto finish;
  }
  if (emit_text("pwd") != 0 || emit_enter() != 0) {
    ok = 0;
    goto finish;
  }
  if (wm_terminal_window_input_len(&left_terminal) != 5u ||
      wm_terminal_window_lines_executed(&right_terminal) != 1u ||
      wm_terminal_window_history_count(&left_terminal) != 0u || !mt_str_eq(wm_terminal_window_cwd(&right_terminal), "/")) {
    ok = 0;
    goto finish;
  }

  if (wm_compositor_activate_window(wm_terminal_window_native(&left_terminal)) != 0) {
    ok = 0;
    goto finish;
  }
  if (emit_text("mp") != 0 || emit_enter() != 0 || emit_text("pwd") != 0 || emit_enter() != 0) {
    ok = 0;
    goto finish;
  }
  if (!mt_str_eq(wm_terminal_window_cwd(&left_terminal), "/tmp") ||
      wm_terminal_window_history_count(&left_terminal) != 2u ||
      wm_terminal_window_lines_executed(&left_terminal) != 2u) {
    ok = 0;
    goto finish;
  }

  if (wm_compositor_activate_window(wm_terminal_window_native(&right_terminal)) != 0) {
    ok = 0;
    goto finish;
  }
  if (emit_text("pwd") != 0 || emit_enter() != 0) {
    ok = 0;
    goto finish;
  }

  if (!mt_str_eq(wm_terminal_window_cwd(&right_terminal), "/") ||
      wm_terminal_window_history_count(&right_terminal) != 2u ||
      wm_terminal_window_lines_executed(&right_terminal) != 2u ||
      wm_terminal_window_input_len(&left_terminal) != 0u || wm_terminal_window_input_len(&right_terminal) != 0u ||
      mt_str_eq(wm_terminal_window_cwd(&left_terminal), wm_terminal_window_cwd(&right_terminal)) != 0 ||
      wm_terminal_window_owner_task(&left_terminal) == wm_terminal_window_owner_task(&right_terminal)) {
    ok = 0;
    goto finish;
  }

  marker = mt_hash_u32(marker, wm_terminal_window_marker(&left_terminal));
  marker = mt_hash_u32(marker, wm_terminal_window_marker(&right_terminal));
  marker = mt_hash_u32(marker, wm_terminal_window_history_count(&left_terminal));
  marker = mt_hash_u32(marker, wm_terminal_window_history_count(&right_terminal));
  marker = mt_hash_u32(marker, wm_compositor_render());
  *out_marker = marker;

finish:
  keyboard_dispatch_set_sink((keyboard_dispatch_fn)0);
  keyboard_dispatch_reset();
  wm_terminal_window_dispatch_reset();
  keyboard_reset();
  wm_compositor_reset(0x00161c26u);

  if (ok == 0) {
    return -1;
  }

  return 0;
}
