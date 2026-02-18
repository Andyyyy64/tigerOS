#include <stdint.h>

#include "wm_layers.h"

void wm_layers_reset(wm_layer_stack_t *stack) {
  uint32_t i;

  if (stack == (wm_layer_stack_t *)0) {
    return;
  }

  for (i = 0u; i < WM_MAX_WINDOWS; ++i) {
    stack->windows[i] = (const wm_window_t *)0;
  }
  stack->count = 0u;
}

int wm_layers_push_back(wm_layer_stack_t *stack, const wm_window_t *window) {
  if (stack == (wm_layer_stack_t *)0 || window == (const wm_window_t *)0) {
    return -1;
  }

  if (stack->count >= WM_MAX_WINDOWS) {
    return -1;
  }

  stack->windows[stack->count] = window;
  stack->count += 1u;
  return 0;
}

uint32_t wm_layers_count(const wm_layer_stack_t *stack) {
  if (stack == (const wm_layer_stack_t *)0) {
    return 0u;
  }

  return stack->count;
}

const wm_window_t *wm_layers_get_at(const wm_layer_stack_t *stack, uint32_t index) {
  if (stack == (const wm_layer_stack_t *)0 || index >= stack->count) {
    return (const wm_window_t *)0;
  }

  return stack->windows[index];
}

int wm_layers_index_of(const wm_layer_stack_t *stack, const wm_window_t *window) {
  uint32_t i;

  if (stack == (const wm_layer_stack_t *)0 || window == (const wm_window_t *)0) {
    return -1;
  }

  for (i = 0u; i < stack->count; ++i) {
    if (stack->windows[i] == window) {
      return (int)i;
    }
  }

  return -1;
}

int wm_layers_move_to_front(wm_layer_stack_t *stack, const wm_window_t *window) {
  int index;
  uint32_t i;
  const wm_window_t *moving_window;
  uint32_t front_index;

  if (stack == (wm_layer_stack_t *)0 || window == (const wm_window_t *)0 || stack->count == 0u) {
    return -1;
  }

  index = wm_layers_index_of(stack, window);
  if (index < 0) {
    return -1;
  }

  front_index = stack->count - 1u;
  if ((uint32_t)index == front_index) {
    return 0;
  }

  moving_window = stack->windows[(uint32_t)index];
  for (i = (uint32_t)index; i < front_index; ++i) {
    stack->windows[i] = stack->windows[i + 1u];
  }
  stack->windows[front_index] = moving_window;
  return 0;
}
