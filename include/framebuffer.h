#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

struct framebuffer_info {
  volatile uint32_t *pixels;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
};

int framebuffer_init(void);
const struct framebuffer_info *framebuffer_get_info(void);
void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                           uint32_t color);
void framebuffer_blit(const uint32_t *src, uint32_t src_stride, uint32_t x, uint32_t y,
                      uint32_t width, uint32_t height);
uint32_t framebuffer_render_test_pattern(void);

#endif
