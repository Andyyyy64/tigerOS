#ifndef WM_FOCUS_H
#define WM_FOCUS_H

#include <stdint.h>

#include "wm_layers.h"

void wm_focus_reset(void);
int wm_focus_set_active_window(const wm_window_t *window);
const wm_window_t *wm_focus_active_window(void);
int wm_focus_window_contains_point(const wm_window_t *window, uint32_t x, uint32_t y);
const wm_window_t *wm_focus_hit_test(const wm_layer_stack_t *stack, uint32_t x, uint32_t y,
                                     uint32_t *z_index_out);

#endif
