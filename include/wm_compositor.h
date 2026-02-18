#ifndef WM_COMPOSITOR_H
#define WM_COMPOSITOR_H

#include <stdint.h>

#include "wm_window.h"

int wm_compositor_reset(uint32_t background_color);
int wm_compositor_add_window(const wm_window_t *window);
uint32_t wm_compositor_render(void);
uint32_t wm_compositor_window_count(void);
const wm_window_t *wm_compositor_window_at(uint32_t z_index);
const wm_window_t *wm_compositor_hit_test(uint32_t x, uint32_t y);
int wm_compositor_activate_window(const wm_window_t *window);
int wm_compositor_activate_at(uint32_t x, uint32_t y);
const wm_window_t *wm_compositor_active_window(void);

#endif
