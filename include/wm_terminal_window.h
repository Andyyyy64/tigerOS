#ifndef WM_TERMINAL_WINDOW_H
#define WM_TERMINAL_WINDOW_H

#include <stdint.h>

#include "keyboard.h"
#include "terminal_session.h"
#include "wm_window.h"

typedef struct wm_terminal_window {
  wm_window_t window;
  terminal_session_t session;
  uint32_t owner_task_id;
} wm_terminal_window_t;

void wm_terminal_window_init(wm_terminal_window_t *terminal_window,
                             const char *title,
                             uint32_t x,
                             uint32_t y,
                             uint32_t width,
                             uint32_t height,
                             uint32_t endpoint_id,
                             uint32_t owner_task_id);
int wm_terminal_window_attach(wm_terminal_window_t *terminal_window);
int wm_terminal_window_handle_key(wm_terminal_window_t *terminal_window,
                                  const keyboard_event_t *event);
void wm_terminal_window_dispatch_reset(void);
void wm_terminal_window_dispatch_event(uint32_t endpoint_id, const keyboard_event_t *event);

uint32_t wm_terminal_window_endpoint(const wm_terminal_window_t *terminal_window);
uint32_t wm_terminal_window_owner_task(const wm_terminal_window_t *terminal_window);
const wm_window_t *wm_terminal_window_native(const wm_terminal_window_t *terminal_window);
uint32_t wm_terminal_window_input_len(const wm_terminal_window_t *terminal_window);
uint32_t wm_terminal_window_history_count(const wm_terminal_window_t *terminal_window);
uint32_t wm_terminal_window_lines_executed(const wm_terminal_window_t *terminal_window);
uint32_t wm_terminal_window_marker(const wm_terminal_window_t *terminal_window);
const char *wm_terminal_window_cwd(const wm_terminal_window_t *terminal_window);

#endif
