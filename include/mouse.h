#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_reset(void);
int mouse_emit_move(uint32_t x, uint32_t y, uint8_t buttons);
int mouse_emit_button_down(uint32_t x, uint32_t y, uint8_t button_mask);
int mouse_emit_button_up(uint32_t x, uint32_t y, uint8_t button_mask);

#endif
