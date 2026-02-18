#include <stdint.h>

#include "event_queue.h"
#include "input_event.h"
#include "wm_compositor.h"
#include "wm_drag.h"
#include "wm_layers.h"
#include "wm_window.h"

typedef struct wm_drag_binding {
  wm_window_t *window;
  uint32_t task_id;
} wm_drag_binding_t;

typedef struct wm_drag_state {
  wm_drag_binding_t bindings[WM_MAX_WINDOWS];
  uint32_t binding_count;
  wm_drag_binding_t *drag_binding;
  uint32_t drag_offset_x;
  uint32_t drag_offset_y;
  wm_drag_dispatch_fn dispatch_fn;
} wm_drag_state_t;

static wm_drag_state_t g_drag_state;

static uint32_t clamp_sub_u32(uint32_t value, uint32_t subtractor) {
  if (value <= subtractor) {
    return 0u;
  }

  return value - subtractor;
}

static int point_in_rect(const wm_rect_t *rect, uint32_t x, uint32_t y) {
  if (rect == (const wm_rect_t *)0 || rect->width == 0u || rect->height == 0u) {
    return 0;
  }

  if (x < rect->x || y < rect->y) {
    return 0;
  }

  if ((x - rect->x) >= rect->width || (y - rect->y) >= rect->height) {
    return 0;
  }

  return 1;
}

static wm_drag_binding_t *find_binding_for_window(const wm_window_t *window) {
  uint32_t i;

  if (window == (const wm_window_t *)0) {
    return (wm_drag_binding_t *)0;
  }

  for (i = 0u; i < g_drag_state.binding_count; ++i) {
    if ((const wm_window_t *)g_drag_state.bindings[i].window == window) {
      return &g_drag_state.bindings[i];
    }
  }

  return (wm_drag_binding_t *)0;
}

static wm_drag_binding_t *find_binding_at(uint32_t x, uint32_t y) {
  const wm_window_t *window = wm_compositor_hit_test(x, y);
  return find_binding_for_window(window);
}

static void dispatch_to_task(const wm_drag_binding_t *binding, wm_dispatch_event_type_t dispatch_type,
                             const input_event_t *event) {
  if (binding == (const wm_drag_binding_t *)0 || g_drag_state.dispatch_fn == (wm_drag_dispatch_fn)0) {
    return;
  }

  g_drag_state.dispatch_fn(binding->task_id, dispatch_type, event);
}

static void process_mouse_move(const input_event_t *event) {
  wm_drag_binding_t *binding = g_drag_state.drag_binding;

  if (binding != (wm_drag_binding_t *)0 && (event->mouse.buttons & INPUT_MOUSE_BUTTON_LEFT) != 0u) {
    binding->window->frame.x = clamp_sub_u32(event->mouse.x, g_drag_state.drag_offset_x);
    binding->window->frame.y = clamp_sub_u32(event->mouse.y, g_drag_state.drag_offset_y);
    dispatch_to_task(binding, WM_DISPATCH_DRAG, event);
    return;
  }

  binding = find_binding_at(event->mouse.x, event->mouse.y);
  dispatch_to_task(binding, WM_DISPATCH_MOVE, event);
}

static void process_mouse_button_down(const input_event_t *event) {
  wm_drag_binding_t *binding = find_binding_at(event->mouse.x, event->mouse.y);

  if (binding == (wm_drag_binding_t *)0) {
    return;
  }

  (void)wm_compositor_activate_window((const wm_window_t *)binding->window);
  dispatch_to_task(binding, WM_DISPATCH_CLICK_DOWN, event);

  if ((event->mouse.button & INPUT_MOUSE_BUTTON_LEFT) != 0u) {
    wm_rect_t title_bar = wm_window_title_bar_rect((const wm_window_t *)binding->window);
    if (point_in_rect(&title_bar, event->mouse.x, event->mouse.y) != 0) {
      g_drag_state.drag_binding = binding;
      g_drag_state.drag_offset_x = event->mouse.x - binding->window->frame.x;
      g_drag_state.drag_offset_y = event->mouse.y - binding->window->frame.y;
    }
  }
}

static void process_mouse_button_up(const input_event_t *event) {
  wm_drag_binding_t *binding = g_drag_state.drag_binding;

  if (binding == (wm_drag_binding_t *)0) {
    binding = find_binding_at(event->mouse.x, event->mouse.y);
  }

  dispatch_to_task(binding, WM_DISPATCH_CLICK_UP, event);

  if ((event->mouse.button & INPUT_MOUSE_BUTTON_LEFT) != 0u) {
    g_drag_state.drag_binding = (wm_drag_binding_t *)0;
  }
}

static void wm_drag_process_event(const input_event_t *event) {
  if (event == (const input_event_t *)0) {
    return;
  }

  switch (event->type) {
  case INPUT_EVENT_MOUSE_MOVE:
    process_mouse_move(event);
    break;
  case INPUT_EVENT_MOUSE_BUTTON_DOWN:
    process_mouse_button_down(event);
    break;
  case INPUT_EVENT_MOUSE_BUTTON_UP:
    process_mouse_button_up(event);
    break;
  default:
    break;
  }
}

void wm_drag_reset(void) {
  uint32_t i;

  g_drag_state.binding_count = 0u;
  g_drag_state.drag_binding = (wm_drag_binding_t *)0;
  g_drag_state.drag_offset_x = 0u;
  g_drag_state.drag_offset_y = 0u;
  g_drag_state.dispatch_fn = (wm_drag_dispatch_fn)0;

  for (i = 0u; i < WM_MAX_WINDOWS; ++i) {
    g_drag_state.bindings[i].window = (wm_window_t *)0;
    g_drag_state.bindings[i].task_id = 0u;
  }
}

int wm_drag_register_window(wm_window_t *window, uint32_t task_id) {
  if (window == (wm_window_t *)0 || g_drag_state.binding_count >= WM_MAX_WINDOWS) {
    return -1;
  }

  g_drag_state.bindings[g_drag_state.binding_count].window = window;
  g_drag_state.bindings[g_drag_state.binding_count].task_id = task_id;
  g_drag_state.binding_count += 1u;
  return 0;
}

void wm_drag_set_dispatch(wm_drag_dispatch_fn dispatch_fn) { g_drag_state.dispatch_fn = dispatch_fn; }

uint32_t wm_drag_dispatch_pending(void) {
  input_event_t event;
  uint32_t processed_count = 0u;

  while (input_event_queue_pop(&event) == 0) {
    wm_drag_process_event(&event);
    processed_count += 1u;
  }

  return processed_count;
}
