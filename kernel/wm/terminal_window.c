#include "keyboard_dispatch.h"
#include "wm_compositor.h"
#include "wm_layers.h"
#include "wm_terminal_window.h"

typedef struct terminal_dispatch_binding {
  wm_terminal_window_t *terminal_window;
  uint32_t endpoint_id;
} terminal_dispatch_binding_t;

typedef struct terminal_dispatch_state {
  terminal_dispatch_binding_t bindings[WM_MAX_WINDOWS];
  uint32_t binding_count;
} terminal_dispatch_state_t;

static terminal_dispatch_state_t g_terminal_dispatch_state;

static terminal_dispatch_binding_t *find_binding_by_endpoint(uint32_t endpoint_id) {
  uint32_t i;

  if (endpoint_id == 0u) {
    return (terminal_dispatch_binding_t *)0;
  }

  for (i = 0u; i < g_terminal_dispatch_state.binding_count; ++i) {
    if (g_terminal_dispatch_state.bindings[i].endpoint_id == endpoint_id) {
      return &g_terminal_dispatch_state.bindings[i];
    }
  }

  return (terminal_dispatch_binding_t *)0;
}

static terminal_dispatch_binding_t *find_binding_by_window(const wm_terminal_window_t *terminal_window) {
  uint32_t i;

  if (terminal_window == (const wm_terminal_window_t *)0) {
    return (terminal_dispatch_binding_t *)0;
  }

  for (i = 0u; i < g_terminal_dispatch_state.binding_count; ++i) {
    if (g_terminal_dispatch_state.bindings[i].terminal_window == terminal_window) {
      return &g_terminal_dispatch_state.bindings[i];
    }
  }

  return (terminal_dispatch_binding_t *)0;
}

static void unregister_terminal_window(wm_terminal_window_t *terminal_window) {
  uint32_t i;

  if (terminal_window == (wm_terminal_window_t *)0) {
    return;
  }

  for (i = 0u; i < g_terminal_dispatch_state.binding_count; ++i) {
    if (g_terminal_dispatch_state.bindings[i].terminal_window == terminal_window) {
      uint32_t last = g_terminal_dispatch_state.binding_count - 1u;
      g_terminal_dispatch_state.bindings[i] = g_terminal_dispatch_state.bindings[last];
      g_terminal_dispatch_state.bindings[last].terminal_window = (wm_terminal_window_t *)0;
      g_terminal_dispatch_state.bindings[last].endpoint_id = 0u;
      g_terminal_dispatch_state.binding_count -= 1u;
      return;
    }
  }
}

static int register_terminal_window(wm_terminal_window_t *terminal_window) {
  terminal_dispatch_binding_t *binding;
  uint32_t endpoint_id;

  if (terminal_window == (wm_terminal_window_t *)0) {
    return -1;
  }

  endpoint_id = terminal_session_endpoint_id(&terminal_window->session);
  if (endpoint_id == 0u) {
    return -1;
  }

  binding = find_binding_by_window(terminal_window);
  if (binding != (terminal_dispatch_binding_t *)0) {
    terminal_dispatch_binding_t *existing = find_binding_by_endpoint(endpoint_id);
    if (existing != (terminal_dispatch_binding_t *)0 && existing != binding) {
      return -1;
    }

    binding->endpoint_id = endpoint_id;
    return 0;
  }

  binding = find_binding_by_endpoint(endpoint_id);
  if (binding != (terminal_dispatch_binding_t *)0) {
    return -1;
  }

  if (g_terminal_dispatch_state.binding_count >= WM_MAX_WINDOWS) {
    return -1;
  }

  g_terminal_dispatch_state.bindings[g_terminal_dispatch_state.binding_count].terminal_window =
      terminal_window;
  g_terminal_dispatch_state.bindings[g_terminal_dispatch_state.binding_count].endpoint_id = endpoint_id;
  g_terminal_dispatch_state.binding_count += 1u;
  return 0;
}

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
  int was_registered;

  if (terminal_window == (wm_terminal_window_t *)0) {
    return -1;
  }

  was_registered = find_binding_by_window(terminal_window) != (terminal_dispatch_binding_t *)0;
  if (register_terminal_window(terminal_window) != 0) {
    return -1;
  }

  if (wm_compositor_add_window(&terminal_window->window) != 0) {
    if (was_registered == 0) {
      unregister_terminal_window(terminal_window);
    }
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

void wm_terminal_window_dispatch_reset(void) {
  uint32_t i;

  g_terminal_dispatch_state.binding_count = 0u;
  for (i = 0u; i < WM_MAX_WINDOWS; ++i) {
    g_terminal_dispatch_state.bindings[i].terminal_window = (wm_terminal_window_t *)0;
    g_terminal_dispatch_state.bindings[i].endpoint_id = 0u;
  }
}

void wm_terminal_window_dispatch_event(uint32_t endpoint_id, const keyboard_event_t *event) {
  terminal_dispatch_binding_t *binding = find_binding_by_endpoint(endpoint_id);

  if (binding == (terminal_dispatch_binding_t *)0 || binding->terminal_window == (wm_terminal_window_t *)0) {
    return;
  }

  (void)wm_terminal_window_handle_key(binding->terminal_window, event);
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
