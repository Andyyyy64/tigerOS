#include <stdint.h>

#include "wm_window.h"

enum {
  WM_DEFAULT_BORDER_THICKNESS = 1u,
  WM_DEFAULT_TITLE_BAR_HEIGHT = 18u,
};

static uint32_t min_u32(uint32_t a, uint32_t b) { return (a < b) ? a : b; }

static uint32_t saturating_add_u32(uint32_t a, uint32_t b) {
  uint32_t sum = a + b;
  if (sum < a) {
    return 0xffffffffu;
  }
  return sum;
}

static uint32_t safe_sub_u32(uint32_t value, uint32_t subtractor) {
  if (value <= subtractor) {
    return 0u;
  }

  return value - subtractor;
}

static uint32_t effective_border(const wm_window_t *window) {
  uint32_t max_border_x;
  uint32_t max_border_y;
  uint32_t max_border;

  max_border_x = window->frame.width / 2u;
  max_border_y = window->frame.height / 2u;
  max_border = min_u32(max_border_x, max_border_y);
  return min_u32(window->style.border_thickness, max_border);
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
  uint32_t frame_width;
  uint32_t frame_height;

  if (window == (const wm_window_t *)0) {
    return rect;
  }

  frame_width = window->frame.width;
  frame_height = window->frame.height;
  if (frame_width == 0u || frame_height == 0u) {
    return rect;
  }

  border = effective_border(window);
  inner_width = safe_sub_u32(frame_width, border);
  inner_width = safe_sub_u32(inner_width, border);
  inner_height = safe_sub_u32(frame_height, border);
  inner_height = safe_sub_u32(inner_height, border);

  rect.x = saturating_add_u32(window->frame.x, border);
  rect.y = saturating_add_u32(window->frame.y, border);
  rect.width = inner_width;
  rect.height = min_u32(window->style.title_bar_height, inner_height);
  return rect;
}

wm_rect_t wm_window_content_rect(const wm_window_t *window) {
  wm_rect_t rect = {0u, 0u, 0u, 0u};
  wm_rect_t title_bar;
  uint32_t inner_height;
  uint32_t border;
  uint32_t frame_height;

  if (window == (const wm_window_t *)0) {
    return rect;
  }

  title_bar = wm_window_title_bar_rect(window);
  frame_height = window->frame.height;
  if (frame_height == 0u) {
    return rect;
  }

  border = effective_border(window);
  inner_height = safe_sub_u32(frame_height, border);
  inner_height = safe_sub_u32(inner_height, border);

  rect.x = title_bar.x;
  rect.y = saturating_add_u32(title_bar.y, title_bar.height);
  rect.width = title_bar.width;
  rect.height = safe_sub_u32(inner_height, title_bar.height);
  return rect;
}
