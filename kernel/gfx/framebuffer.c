#include <stdint.h>

#include "framebuffer.h"
#include "video.h"

static struct framebuffer_info g_framebuffer;

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

int framebuffer_init(void) {
  const struct video_mode *mode;

  if (video_init() != 0) {
    return -1;
  }

  mode = video_get_mode();
  if (mode == (const struct video_mode *)0 || mode->base == (uint32_t *)0 ||
      mode->width == 0u || mode->height == 0u || mode->stride < mode->width) {
    return -1;
  }

  g_framebuffer.pixels = (volatile uint32_t *)mode->base;
  g_framebuffer.width = mode->width;
  g_framebuffer.height = mode->height;
  g_framebuffer.stride = mode->stride;
  return 0;
}

const struct framebuffer_info *framebuffer_get_info(void) { return &g_framebuffer; }

void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
  if (g_framebuffer.pixels == (volatile uint32_t *)0) {
    return;
  }

  if (x >= g_framebuffer.width || y >= g_framebuffer.height) {
    return;
  }

  g_framebuffer.pixels[(y * g_framebuffer.stride) + x] = color;
}

void framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                           uint32_t color) {
  uint32_t max_x;
  uint32_t max_y;
  uint32_t row;

  if (g_framebuffer.pixels == (volatile uint32_t *)0 || width == 0u || height == 0u) {
    return;
  }

  if (x >= g_framebuffer.width || y >= g_framebuffer.height) {
    return;
  }

  max_x = x + width;
  if (max_x < x || max_x > g_framebuffer.width) {
    max_x = g_framebuffer.width;
  }

  max_y = y + height;
  if (max_y < y || max_y > g_framebuffer.height) {
    max_y = g_framebuffer.height;
  }

  for (row = y; row < max_y; ++row) {
    uint32_t col;
    uint32_t row_base = row * g_framebuffer.stride;

    for (col = x; col < max_x; ++col) {
      g_framebuffer.pixels[row_base + col] = color;
    }
  }
}

void framebuffer_blit(const uint32_t *src, uint32_t src_stride, uint32_t x, uint32_t y,
                      uint32_t width, uint32_t height) {
  uint32_t copy_width;
  uint32_t copy_height;
  uint32_t row;

  if (src == (const uint32_t *)0 || src_stride == 0u || width == 0u || height == 0u) {
    return;
  }

  if (src_stride < width || g_framebuffer.pixels == (volatile uint32_t *)0) {
    return;
  }

  if (x >= g_framebuffer.width || y >= g_framebuffer.height) {
    return;
  }

  copy_width = width;
  if (copy_width > g_framebuffer.width - x) {
    copy_width = g_framebuffer.width - x;
  }

  copy_height = height;
  if (copy_height > g_framebuffer.height - y) {
    copy_height = g_framebuffer.height - y;
  }

  for (row = 0u; row < copy_height; ++row) {
    uint32_t col;
    uint32_t dest_row_base = (y + row) * g_framebuffer.stride;
    uint32_t src_row_base = row * src_stride;

    for (col = 0u; col < copy_width; ++col) {
      g_framebuffer.pixels[dest_row_base + x + col] = src[src_row_base + col];
    }
  }
}

uint32_t framebuffer_render_test_pattern(void) {
  static const uint32_t center_stamp[8u * 8u] = {
      0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u,
      0x00ffffffu, 0x00000000u, 0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu,
      0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00ffffffu, 0x00000000u,
      0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u,
      0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu,
      0x00000000u, 0x00ffffffu, 0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u,
      0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u, 0x00000000u, 0x00ffffffu,
      0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu,
      0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu, 0x00000000u,
      0x00ffffffu, 0x00000000u, 0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu,
      0x00000000u, 0x00ffffffu, 0x00000000u, 0x00ffffffu,
  };
  uint32_t i;

  if (g_framebuffer.pixels == (volatile uint32_t *)0) {
    return 0u;
  }

  framebuffer_fill_rect(0u, 0u, g_framebuffer.width, g_framebuffer.height, 0x00101010u);
  framebuffer_fill_rect(0u, 0u, g_framebuffer.width / 2u, g_framebuffer.height / 2u,
                        0x00e53935u);
  framebuffer_fill_rect(g_framebuffer.width / 2u, 0u, g_framebuffer.width / 2u,
                        g_framebuffer.height / 2u, 0x0043a047u);
  framebuffer_fill_rect(0u, g_framebuffer.height / 2u, g_framebuffer.width / 2u,
                        g_framebuffer.height / 2u, 0x001e88e5u);
  framebuffer_fill_rect(g_framebuffer.width / 2u, g_framebuffer.height / 2u,
                        g_framebuffer.width / 2u, g_framebuffer.height / 2u, 0x00fdd835u);

  for (i = 0u; i < g_framebuffer.width; ++i) {
    framebuffer_put_pixel(i, 0u, 0x00ffffffu);
    framebuffer_put_pixel(i, g_framebuffer.height - 1u, 0x00ffffffu);
  }

  for (i = 0u; i < g_framebuffer.height; ++i) {
    framebuffer_put_pixel(0u, i, 0x00ffffffu);
    framebuffer_put_pixel(g_framebuffer.width - 1u, i, 0x00ffffffu);
  }

  for (i = 0u; i < g_framebuffer.height && i < g_framebuffer.width; ++i) {
    framebuffer_put_pixel(i, i, 0x00ffffffu);
  }

  framebuffer_blit(center_stamp, 8u, (g_framebuffer.width / 2u) - 4u,
                   (g_framebuffer.height / 2u) - 4u, 8u, 8u);

  return fnv1a32_pixels(g_framebuffer.pixels, g_framebuffer.height * g_framebuffer.stride);
}
