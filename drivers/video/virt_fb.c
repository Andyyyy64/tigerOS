#include <stddef.h>

#include "video_fb.h"

enum {
  VIRT_FB_WIDTH = 64u,
  VIRT_FB_HEIGHT = 64u,
  VIRT_FB_BPP = 4u,
  VIRT_FB_SIZE = VIRT_FB_WIDTH * VIRT_FB_HEIGHT * VIRT_FB_BPP,
};

static volatile uint8_t g_fb_storage[VIRT_FB_SIZE] __attribute__((aligned(64)));

bool video_fb_init(framebuffer_info_t *out_info) {
  size_t i;

  if (out_info == 0) {
    return false;
  }

  for (i = 0u; i < (size_t)VIRT_FB_SIZE; ++i) {
    g_fb_storage[i] = 0u;
  }

  out_info->base = g_fb_storage;
  out_info->width = VIRT_FB_WIDTH;
  out_info->height = VIRT_FB_HEIGHT;
  out_info->pitch = VIRT_FB_WIDTH * VIRT_FB_BPP;
  out_info->bytes_per_pixel = VIRT_FB_BPP;

  return true;
}
