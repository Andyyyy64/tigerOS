#ifndef KEYBOARD_DISPATCH_H
#define KEYBOARD_DISPATCH_H

#include "keyboard.h"
#include "wm_window.h"

typedef void (*keyboard_text_handler_t)(const wm_window_t *window, char ch, void *ctx);
typedef void (*keyboard_control_handler_t)(const wm_window_t *window, keyboard_event_type_t control,
                                           void *ctx);

void keyboard_dispatch_reset(void);
int keyboard_dispatch_register_endpoint(const wm_window_t *window, keyboard_text_handler_t text_handler,
                                        keyboard_control_handler_t control_handler, void *ctx);
int keyboard_dispatch_unregister_endpoint(const wm_window_t *window);
int keyboard_dispatch_route_event(const keyboard_event_t *event);
int keyboard_dispatch_poll_and_route(void);

#endif
