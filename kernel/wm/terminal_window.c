#include "keyboard_dispatch.h"
#include "wm_compositor.h"
#include "wm_terminal_window.h"

void wm_terminal_window_init(wm_terminal_window_t *terminal_window,
                             const char *title,
                             uint32_t x,
                             uint32_t y,
                             uint32_t width,
                             uint32_t height,
                             uint32_t endpoint_id,
                             uint32_t owner_task_id) {
  if (terminal_window == (wm_terminal_window_t *)0) {
    return;
  }

  wm_window_init(&terminal_window->window, title, x, y, width, height);
  terminal_window->window.style.title_bar_color = 0x003a4b69u;
  terminal_window->window.style.content_color = 0x00f4f7fcu;
  terminal_window->owner_task_id = owner_task_id;
  terminal_session_init(&terminal_window->session, endpoint_id, &terminal_window->window);
}

int wm_terminal_window_attach(wm_terminal_window_t *terminal_window) {
  if (terminal_window == (wm_terminal_window_t *)0) {
    return -1;
  }

  if (wm_compositor_add_window(&terminal_window->window) != 0) {
    return -1;
  }

  return keyboard_dispatch_register_window(&terminal_window->window,
                                           terminal_session_endpoint_id(&terminal_window->session));
}

int wm_terminal_window_handle_key(wm_terminal_window_t *terminal_window,
                                  const keyboard_event_t *event) {
  if (terminal_window == (wm_terminal_window_t *)0) {
    return -1;
  }

  return terminal_session_handle_event(&terminal_window->session, event);
}

uint32_t wm_terminal_window_endpoint(const wm_terminal_window_t *terminal_window) {
  if (terminal_window == (const wm_terminal_window_t *)0) {
    return 0u;
  }
  return terminal_session_endpoint_id(&terminal_window->session);
}

uint32_t wm_terminal_window_owner_task(const wm_terminal_window_t *terminal_window) {
  if (terminal_window == (const wm_terminal_window_t *)0) {
    return 0u;
  }
  return terminal_window->owner_task_id;
}

const wm_window_t *wm_terminal_window_native(const wm_terminal_window_t *terminal_window) {
  if (terminal_window == (const wm_terminal_window_t *)0) {
    return (const wm_window_t *)0;
  }
  return &terminal_window->window;
}

uint32_t wm_terminal_window_input_len(const wm_terminal_window_t *terminal_window) {
  if (terminal_window == (const wm_terminal_window_t *)0) {
    return 0u;
  }
  return terminal_session_input_len(&terminal_window->session);
}

uint32_t wm_terminal_window_history_count(const wm_terminal_window_t *terminal_window) {
  if (terminal_window == (const wm_terminal_window_t *)0) {
    return 0u;
  }
  return terminal_session_history_count(&terminal_window->session);
}

uint32_t wm_terminal_window_lines_executed(const wm_terminal_window_t *terminal_window) {
  if (terminal_window == (const wm_terminal_window_t *)0) {
    return 0u;
  }
  return terminal_session_lines_executed(&terminal_window->session);
}

uint32_t wm_terminal_window_marker(const wm_terminal_window_t *terminal_window) {
  if (terminal_window == (const wm_terminal_window_t *)0) {
    return 0u;
  }
  return terminal_session_marker(&terminal_window->session);
}

const char *wm_terminal_window_cwd(const wm_terminal_window_t *terminal_window) {
  if (terminal_window == (const wm_terminal_window_t *)0) {
    return "";
  }
  return terminal_session_cwd(&terminal_window->session);
}
