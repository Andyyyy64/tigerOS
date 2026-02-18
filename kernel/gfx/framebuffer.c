#include <stddef.h>
#include <stdint.h>

#include "framebuffer.h"
#include "video_fb.h"

static framebuffer_info_t g_fb;
static bool g_fb_ready;

static volatile uint32_t *framebuffer_pixel_ptr(uint32_t x, uint32_t y) {
  uintptr_t row = (uintptr_t)y * (uintptr_t)g_fb.pitch;
  uintptr_t col = (uintptr_t)x * (uintptr_t)g_fb.bytes_per_pixel;
  return (volatile uint32_t *)(void *)(g_fb.base + row + col);
}

static uint32_t min_u32(uint32_t a, uint32_t b) {
  if (a < b) {
    return a;
  }
  return b;
}

bool framebuffer_init(void) {
  g_fb_ready = video_fb_init(&g_fb);
  return g_fb_ready;
}

bool framebuffer_is_ready(void) {
  return g_fb_ready;
}

const framebuffer_info_t *framebuffer_get_info(void) {
  if (!g_fb_ready) {
    return 0;
  }

  return &g_fb;
}

void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
  volatile uint32_t *pixel;

  if (!g_fb_ready || x >= g_fb.width || y >= g_fb.height) {
    return;
  }

  pixel = framebuffer_pixel_ptr(x, y);
  *pixel = color;
}

void framebuffer_fill(uint32_t color) {
  uint32_t x;
  uint32_t y;

  if (!g_fb_ready) {
    return;
  }

  for (y = 0u; y < g_fb.height; ++y) {
    for (x = 0u; x < g_fb.width; ++x) {
      framebuffer_put_pixel(x, y, color);
    }
  }
}

void framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                           uint32_t color) {
  uint32_t max_x;
  uint32_t max_y;
  uint32_t ix;
  uint32_t iy;

  if (!g_fb_ready || width == 0u || height == 0u || x >= g_fb.width || y >= g_fb.height) {
    return;
  }

  width = min_u32(width, g_fb.width - x);
  height = min_u32(height, g_fb.height - y);
  max_x = x + width;
  max_y = y + height;

  for (iy = y; iy < max_y; ++iy) {
    for (ix = x; ix < max_x; ++ix) {
      framebuffer_put_pixel(ix, iy, color);
    }
  }
}

void framebuffer_blit(uint32_t dst_x, uint32_t dst_y, const uint32_t *src,
                      uint32_t src_width, uint32_t src_height, uint32_t src_stride) {
  uint32_t x;
  uint32_t y;

  if (!g_fb_ready || src == 0 || src_width == 0u || src_height == 0u ||
      dst_x >= g_fb.width || dst_y >= g_fb.height) {
    return;
  }

  if (src_stride < src_width) {
    src_stride = src_width;
  }

  src_width = min_u32(src_width, g_fb.width - dst_x);
  src_height = min_u32(src_height, g_fb.height - dst_y);

  for (y = 0u; y < src_height; ++y) {
    for (x = 0u; x < src_width; ++x) {
      framebuffer_put_pixel(dst_x + x, dst_y + y, src[(size_t)y * src_stride + x]);
    }
  }
}

uint32_t framebuffer_compute_marker(void) {
  uint32_t hash = 2166136261u;
  size_t i;
  size_t total_bytes;

  if (!g_fb_ready) {
    return 0u;
  }

  total_bytes = (size_t)g_fb.height * (size_t)g_fb.pitch;
  for (i = 0u; i < total_bytes; ++i) {
    hash ^= g_fb.base[i];
    hash *= 16777619u;
  }

  return hash;
}

uint32_t framebuffer_render_test_pattern(void) {
  static const uint32_t sprite[] = {
      0x00ffffffu, 0x00000000u, 0x00000000u, 0x00ffffffu,
      0x00000000u, 0x00ffffffu, 0x00ffffffu, 0x00000000u,
      0x00000000u, 0x00ffffffu, 0x00ffffffu, 0x00000000u,
      0x00ffffffu, 0x00000000u, 0x00000000u, 0x00ffffffu,
  };
  uint32_t diagonal_pixels;
  uint32_t i;

  if (!g_fb_ready) {
    return 0u;
  }

  framebuffer_fill(0x00111220u);

  framebuffer_fill_rect(0u, 0u, g_fb.width / 2u, g_fb.height / 2u, 0x00cc3333u);
  framebuffer_fill_rect(g_fb.width / 2u, 0u, g_fb.width / 2u, g_fb.height / 2u,
                        0x0033cc33u);
  framebuffer_fill_rect(0u, g_fb.height / 2u, g_fb.width / 2u, g_fb.height / 2u,
                        0x003333ccu);
  framebuffer_fill_rect(g_fb.width / 2u, g_fb.height / 2u, g_fb.width / 2u,
                        g_fb.height / 2u, 0x00cccc33u);

  diagonal_pixels = min_u32(g_fb.width, g_fb.height);
  for (i = 0u; i < diagonal_pixels; ++i) {
    framebuffer_put_pixel(i, i, 0x00ffffffu);
  }

  framebuffer_blit((g_fb.width / 2u) - 2u, (g_fb.height / 2u) - 2u, sprite, 4u, 4u, 4u);

  framebuffer_fill_rect(0u, 0u, g_fb.width, 1u, 0x00000000u);
  framebuffer_fill_rect(0u, g_fb.height - 1u, g_fb.width, 1u, 0x00000000u);
  framebuffer_fill_rect(0u, 0u, 1u, g_fb.height, 0x00000000u);
  framebuffer_fill_rect(g_fb.width - 1u, 0u, 1u, g_fb.height, 0x00000000u);

  return framebuffer_compute_marker();
}
