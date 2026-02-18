#include <stdint.h>

#include "apps/app_window.h"
#include "apps/demo_window_app.h"
#include "console.h"
#include "line_io.h"
#include "mouse.h"
#include "task.h"
#include "wm_compositor.h"

typedef struct demo_window_app_state {
  task_control_block_t *task;
  app_window_t window;
  uint32_t callback_count;
  uint32_t render_marker;
  int registered;
  int ran_once;
} demo_window_app_state_t;

static demo_window_app_state_t g_demo_window_app;

static void demo_window_app_on_event(struct app_window *window, const app_window_event_t *event,
                                     void *user_ctx) {
  demo_window_app_state_t *state = (demo_window_app_state_t *)user_ctx;
  (void)window;
  (void)event;

  if (state == (demo_window_app_state_t *)0) {
    return;
  }

  state->callback_count += 1u;
}

static void demo_window_app_task(task_control_block_t *task) {
  app_window_desc_t window_desc;
  uint32_t x;
  uint32_t y;

  if (task == (task_control_block_t *)0 || g_demo_window_app.ran_once != 0) {
    return;
  }

  wm_compositor_reset(0x0014161au);
  app_window_reset();
  mouse_reset();

  g_demo_window_app.callback_count = 0u;
  g_demo_window_app.render_marker = 0u;

  window_desc.title = "DemoApp";
  window_desc.x = 72u;
  window_desc.y = 56u;
  window_desc.width = 224u;
  window_desc.height = 140u;

  if (app_window_open(&g_demo_window_app.window, &window_desc, task, demo_window_app_on_event,
                      &g_demo_window_app) != 0) {
    line_io_write("APP: demo window open failed\n");
    g_demo_window_app.ran_once = 1;
    return;
  }

  x = window_desc.x + 12u;
  y = window_desc.y + 12u;
  (void)mouse_emit_move(x, y, 0u);
  (void)mouse_emit_button_down(x, y, INPUT_MOUSE_BUTTON_LEFT);
  (void)mouse_emit_button_up(x, y, INPUT_MOUSE_BUTTON_LEFT);
  (void)app_window_dispatch_pending();

  g_demo_window_app.render_marker = wm_compositor_render();
  if (g_demo_window_app.callback_count > 0u) {
    line_io_write("APP: demo window callback marker 0x");
    {
      static const char digits[] = "0123456789ABCDEF";
      int shift;

      for (shift = 28; shift >= 0; shift -= 4) {
        console_putc(digits[(g_demo_window_app.render_marker >> (uint32_t)shift) & 0x0fu]);
      }
    }
    line_io_write("\n");
  } else {
    line_io_write("APP: demo window callback missing\n");
  }

  g_demo_window_app.ran_once = 1;
}

int demo_window_app_register(void) {
  if (g_demo_window_app.registered != 0) {
    return 0;
  }

  g_demo_window_app.task = task_create("demo-window-app", demo_window_app_task);
  if (g_demo_window_app.task == (task_control_block_t *)0) {
    return -1;
  }

  g_demo_window_app.registered = 1;
  return 0;
}

void demo_window_app_run_once(void) {
  if (g_demo_window_app.registered == 0 || g_demo_window_app.task == (task_control_block_t *)0) {
    return;
  }

  if (g_demo_window_app.task->entry != (task_entry_fn)0) {
    g_demo_window_app.task->entry(g_demo_window_app.task);
  }
}
