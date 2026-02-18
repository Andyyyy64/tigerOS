#ifndef WM_LAYERS_H
#define WM_LAYERS_H

#include <stdint.h>

#include "wm_window.h"

enum {
  WM_MAX_WINDOWS = 8u,
};

typedef struct wm_layer_stack {
  const wm_window_t *windows[WM_MAX_WINDOWS];
  uint32_t count;
} wm_layer_stack_t;

void wm_layers_reset(wm_layer_stack_t *stack);
int wm_layers_push_back(wm_layer_stack_t *stack, const wm_window_t *window);
uint32_t wm_layers_count(const wm_layer_stack_t *stack);
const wm_window_t *wm_layers_get_at(const wm_layer_stack_t *stack, uint32_t index);
int wm_layers_index_of(const wm_layer_stack_t *stack, const wm_window_t *window);
int wm_layers_move_to_front(wm_layer_stack_t *stack, const wm_window_t *window);

#endif
