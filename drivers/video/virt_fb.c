#include <stddef.h>
#include <stdint.h>

#include "video_fb.h"

enum {
  VIRT_FB_WIDTH = 64u,
  VIRT_FB_HEIGHT = 64u,
  VIRT_FB_SIZE = VIRT_FB_WIDTH * VIRT_FB_HEIGHT,
};

static volatile uint32_t g_fb_storage[VIRT_FB_SIZE] __attribute__((aligned(64)));

bool video_fb_init(struct framebuffer_info *out_info) {
  size_t i;

  if (out_info == 0) {
    return false;
  }

  for (i = 0u; i < (size_t)VIRT_FB_SIZE; ++i) {
    g_fb_storage[i] = 0u;
  }

  out_info->pixels = g_fb_storage;
  out_info->width = VIRT_FB_WIDTH;
  out_info->height = VIRT_FB_HEIGHT;
  out_info->stride = VIRT_FB_WIDTH;

  return true;
}
