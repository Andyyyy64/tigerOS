#include <stdint.h>

#include "framebuffer.h"
#include "wm_compositor.h"

enum {
  WM_MAX_WINDOWS = 8u,
};

typedef struct wm_scene {
  uint32_t background_color;
  const wm_window_t *windows[WM_MAX_WINDOWS];
  uint32_t window_count;
} wm_scene_t;

static wm_scene_t g_scene;

static uint32_t fnv1a32_pixels(const volatile uint32_t *pixels, uint32_t pixel_count) {
  uint32_t i;
  uint32_t hash = 2166136261u;

  for (i = 0u; i < pixel_count; ++i) {
    uint32_t value = pixels[i];
    uint32_t byte_index;

    for (byte_index = 0u; byte_index < 4u; ++byte_index) {
      hash ^= (value >> (byte_index * 8u)) & 0xffu;
      hash *= 16777619u;
    }
  }

  return hash;
}

static uint32_t hash_title(const char *title) {
  uint32_t hash = 2166136261u;
  uint32_t i;

  if (title == (const char *)0) {
    return hash;
  }

  for (i = 0u; title[i] != '\0'; ++i) {
    hash ^= (uint32_t)(uint8_t)title[i];
    hash *= 16777619u;
  }

  return hash;
}

static void wm_draw_window(const wm_window_t *window) {
  wm_rect_t title_bar;
  wm_rect_t content;
  uint32_t title_hash;

  if (window == (const wm_window_t *)0 || window->frame.width == 0u || window->frame.height == 0u) {
    return;
  }

  framebuffer_fill_rect(window->frame.x, window->frame.y, window->frame.width, window->frame.height,
                        window->style.border_color);

  title_bar = wm_window_title_bar_rect(window);
  if (title_bar.width != 0u && title_bar.height != 0u) {
    framebuffer_fill_rect(title_bar.x, title_bar.y, title_bar.width, title_bar.height,
                          window->style.title_bar_color);
  }

  content = wm_window_content_rect(window);
  if (content.width != 0u && content.height != 0u) {
    framebuffer_fill_rect(content.x, content.y, content.width, content.height, window->style.content_color);
  }

  if (title_bar.width > 8u && title_bar.height > 4u) {
    uint32_t accent_width;
    uint32_t accent_color;

    title_hash = hash_title(window->title);
    accent_width = 8u + (title_hash % (title_bar.width - 8u));
    accent_color = 0x00202020u | (title_hash & 0x000f0f0fu);
    framebuffer_fill_rect(title_bar.x + 4u, title_bar.y + 4u, accent_width - 4u, 1u, accent_color);
  }
}

int wm_compositor_reset(uint32_t background_color) {
  g_scene.background_color = background_color;
  g_scene.window_count = 0u;
  return 0;
}

int wm_compositor_add_window(const wm_window_t *window) {
  if (window == (const wm_window_t *)0) {
    return -1;
  }

  if (g_scene.window_count >= WM_MAX_WINDOWS) {
    return -1;
  }

  g_scene.windows[g_scene.window_count] = window;
  g_scene.window_count += 1u;
  return 0;
}

uint32_t wm_compositor_window_count(void) { return g_scene.window_count; }

uint32_t wm_compositor_render(void) {
  const struct framebuffer_info *fb;
  uint32_t i;

  fb = framebuffer_get_info();
  if (fb == (const struct framebuffer_info *)0 || fb->pixels == (volatile uint32_t *)0 ||
      fb->width == 0u || fb->height == 0u || fb->stride < fb->width) {
    return 0u;
  }

  framebuffer_fill_rect(0u, 0u, fb->width, fb->height, g_scene.background_color);

  for (i = 0u; i < g_scene.window_count; ++i) {
    wm_draw_window(g_scene.windows[i]);
  }

  return fnv1a32_pixels(fb->pixels, fb->height * fb->stride);
}
