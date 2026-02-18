#ifndef APPS_APP_WINDOW_H
#define APPS_APP_WINDOW_H

#include <stdint.h>

#include "input_event.h"
#include "task.h"
#include "wm_window.h"

typedef enum app_window_event_type {
  APP_WINDOW_EVENT_MOVE = 0,
  APP_WINDOW_EVENT_POINTER_DOWN = 1,
  APP_WINDOW_EVENT_POINTER_UP = 2,
  APP_WINDOW_EVENT_DRAG = 3,
} app_window_event_type_t;

typedef struct app_window_event {
  app_window_event_type_t type;
  input_event_t input;
} app_window_event_t;

struct app_window;
typedef void (*app_window_event_fn)(struct app_window *window, const app_window_event_t *event,
                                    void *user_ctx);

typedef struct app_window_desc {
  const char *title;
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;
} app_window_desc_t;

typedef struct app_window {
  wm_window_t window;
  task_control_block_t *owner_task;
  app_window_event_fn on_event;
  void *user_ctx;
} app_window_t;

void app_window_reset(void);
int app_window_open(app_window_t *app_window, const app_window_desc_t *desc,
                    task_control_block_t *owner_task, app_window_event_fn on_event,
                    void *user_ctx);
uint32_t app_window_dispatch_pending(void);
const wm_window_t *app_window_native(const app_window_t *app_window);

#endif
