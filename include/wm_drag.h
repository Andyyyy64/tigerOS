#ifndef WM_DRAG_H
#define WM_DRAG_H

#include <stdint.h>

#include "input_event.h"
#include "wm_window.h"

typedef enum wm_dispatch_event_type {
  WM_DISPATCH_MOVE = 0,
  WM_DISPATCH_CLICK_DOWN = 1,
  WM_DISPATCH_CLICK_UP = 2,
  WM_DISPATCH_DRAG = 3,
} wm_dispatch_event_type_t;

typedef void (*wm_drag_dispatch_fn)(uint32_t task_id, wm_dispatch_event_type_t dispatch_type,
                                    const input_event_t *event);

void wm_drag_reset(void);
int wm_drag_register_window(wm_window_t *window, uint32_t task_id);
void wm_drag_set_dispatch(wm_drag_dispatch_fn dispatch_fn);
uint32_t wm_drag_dispatch_pending(void);

#endif
