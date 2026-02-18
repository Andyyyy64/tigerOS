#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct framebuffer_info {
  volatile uint8_t *base;
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bytes_per_pixel;
} framebuffer_info_t;

bool framebuffer_init(void);
bool framebuffer_is_ready(void);

const framebuffer_info_t *framebuffer_get_info(void);

void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void framebuffer_fill(uint32_t color);
void framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                           uint32_t color);
void framebuffer_blit(uint32_t dst_x, uint32_t dst_y, const uint32_t *src,
                      uint32_t src_width, uint32_t src_height, uint32_t src_stride);

uint32_t framebuffer_compute_marker(void);
uint32_t framebuffer_render_test_pattern(void);

#endif
