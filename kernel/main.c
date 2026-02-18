#include <stdint.h>

#include "console.h"
#include "framebuffer.h"
#include "keyboard_dispatch.h"
#include "line_io.h"
#include "mm_init.h"
#include "trap.h"
#include "wm_compositor.h"
#include "wm_focus.h"
#include "wm_window.h"

typedef struct keyboard_terminal_state {
  char text[16];
  uint32_t text_len;
  uint32_t enter_count;
  uint32_t backspace_count;
  uint32_t tab_count;
  uint32_t escape_count;
} keyboard_terminal_state_t;

static void console_put_hex32(uint32_t value) {
  static const char digits[] = "0123456789ABCDEF";
  int shift;

  for (shift = 28; shift >= 0; shift -= 4) {
    console_putc(digits[(value >> (uint32_t)shift) & 0x0fu]);
  }
}

static void keyboard_terminal_on_text(const wm_window_t *window, char ch, void *ctx) {
  keyboard_terminal_state_t *terminal = (keyboard_terminal_state_t *)ctx;
  (void)window;

  if (terminal == (keyboard_terminal_state_t *)0 || terminal->text_len + 1u >= sizeof(terminal->text)) {
    return;
  }

  terminal->text[terminal->text_len++] = ch;
  terminal->text[terminal->text_len] = '\0';
}

static void keyboard_terminal_on_control(const wm_window_t *window, keyboard_event_type_t control, void *ctx) {
  keyboard_terminal_state_t *terminal = (keyboard_terminal_state_t *)ctx;
  (void)window;

  if (terminal == (keyboard_terminal_state_t *)0) {
    return;
  }

  switch (control) {
    case KEYBOARD_EVENT_BACKSPACE:
      terminal->backspace_count += 1u;
      if (terminal->text_len > 0u) {
        terminal->text_len -= 1u;
        terminal->text[terminal->text_len] = '\0';
      }
      break;
    case KEYBOARD_EVENT_ENTER:
      terminal->enter_count += 1u;
      break;
    case KEYBOARD_EVENT_TAB:
      terminal->tab_count += 1u;
      break;
    case KEYBOARD_EVENT_ESCAPE:
      terminal->escape_count += 1u;
      break;
    case KEYBOARD_EVENT_TEXT:
    default:
      break;
  }
}

static int keyboard_route_byte(uint8_t byte) {
  keyboard_event_t event;

  if (keyboard_event_from_byte(byte, &event) == 0) {
    return 0;
  }

  return keyboard_dispatch_route_event(&event);
}

static int keyboard_text_equals(const keyboard_terminal_state_t *terminal, const char *text) {
  uint32_t i;

  if (terminal == (const keyboard_terminal_state_t *)0 || text == (const char *)0) {
    return 0;
  }

  for (i = 0u; i < terminal->text_len; ++i) {
    if (terminal->text[i] != text[i]) {
      return 0;
    }
  }

  return text[i] == '\0';
}

static uint32_t hash_u8(uint32_t hash, uint8_t value) {
  hash ^= (uint32_t)value;
  hash *= 16777619u;
  return hash;
}

static uint32_t keyboard_focus_hash(const keyboard_terminal_state_t *left,
                                    const keyboard_terminal_state_t *right) {
  uint32_t i;
  uint32_t hash = 2166136261u;

  if (left == (const keyboard_terminal_state_t *)0 || right == (const keyboard_terminal_state_t *)0) {
    return 0u;
  }

  for (i = 0u; i < left->text_len; ++i) {
    hash = hash_u8(hash, (uint8_t)left->text[i]);
  }
  for (i = 0u; i < right->text_len; ++i) {
    hash = hash_u8(hash, (uint8_t)right->text[i]);
  }

  hash = hash_u8(hash, (uint8_t)left->enter_count);
  hash = hash_u8(hash, (uint8_t)left->backspace_count);
  hash = hash_u8(hash, (uint8_t)left->tab_count);
  hash = hash_u8(hash, (uint8_t)left->escape_count);
  hash = hash_u8(hash, (uint8_t)right->enter_count);
  hash = hash_u8(hash, (uint8_t)right->backspace_count);
  hash = hash_u8(hash, (uint8_t)right->tab_count);
  hash = hash_u8(hash, (uint8_t)right->escape_count);
  return hash;
}

static uint32_t run_keyboard_focus_routing_test(int *ok_out) {
  int ok = 1;
  keyboard_terminal_state_t left_terminal = {{0}, 0u, 0u, 0u, 0u, 0u};
  keyboard_terminal_state_t right_terminal = {{0}, 0u, 0u, 0u, 0u, 0u};
  wm_window_t left_window;
  wm_window_t right_window;

  wm_window_init(&left_window, "Term-A", 24u, 28u, 200u, 140u);
  wm_window_init(&right_window, "Term-B", 88u, 72u, 220u, 150u);

  wm_compositor_reset(0x0010151eu);
  keyboard_dispatch_reset();

  if (wm_compositor_add_window(&left_window) != 0 || wm_compositor_add_window(&right_window) != 0) {
    ok = 0;
  }

  if (keyboard_dispatch_register_endpoint(&left_window, keyboard_terminal_on_text,
                                          keyboard_terminal_on_control, &left_terminal) != 0 ||
      keyboard_dispatch_register_endpoint(&right_window, keyboard_terminal_on_text,
                                          keyboard_terminal_on_control, &right_terminal) != 0) {
    ok = 0;
  }

  if (wm_compositor_activate_window(&left_window) != 0) {
    ok = 0;
  }

  if (keyboard_route_byte((uint8_t)'a') != 1 || keyboard_route_byte((uint8_t)'b') != 1 ||
      keyboard_route_byte(0x7fu) != 1 || keyboard_route_byte((uint8_t)'c') != 1 ||
      keyboard_route_byte((uint8_t)'\n') != 1) {
    ok = 0;
  }

  if (wm_compositor_activate_window(&right_window) != 0) {
    ok = 0;
  }

  if (keyboard_route_byte((uint8_t)'x') != 1 || keyboard_route_byte((uint8_t)'y') != 1 ||
      keyboard_route_byte((uint8_t)'\t') != 1 || keyboard_route_byte((uint8_t)'\r') != 1) {
    ok = 0;
  }

  wm_focus_set_active_window((const wm_window_t *)0);
  if (keyboard_route_byte((uint8_t)'z') != 0) {
    ok = 0;
  }

  if (keyboard_text_equals(&left_terminal, "ac") == 0 || left_terminal.enter_count != 1u ||
      left_terminal.backspace_count != 1u || left_terminal.tab_count != 0u ||
      left_terminal.escape_count != 0u) {
    ok = 0;
  }

  if (keyboard_text_equals(&right_terminal, "xy") == 0 || right_terminal.enter_count != 1u ||
      right_terminal.backspace_count != 0u || right_terminal.tab_count != 1u ||
      right_terminal.escape_count != 0u) {
    ok = 0;
  }

  if (ok_out != (int *)0) {
    *ok_out = ok;
  }

  return keyboard_focus_hash(&left_terminal, &right_terminal);
}

void kernel_main(void) {
  uint32_t marker_a;
  uint32_t marker_b;
  uint32_t wm_marker;
  uint32_t overlap_marker_before;
  uint32_t overlap_marker_after;
  uint32_t keyboard_marker;
  int overlap_ok = 0;
  int keyboard_ok = 0;
  const wm_window_t *hit_before;
  const wm_window_t *hit_after;
  const wm_window_t *active_window;
  char line[128];
  wm_window_t main_window;
  wm_window_t back_window;
  wm_window_t front_window;

  console_init();
  mm_init();
  line_io_write("BOOT: kernel entry\n");
  trap_init();
  line_io_write("console: line io ready\n");
  trap_test_trigger();

  if (framebuffer_init() != 0) {
    line_io_write("GFX: framebuffer init failed\n");
    for (;;) {
      __asm__ volatile("wfi");
    }
  }

  line_io_write("GFX: framebuffer initialized\n");

  marker_a = framebuffer_render_test_pattern();
  marker_b = framebuffer_render_test_pattern();

  if (marker_a == marker_b) {
    line_io_write("GFX: deterministic marker 0x");
    console_put_hex32(marker_a);
    line_io_write("\n");
  } else {
    line_io_write("GFX: marker mismatch 0x");
    console_put_hex32(marker_a);
    line_io_write(" != 0x");
    console_put_hex32(marker_b);
    line_io_write("\n");
  }

  wm_window_init(&main_window, "Terminal", 32u, 20u, 220u, 140u);
  main_window.style.title_bar_color = 0x002f4f89u;
  main_window.style.content_color = 0x00f8f9fbu;
  wm_compositor_reset(0x00161c26u);

  if (wm_compositor_add_window(&main_window) == 0) {
    wm_marker = wm_compositor_render();
    line_io_write("WM: single window composed marker 0x");
    console_put_hex32(wm_marker);
    line_io_write("\n");
  } else {
    line_io_write("WM: single window compose failed\n");
  }

  wm_window_init(&back_window, "Back", 48u, 40u, 220u, 160u);
  back_window.style.title_bar_color = 0x00326f95u;
  back_window.style.content_color = 0x00e9f3fbu;
  wm_window_init(&front_window, "Front", 120u, 92u, 220u, 160u);
  front_window.style.title_bar_color = 0x00814444u;
  front_window.style.content_color = 0x00f5e6deu;

  wm_compositor_reset(0x0012181fu);
  if (wm_compositor_add_window(&back_window) == 0 && wm_compositor_add_window(&front_window) == 0) {
    hit_before = wm_compositor_hit_test(140u, 120u);
    overlap_marker_before = wm_compositor_render();
    if (wm_compositor_activate_window(&back_window) == 0) {
      hit_after = wm_compositor_hit_test(140u, 120u);
      active_window = wm_compositor_active_window();
      overlap_marker_after = wm_compositor_render();
      if (hit_before == &front_window && hit_after == &back_window &&
          active_window == &back_window && overlap_marker_before != overlap_marker_after) {
        overlap_ok = 1;
      }
    }
  }

  if (overlap_ok != 0) {
    line_io_write("WM: overlap focus activation marker 0x");
    console_put_hex32(overlap_marker_after);
    line_io_write("\n");
  } else {
    line_io_write("WM: overlap focus activation failed\n");
  }

  keyboard_marker = run_keyboard_focus_routing_test(&keyboard_ok);
  if (keyboard_ok != 0) {
    line_io_write("WM: keyboard focus routing marker 0x");
    console_put_hex32(keyboard_marker);
    line_io_write("\n");
  } else {
    line_io_write("WM: keyboard focus routing failed\n");
  }

  for (;;) {
    line_io_write("shell> ");
    if (line_io_readline(line, sizeof(line), true) > 0) {
      line_io_write("echo: ");
      line_io_write(line);
      line_io_write("\n");
    }
  }
}
