#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

struct video_mode {
  uint32_t *base;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
};

int video_init(void);
const struct video_mode *video_get_mode(void);

#endif
