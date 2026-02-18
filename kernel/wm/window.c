#include <stdint.h>

#include "wm_window.h"

enum {
  WM_DEFAULT_BORDER_THICKNESS = 1u,
  WM_DEFAULT_TITLE_BAR_HEIGHT = 18u,
};

static uint32_t min_u32(uint32_t a, uint32_t b) { return (a < b) ? a : b; }

static uint32_t safe_sub_u32(uint32_t value, uint32_t subtractor) {
  if (value <= subtractor) {
    return 0u;
  }

  return value - subtractor;
}

void wm_window_init(wm_window_t *window, const char *title, uint32_t x, uint32_t y, uint32_t width,
                    uint32_t height) {
  if (window == (wm_window_t *)0) {
    return;
  }

  window->title = title;
  window->frame.x = x;
  window->frame.y = y;
  window->frame.width = width;
  window->frame.height = height;

  window->style.border_color = 0x00d8d8d8u;
  window->style.title_bar_color = 0x0039699fu;
  window->style.content_color = 0x00f2f4f8u;
  window->style.border_thickness = WM_DEFAULT_BORDER_THICKNESS;
  window->style.title_bar_height = WM_DEFAULT_TITLE_BAR_HEIGHT;
}

wm_rect_t wm_window_title_bar_rect(const wm_window_t *window) {
  wm_rect_t rect = {0u, 0u, 0u, 0u};
  uint32_t inner_width;
  uint32_t inner_height;
  uint32_t border;
  uint32_t border_total;

  if (window == (const wm_window_t *)0) {
    return rect;
  }

  border = window->style.border_thickness;
  border_total = border * 2u;

  inner_width = safe_sub_u32(window->frame.width, border_total);
  inner_height = safe_sub_u32(window->frame.height, border_total);

  rect.x = window->frame.x + border;
  rect.y = window->frame.y + border;
  rect.width = inner_width;
  rect.height = min_u32(window->style.title_bar_height, inner_height);
  return rect;
}

wm_rect_t wm_window_content_rect(const wm_window_t *window) {
  wm_rect_t rect = {0u, 0u, 0u, 0u};
  wm_rect_t title_bar;
  uint32_t inner_height;
  uint32_t border;
  uint32_t border_total;

  if (window == (const wm_window_t *)0) {
    return rect;
  }

  title_bar = wm_window_title_bar_rect(window);
  border = window->style.border_thickness;
  border_total = border * 2u;
  inner_height = safe_sub_u32(window->frame.height, border_total);

  rect.x = title_bar.x;
  rect.y = title_bar.y + title_bar.height;
  rect.width = title_bar.width;
  rect.height = safe_sub_u32(inner_height, title_bar.height);
  return rect;
}
