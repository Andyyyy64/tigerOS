#include <stdint.h>

#include "wm_focus.h"

static const wm_window_t *g_active_window;

static int wm_point_in_rect(const wm_rect_t *rect, uint32_t x, uint32_t y) {
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

void wm_focus_reset(void) { g_active_window = (const wm_window_t *)0; }

int wm_focus_set_active_window(const wm_window_t *window) {
  g_active_window = window;
  return 0;
}

const wm_window_t *wm_focus_active_window(void) { return g_active_window; }

int wm_focus_is_active_window(const wm_window_t *window) {
  if (window == (const wm_window_t *)0) {
    return 0;
  }

  return g_active_window == window ? 1 : 0;
}

int wm_focus_clear_if_active(const wm_window_t *window) {
  if (window == (const wm_window_t *)0 || g_active_window != window) {
    return 0;
  }

  g_active_window = (const wm_window_t *)0;
  return 1;
}

int wm_focus_window_contains_point(const wm_window_t *window, uint32_t x, uint32_t y) {
  if (window == (const wm_window_t *)0) {
    return 0;
  }

  return wm_point_in_rect(&window->frame, x, y);
}

const wm_window_t *wm_focus_hit_test(const wm_layer_stack_t *stack, uint32_t x, uint32_t y,
                                     uint32_t *z_index_out) {
  uint32_t i;
  const wm_window_t *window;

  if (stack == (const wm_layer_stack_t *)0) {
    return (const wm_window_t *)0;
  }

  for (i = wm_layers_count(stack); i > 0u; --i) {
    uint32_t z_index = i - 1u;
    window = wm_layers_get_at(stack, z_index);
    if (wm_focus_window_contains_point(window, x, y) != 0) {
      if (z_index_out != (uint32_t *)0) {
        *z_index_out = z_index;
      }
      return window;
    }
  }

  return (const wm_window_t *)0;
}
