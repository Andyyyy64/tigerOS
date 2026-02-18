#include <stddef.h>
#include <stdint.h>

#include "video.h"

enum {
  VIDEO_FB_WIDTH = 320u,
  VIDEO_FB_HEIGHT = 200u,
};

static uint32_t g_framebuffer[VIDEO_FB_WIDTH * VIDEO_FB_HEIGHT];

static struct video_mode g_mode = {
    .base = g_framebuffer,
    .width = VIDEO_FB_WIDTH,
    .height = VIDEO_FB_HEIGHT,
    .stride = VIDEO_FB_WIDTH,
};

int video_init(void) {
  size_t i;

  for (i = 0u; i < (size_t)(VIDEO_FB_WIDTH * VIDEO_FB_HEIGHT); ++i) {
    g_framebuffer[i] = 0u;
  }

  return 0;
}

const struct video_mode *video_get_mode(void) { return &g_mode; }
