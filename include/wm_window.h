#ifndef WM_WINDOW_H
#define WM_WINDOW_H

#include <stdint.h>

typedef struct wm_rect {
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;
} wm_rect_t;

typedef struct wm_window_style {
  uint32_t border_color;
  uint32_t title_bar_color;
  uint32_t content_color;
  uint32_t border_thickness;
  uint32_t title_bar_height;
} wm_window_style_t;

typedef struct wm_window {
  const char *title;
  wm_rect_t frame;
  wm_window_style_t style;
} wm_window_t;

void wm_window_init(wm_window_t *window, const char *title, uint32_t x, uint32_t y, uint32_t width,
                    uint32_t height);
int wm_window_is_valid(const wm_window_t *window);
wm_rect_t wm_window_title_bar_rect(const wm_window_t *window);
wm_rect_t wm_window_content_rect(const wm_window_t *window);

#endif
