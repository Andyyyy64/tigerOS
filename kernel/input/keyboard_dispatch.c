#include <stdint.h>

#include "keyboard.h"
#include "keyboard_dispatch.h"
#include "wm_compositor.h"
#include "wm_layers.h"

typedef struct keyboard_binding {
  const wm_window_t *window;
  uint32_t endpoint_id;
} keyboard_binding_t;

typedef struct keyboard_dispatch_state {
  keyboard_binding_t bindings[WM_MAX_WINDOWS];
  uint32_t binding_count;
  keyboard_dispatch_fn sink;
} keyboard_dispatch_state_t;

static keyboard_dispatch_state_t g_keyboard_dispatch_state;

static keyboard_binding_t *find_binding(const wm_window_t *window) {
  uint32_t i;

  if (window == (const wm_window_t *)0) {
    return (keyboard_binding_t *)0;
  }

  for (i = 0u; i < g_keyboard_dispatch_state.binding_count; ++i) {
    if (g_keyboard_dispatch_state.bindings[i].window == window) {
      return &g_keyboard_dispatch_state.bindings[i];
    }
  }

  return (keyboard_binding_t *)0;
}

static void dispatch_to_focused_endpoint(const keyboard_event_t *event) {
  const wm_window_t *focused_window;
  keyboard_binding_t *binding;

  if (event == (const keyboard_event_t *)0 || g_keyboard_dispatch_state.sink == (keyboard_dispatch_fn)0) {
    return;
  }

  focused_window = wm_compositor_active_window();
  binding = find_binding(focused_window);
  if (binding == (keyboard_binding_t *)0) {
    return;
  }

  g_keyboard_dispatch_state.sink(binding->endpoint_id, event);
}

void keyboard_dispatch_reset(void) {
  uint32_t i;

  g_keyboard_dispatch_state.binding_count = 0u;
  g_keyboard_dispatch_state.sink = (keyboard_dispatch_fn)0;

  for (i = 0u; i < WM_MAX_WINDOWS; ++i) {
    g_keyboard_dispatch_state.bindings[i].window = (const wm_window_t *)0;
    g_keyboard_dispatch_state.bindings[i].endpoint_id = 0u;
  }
}

int keyboard_dispatch_register_window(const wm_window_t *window, uint32_t endpoint_id) {
  keyboard_binding_t *binding;

  if (window == (const wm_window_t *)0 || endpoint_id == 0u) {
    return -1;
  }

  binding = find_binding(window);
  if (binding != (keyboard_binding_t *)0) {
    binding->endpoint_id = endpoint_id;
    return 0;
  }

  if (g_keyboard_dispatch_state.binding_count >= WM_MAX_WINDOWS) {
    return -1;
  }

  g_keyboard_dispatch_state.bindings[g_keyboard_dispatch_state.binding_count].window = window;
  g_keyboard_dispatch_state.bindings[g_keyboard_dispatch_state.binding_count].endpoint_id = endpoint_id;
  g_keyboard_dispatch_state.binding_count += 1u;
  return 0;
}

void keyboard_dispatch_set_sink(keyboard_dispatch_fn dispatch_fn) {
  g_keyboard_dispatch_state.sink = dispatch_fn;
}

uint32_t keyboard_dispatch_pending(void) {
  keyboard_event_t event;
  uint32_t processed_count = 0u;

  while (keyboard_pop_event(&event) == 0) {
    dispatch_to_focused_endpoint(&event);
    processed_count += 1u;
  }

  return processed_count;
}
