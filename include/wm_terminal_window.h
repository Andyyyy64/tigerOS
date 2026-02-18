#ifndef WM_TERMINAL_WINDOW_H
#define WM_TERMINAL_WINDOW_H

#include <stdint.h>

#include "keyboard.h"
#include "tty_session.h"
#include "wm_window.h"

enum {
  WM_TERMINAL_MAX_WINDOWS = 4u,
};

typedef struct wm_terminal_window {
  wm_window_t window;
  uint32_t endpoint_id;
  tty_session_t session;
} wm_terminal_window_t;

typedef struct wm_terminal_manager {
  wm_terminal_window_t terminals[WM_TERMINAL_MAX_WINDOWS];
  uint32_t terminal_count;
} wm_terminal_manager_t;

void wm_terminal_manager_reset(wm_terminal_manager_t *manager);
int wm_terminal_manager_add(wm_terminal_manager_t *manager,
                            const char *title,
                            uint32_t endpoint_id,
                            uint32_t x,
                            uint32_t y,
                            uint32_t width,
                            uint32_t height,
                            wm_terminal_window_t **out_terminal);
void wm_terminal_manager_bind(wm_terminal_manager_t *manager);
void wm_terminal_keyboard_sink(uint32_t endpoint_id, const keyboard_event_t *event);
int wm_terminal_multi_session_test(uint32_t *out_marker);

#endif
