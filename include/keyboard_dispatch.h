#ifndef KEYBOARD_DISPATCH_H
#define KEYBOARD_DISPATCH_H

#include <stdint.h>

#include "keyboard.h"
#include "wm_window.h"

typedef void (*keyboard_dispatch_fn)(uint32_t endpoint_id, const keyboard_event_t *event);

void keyboard_dispatch_reset(void);
int keyboard_dispatch_register_window(const wm_window_t *window, uint32_t endpoint_id);
void keyboard_dispatch_set_sink(keyboard_dispatch_fn dispatch_fn);
uint32_t keyboard_dispatch_pending(void);

#endif
