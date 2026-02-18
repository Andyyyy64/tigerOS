#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"
#include "keyboard_dispatch.h"
#include "tty_session.h"
#include "wm_compositor.h"
#include "wm_terminal_window.h"

static wm_terminal_manager_t *g_bound_manager;
static wm_terminal_manager_t g_terminal_manager;

static int term_streq(const char *a, const char *b) {
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

  return a[i] == b[i];
}

static uint32_t term_hash_byte(uint32_t hash, uint8_t value) {
  hash ^= (uint32_t)value;
  hash *= 16777619u;
  return hash;
}

static uint32_t term_hash_u32(uint32_t hash, uint32_t value) {
  hash = term_hash_byte(hash, (uint8_t)(value & 0xffu));
  hash = term_hash_byte(hash, (uint8_t)((value >> 8u) & 0xffu));
  hash = term_hash_byte(hash, (uint8_t)((value >> 16u) & 0xffu));
  hash = term_hash_byte(hash, (uint8_t)((value >> 24u) & 0xffu));
  return hash;
}

static uint32_t term_hash_cstr(uint32_t hash, const char *s) {
  size_t i;

  if (s == NULL) {
    return term_hash_byte(hash, 0u);
  }

  for (i = 0u; s[i] != '\0'; ++i) {
    hash = term_hash_byte(hash, (uint8_t)s[i]);
  }

  return term_hash_byte(hash, 0u);
}

static wm_terminal_window_t *find_terminal_by_endpoint(wm_terminal_manager_t *manager,
                                                        uint32_t endpoint_id) {
  uint32_t i;

  if (manager == (wm_terminal_manager_t *)0 || endpoint_id == 0u) {
    return (wm_terminal_window_t *)0;
  }

  for (i = 0u; i < manager->terminal_count; ++i) {
    if (manager->terminals[i].endpoint_id == endpoint_id) {
      return &manager->terminals[i];
    }
  }

  return (wm_terminal_window_t *)0;
}

static int inject_scancode_sequence(const uint8_t *scancodes, size_t count) {
  size_t i;

  if (scancodes == NULL) {
    return -1;
  }

  for (i = 0u; i < count; ++i) {
    if (keyboard_handle_scancode(scancodes[i]) != 0) {
      return -1;
    }
  }

  return 0;
}

void wm_terminal_manager_reset(wm_terminal_manager_t *manager) {
  uint32_t i;

  if (manager == (wm_terminal_manager_t *)0) {
    return;
  }

  manager->terminal_count = 0u;
  for (i = 0u; i < WM_TERMINAL_MAX_WINDOWS; ++i) {
    manager->terminals[i].endpoint_id = 0u;
    manager->terminals[i].window.title = "";
    manager->terminals[i].window.frame.x = 0u;
    manager->terminals[i].window.frame.y = 0u;
    manager->terminals[i].window.frame.width = 0u;
    manager->terminals[i].window.frame.height = 0u;
    manager->terminals[i].window.style.border_color = 0u;
    manager->terminals[i].window.style.title_bar_color = 0u;
    manager->terminals[i].window.style.content_color = 0u;
    manager->terminals[i].window.style.border_thickness = 0u;
    manager->terminals[i].window.style.title_bar_height = 0u;
    tty_session_init(&manager->terminals[i].session, i + 1u);
  }
}

int wm_terminal_manager_add(wm_terminal_manager_t *manager,
                            const char *title,
                            uint32_t endpoint_id,
                            uint32_t x,
                            uint32_t y,
                            uint32_t width,
                            uint32_t height,
                            wm_terminal_window_t **out_terminal) {
  wm_terminal_window_t *terminal;

  if (out_terminal != (wm_terminal_window_t **)0) {
    *out_terminal = (wm_terminal_window_t *)0;
  }

  if (manager == (wm_terminal_manager_t *)0 || endpoint_id == 0u) {
    return -1;
  }

  if (manager->terminal_count >= WM_TERMINAL_MAX_WINDOWS) {
    return -1;
  }

  if (find_terminal_by_endpoint(manager, endpoint_id) != (wm_terminal_window_t *)0) {
    return -1;
  }

  terminal = &manager->terminals[manager->terminal_count];
  wm_window_init(&terminal->window, title, x, y, width, height);
  terminal->window.style.title_bar_color = 0x00396d8cu + (manager->terminal_count * 0x000a0805u);
  terminal->window.style.content_color = 0x00f0f4f8u - (manager->terminal_count * 0x00050503u);
  terminal->endpoint_id = endpoint_id;
  tty_session_init(&terminal->session, endpoint_id);
  manager->terminal_count += 1u;

  if (out_terminal != (wm_terminal_window_t **)0) {
    *out_terminal = terminal;
  }

  return 0;
}

void wm_terminal_manager_bind(wm_terminal_manager_t *manager) { g_bound_manager = manager; }

void wm_terminal_keyboard_sink(uint32_t endpoint_id, const keyboard_event_t *event) {
  wm_terminal_window_t *terminal;

  if (event == (const keyboard_event_t *)0 || g_bound_manager == (wm_terminal_manager_t *)0) {
    return;
  }

  terminal = find_terminal_by_endpoint(g_bound_manager, endpoint_id);
  if (terminal == (wm_terminal_window_t *)0) {
    return;
  }

  (void)tty_session_handle_keyboard_event(&terminal->session, event);
}

int wm_terminal_multi_session_test(uint32_t *out_marker) {
  wm_terminal_window_t *left_terminal = (wm_terminal_window_t *)0;
  wm_terminal_window_t *right_terminal = (wm_terminal_window_t *)0;
  char left_cwd[FS_PATH_MAX];
  char right_cwd[FS_PATH_MAX];
  uint32_t processed;
  uint32_t expected_events;
  uint32_t compositor_marker;
  uint32_t marker;

  static const uint8_t seq_cd_home[] = {0x2eu, 0x20u, 0x39u, 0x23u, 0x18u, 0x32u, 0x12u, 0x1cu};
  static const uint8_t seq_pwd[] = {0x19u, 0x11u, 0x20u, 0x1cu};

  if (out_marker == (uint32_t *)0) {
    return -1;
  }
  *out_marker = 0u;

  wm_terminal_manager_reset(&g_terminal_manager);
  wm_terminal_manager_bind(&g_terminal_manager);
  keyboard_reset();
  keyboard_dispatch_reset();
  keyboard_dispatch_set_sink(wm_terminal_keyboard_sink);
  wm_compositor_reset(0x00141a25u);

  if (wm_terminal_manager_add(&g_terminal_manager, "Term A", 1u, 32u, 34u, 228u, 146u, &left_terminal) !=
          0 ||
      wm_terminal_manager_add(&g_terminal_manager, "Term B", 2u, 128u, 86u, 228u, 146u, &right_terminal) !=
          0) {
    wm_terminal_manager_bind((wm_terminal_manager_t *)0);
    return -1;
  }

  if (wm_compositor_add_window(&left_terminal->window) != 0 ||
      wm_compositor_add_window(&right_terminal->window) != 0 ||
      keyboard_dispatch_register_window(&left_terminal->window, left_terminal->endpoint_id) != 0 ||
      keyboard_dispatch_register_window(&right_terminal->window, right_terminal->endpoint_id) != 0) {
    wm_terminal_manager_bind((wm_terminal_manager_t *)0);
    return -1;
  }

  if (inject_scancode_sequence(seq_cd_home, sizeof(seq_cd_home)) != 0) {
    wm_terminal_manager_bind((wm_terminal_manager_t *)0);
    return -1;
  }

  if (wm_compositor_activate_window(&left_terminal->window) != 0 ||
      inject_scancode_sequence(seq_pwd, sizeof(seq_pwd)) != 0) {
    wm_terminal_manager_bind((wm_terminal_manager_t *)0);
    return -1;
  }

  if (wm_compositor_activate_window(&right_terminal->window) != 0 ||
      inject_scancode_sequence(seq_pwd, sizeof(seq_pwd)) != 0) {
    wm_terminal_manager_bind((wm_terminal_manager_t *)0);
    return -1;
  }

  processed = keyboard_dispatch_pending();
  expected_events = (uint32_t)(sizeof(seq_cd_home) + sizeof(seq_pwd) + sizeof(seq_pwd));

  if (processed != expected_events || keyboard_pending_count() != 0u) {
    wm_terminal_manager_bind((wm_terminal_manager_t *)0);
    return -1;
  }

  if (tty_session_get_cwd(&left_terminal->session, left_cwd, sizeof(left_cwd)) != 0 ||
      tty_session_get_cwd(&right_terminal->session, right_cwd, sizeof(right_cwd)) != 0 ||
      term_streq(left_cwd, "/") == 0 || term_streq(right_cwd, "/home") == 0 ||
      tty_session_command_count(&left_terminal->session) != 1u ||
      tty_session_command_count(&right_terminal->session) != 2u ||
      tty_session_history_count(&left_terminal->session) != 1u ||
      tty_session_history_count(&right_terminal->session) != 2u ||
      tty_session_input_length(&left_terminal->session) != 0u ||
      tty_session_input_length(&right_terminal->session) != 0u ||
      wm_compositor_active_window() != &right_terminal->window) {
    wm_terminal_manager_bind((wm_terminal_manager_t *)0);
    return -1;
  }

  compositor_marker = wm_compositor_render();

  marker = 2166136261u;
  marker = term_hash_u32(marker, processed);
  marker = term_hash_u32(marker, tty_session_command_count(&left_terminal->session));
  marker = term_hash_u32(marker, tty_session_command_count(&right_terminal->session));
  marker = term_hash_u32(marker, tty_session_history_count(&left_terminal->session));
  marker = term_hash_u32(marker, tty_session_history_count(&right_terminal->session));
  marker = term_hash_u32(marker, g_terminal_manager.terminal_count);
  marker = term_hash_cstr(marker, left_cwd);
  marker = term_hash_cstr(marker, right_cwd);
  marker = term_hash_u32(marker, compositor_marker);

  *out_marker = marker;
  wm_terminal_manager_bind((wm_terminal_manager_t *)0);
  return 0;
}
