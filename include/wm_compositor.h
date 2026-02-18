#ifndef WM_COMPOSITOR_H
#define WM_COMPOSITOR_H

#include <stdint.h>

#include "wm_window.h"

int wm_compositor_reset(uint32_t background_color);
int wm_compositor_add_window(const wm_window_t *window);
uint32_t wm_compositor_render(void);
uint32_t wm_compositor_window_count(void);

#endif
