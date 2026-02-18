#include <stdint.h>

#include "apps/app_window.h"
#include "wm_compositor.h"
#include "wm_drag.h"
#include "wm_layers.h"

typedef struct app_window_binding {
  uint32_t task_id;
  app_window_t *app_window;
} app_window_binding_t;

static app_window_binding_t g_bindings[WM_MAX_WINDOWS];
static uint32_t g_binding_count;

static app_window_binding_t *find_binding(uint32_t task_id) {
  uint32_t i;

  if (task_id == 0u) {
    return (app_window_binding_t *)0;
  }

  for (i = 0u; i < g_binding_count; ++i) {
    if (g_bindings[i].task_id == task_id) {
      return &g_bindings[i];
    }
  }

  return (app_window_binding_t *)0;
}

static app_window_event_type_t app_window_event_map(wm_dispatch_event_type_t dispatch_type) {
  switch (dispatch_type) {
  case WM_DISPATCH_CLICK_DOWN:
    return APP_WINDOW_EVENT_POINTER_DOWN;
  case WM_DISPATCH_CLICK_UP:
    return APP_WINDOW_EVENT_POINTER_UP;
  case WM_DISPATCH_DRAG:
    return APP_WINDOW_EVENT_DRAG;
  case WM_DISPATCH_MOVE:
  default:
    return APP_WINDOW_EVENT_MOVE;
  }
}

static void app_window_dispatch_cb(uint32_t task_id, wm_dispatch_event_type_t dispatch_type,
                                   const input_event_t *event) {
  app_window_binding_t *binding;
  app_window_event_t app_event;

  binding = find_binding(task_id);
  if (binding == (app_window_binding_t *)0 || binding->app_window == (app_window_t *)0 ||
      binding->app_window->on_event == (app_window_event_fn)0 ||
      event == (const input_event_t *)0) {
    return;
  }

  app_event.type = app_window_event_map(dispatch_type);
  app_event.input = *event;
  binding->app_window->on_event(binding->app_window, &app_event, binding->app_window->user_ctx);
}

void app_window_reset(void) {
  uint32_t i;

  g_binding_count = 0u;
  for (i = 0u; i < WM_MAX_WINDOWS; ++i) {
    g_bindings[i].task_id = 0u;
    g_bindings[i].app_window = (app_window_t *)0;
  }

  wm_drag_reset();
  wm_drag_set_dispatch(app_window_dispatch_cb);
}

int app_window_open(app_window_t *app_window, const app_window_desc_t *desc,
                    task_control_block_t *owner_task, app_window_event_fn on_event,
                    void *user_ctx) {
  app_window_binding_t *binding;

  if (app_window == (app_window_t *)0 || desc == (const app_window_desc_t *)0 ||
      owner_task == (task_control_block_t *)0 || owner_task->id == 0u ||
      on_event == (app_window_event_fn)0) {
    return -1;
  }

  wm_window_init(&app_window->window, desc->title, desc->x, desc->y, desc->width, desc->height);
  app_window->owner_task = owner_task;
  app_window->on_event = on_event;
  app_window->user_ctx = user_ctx;

  if (wm_compositor_add_window(&app_window->window) != 0) {
    return -1;
  }

  if (wm_drag_register_window(&app_window->window, owner_task->id) != 0) {
    return -1;
  }

  binding = find_binding(owner_task->id);
  if (binding != (app_window_binding_t *)0) {
    binding->app_window = app_window;
    return 0;
  }

  if (g_binding_count >= WM_MAX_WINDOWS) {
    return -1;
  }

  g_bindings[g_binding_count].task_id = owner_task->id;
  g_bindings[g_binding_count].app_window = app_window;
  g_binding_count += 1u;
  return 0;
}

uint32_t app_window_dispatch_pending(void) { return wm_drag_dispatch_pending(); }

const wm_window_t *app_window_native(const app_window_t *app_window) {
  if (app_window == (const app_window_t *)0) {
    return (const wm_window_t *)0;
  }

  return &app_window->window;
}
