#include <stdint.h>

#include "apps/multi_terminal_test.h"
#include "apps/demo_window_app.h"
#include "clock.h"
#include "console.h"
#include "framebuffer.h"
#include "keyboard.h"
#include "keyboard_dispatch.h"
#include "line_io.h"
#include "mm_init.h"
#include "mouse.h"
#include "sched.h"
#include "shell.h"
#include "trap.h"
#include "wm_compositor.h"
#include "wm_drag.h"
#include "wm_window.h"

static void console_put_hex32(uint32_t value) {
  static const char digits[] = "0123456789ABCDEF";
  int shift;

  for (shift = 28; shift >= 0; shift -= 4) {
    console_putc(digits[(value >> (uint32_t)shift) & 0x0fu]);
  }
}

typedef struct mouse_dispatch_stats {
  uint32_t move_count;
  uint32_t button_down_count;
  uint32_t button_up_count;
  uint32_t drag_count;
  uint32_t last_button_down_task;
  uint32_t last_drag_task;
} mouse_dispatch_stats_t;

static mouse_dispatch_stats_t g_mouse_dispatch_stats;

static void mouse_dispatch_reset(void) {
  g_mouse_dispatch_stats.move_count = 0u;
  g_mouse_dispatch_stats.button_down_count = 0u;
  g_mouse_dispatch_stats.button_up_count = 0u;
  g_mouse_dispatch_stats.drag_count = 0u;
  g_mouse_dispatch_stats.last_button_down_task = 0u;
  g_mouse_dispatch_stats.last_drag_task = 0u;
}

static void mouse_dispatch_record(uint32_t task_id, wm_dispatch_event_type_t dispatch_type,
                                  const input_event_t *event) {
  (void)event;

  switch (dispatch_type) {
  case WM_DISPATCH_MOVE:
    g_mouse_dispatch_stats.move_count += 1u;
    break;
  case WM_DISPATCH_CLICK_DOWN:
    g_mouse_dispatch_stats.button_down_count += 1u;
    g_mouse_dispatch_stats.last_button_down_task = task_id;
    break;
  case WM_DISPATCH_CLICK_UP:
    g_mouse_dispatch_stats.button_up_count += 1u;
    break;
  case WM_DISPATCH_DRAG:
    g_mouse_dispatch_stats.drag_count += 1u;
    g_mouse_dispatch_stats.last_drag_task = task_id;
    break;
  default:
    break;
  }
}

typedef struct keyboard_dispatch_stats {
  uint32_t text_count_task_1;
  uint32_t text_count_task_2;
  uint32_t control_count_task_1;
  uint32_t control_count_task_2;
  uint32_t invalid_endpoint_count;
  uint32_t marker_hash;
} keyboard_dispatch_stats_t;

static keyboard_dispatch_stats_t g_keyboard_dispatch_stats;

static void keyboard_dispatch_stats_reset(void) {
  g_keyboard_dispatch_stats.text_count_task_1 = 0u;
  g_keyboard_dispatch_stats.text_count_task_2 = 0u;
  g_keyboard_dispatch_stats.control_count_task_1 = 0u;
  g_keyboard_dispatch_stats.control_count_task_2 = 0u;
  g_keyboard_dispatch_stats.invalid_endpoint_count = 0u;
  g_keyboard_dispatch_stats.marker_hash = 2166136261u;
}

static void keyboard_dispatch_hash_byte(uint8_t value) {
  g_keyboard_dispatch_stats.marker_hash ^= (uint32_t)value;
  g_keyboard_dispatch_stats.marker_hash *= 16777619u;
}

static void keyboard_dispatch_record(uint32_t endpoint_id, const keyboard_event_t *event) {
  if (event == (const keyboard_event_t *)0) {
    return;
  }

  keyboard_dispatch_hash_byte((uint8_t)(endpoint_id & 0xffu));
  keyboard_dispatch_hash_byte((uint8_t)event->type);

  switch (event->type) {
  case KEYBOARD_EVENT_TEXT:
    keyboard_dispatch_hash_byte((uint8_t)event->text);
    if (endpoint_id == 1u) {
      g_keyboard_dispatch_stats.text_count_task_1 += 1u;
    } else if (endpoint_id == 2u) {
      g_keyboard_dispatch_stats.text_count_task_2 += 1u;
    } else {
      g_keyboard_dispatch_stats.invalid_endpoint_count += 1u;
    }
    break;
  case KEYBOARD_EVENT_CONTROL:
    keyboard_dispatch_hash_byte(event->control);
    if (endpoint_id == 1u) {
      g_keyboard_dispatch_stats.control_count_task_1 += 1u;
    } else if (endpoint_id == 2u) {
      g_keyboard_dispatch_stats.control_count_task_2 += 1u;
    } else {
      g_keyboard_dispatch_stats.invalid_endpoint_count += 1u;
    }
    break;
  default:
    g_keyboard_dispatch_stats.invalid_endpoint_count += 1u;
    break;
  }
}

void kernel_main(void) {
  uint32_t marker_a;
  uint32_t marker_b;
  uint32_t wm_marker;
  uint32_t overlap_marker_before;
  uint32_t overlap_marker_after;
  uint32_t mouse_marker = 0u;
  uint32_t keyboard_marker = 0u;
  uint32_t mouse_events_processed = 0u;
  uint32_t keyboard_events_front = 0u;
  uint32_t keyboard_events_back = 0u;
  uint32_t keyboard_events_processed = 0u;
  uint32_t multi_terminal_marker = 0u;
  uint32_t front_initial_x = 0u;
  uint32_t front_initial_y = 0u;
  int overlap_ok = 0;
  int mouse_ok = 0;
  int keyboard_ok = 0;
  int multi_terminal_ok = 0;
  const wm_window_t *hit_before;
  const wm_window_t *hit_after;
  const wm_window_t *active_window;
  wm_window_t main_window;
  wm_window_t back_window;
  wm_window_t front_window;

  console_init();
  mm_init();
  line_io_write("BOOT: kernel entry\n");
  trap_init();
  line_io_write("console: line io ready\n");
  trap_test_trigger();
  clock_init();
  sched_bootstrap_test_tasks();

  if (framebuffer_init() != 0) {
    line_io_write("GFX: framebuffer init failed\n");
    for (;;) {
      __asm__ volatile("wfi");
    }
  }

  line_io_write("GFX: framebuffer initialized\n");

  marker_a = framebuffer_render_test_pattern();
  marker_b = framebuffer_render_test_pattern();

  if (marker_a == marker_b) {
    line_io_write("GFX: deterministic marker 0x");
    console_put_hex32(marker_a);
    line_io_write("\n");
  } else {
    line_io_write("GFX: marker mismatch 0x");
    console_put_hex32(marker_a);
    line_io_write(" != 0x");
    console_put_hex32(marker_b);
    line_io_write("\n");
  }

  wm_window_init(&main_window, "Terminal", 32u, 20u, 220u, 140u);
  main_window.style.title_bar_color = 0x002f4f89u;
  main_window.style.content_color = 0x00f8f9fbu;
  wm_compositor_reset(0x00161c26u);

  if (wm_compositor_add_window(&main_window) == 0) {
    wm_marker = wm_compositor_render();
    line_io_write("WM: single window composed marker 0x");
    console_put_hex32(wm_marker);
    line_io_write("\n");
  } else {
    line_io_write("WM: single window compose failed\n");
  }

  wm_window_init(&back_window, "Back", 48u, 40u, 220u, 160u);
  back_window.style.title_bar_color = 0x00326f95u;
  back_window.style.content_color = 0x00e9f3fbu;
  wm_window_init(&front_window, "Front", 120u, 92u, 220u, 160u);
  front_window.style.title_bar_color = 0x00814444u;
  front_window.style.content_color = 0x00f5e6deu;

  wm_compositor_reset(0x0012181fu);
  if (wm_compositor_add_window(&back_window) == 0 && wm_compositor_add_window(&front_window) == 0) {
    hit_before = wm_compositor_hit_test(140u, 120u);
    overlap_marker_before = wm_compositor_render();
    if (wm_compositor_activate_window(&back_window) == 0) {
      hit_after = wm_compositor_hit_test(140u, 120u);
      active_window = wm_compositor_active_window();
      overlap_marker_after = wm_compositor_render();
      if (hit_before == &front_window && hit_after == &back_window &&
          active_window == &back_window && overlap_marker_before != overlap_marker_after) {
        overlap_ok = 1;
      }
    }
  }

  if (overlap_ok != 0) {
    line_io_write("WM: overlap focus activation marker 0x");
    console_put_hex32(overlap_marker_after);
    line_io_write("\n");
  } else {
    line_io_write("WM: overlap focus activation failed\n");
  }

  wm_window_init(&back_window, "Back", 48u, 40u, 220u, 160u);
  back_window.style.title_bar_color = 0x00326f95u;
  back_window.style.content_color = 0x00e9f3fbu;
  wm_window_init(&front_window, "Front", 120u, 92u, 220u, 160u);
  front_window.style.title_bar_color = 0x00814444u;
  front_window.style.content_color = 0x00f5e6deu;
  wm_compositor_reset(0x0012171du);
  wm_drag_reset();
  wm_drag_set_dispatch(mouse_dispatch_record);
  mouse_dispatch_reset();
  mouse_reset();

  if (wm_compositor_add_window(&back_window) == 0 && wm_compositor_add_window(&front_window) == 0 &&
      wm_drag_register_window(&back_window, 1u) == 0 && wm_drag_register_window(&front_window, 2u) == 0) {
    front_initial_x = front_window.frame.x;
    front_initial_y = front_window.frame.y;

    (void)mouse_emit_move(150u, 100u, 0u);
    (void)mouse_emit_button_down(150u, 100u, INPUT_MOUSE_BUTTON_LEFT);
    (void)mouse_emit_move(180u, 120u, INPUT_MOUSE_BUTTON_LEFT);
    (void)mouse_emit_move(210u, 142u, INPUT_MOUSE_BUTTON_LEFT);
    (void)mouse_emit_button_up(210u, 142u, INPUT_MOUSE_BUTTON_LEFT);

    mouse_events_processed = wm_drag_dispatch_pending();
    active_window = wm_compositor_active_window();
    mouse_marker = wm_compositor_render();
    if (mouse_events_processed == 5u && g_mouse_dispatch_stats.button_down_count == 1u &&
        g_mouse_dispatch_stats.last_button_down_task == 2u && g_mouse_dispatch_stats.drag_count >= 1u &&
        g_mouse_dispatch_stats.last_drag_task == 2u && front_window.frame.x > front_initial_x &&
        front_window.frame.y > front_initial_y && back_window.frame.x == 48u &&
        back_window.frame.y == 40u && active_window == &front_window) {
      mouse_ok = 1;
    }
  }

  if (mouse_ok != 0) {
    line_io_write("WM: mouse dispatch drag marker 0x");
    console_put_hex32(mouse_marker);
    line_io_write("\n");
  } else {
    line_io_write("WM: mouse dispatch drag failed\n");
  }

  wm_window_init(&back_window, "Back", 48u, 40u, 220u, 160u);
  back_window.style.title_bar_color = 0x00326f95u;
  back_window.style.content_color = 0x00e9f3fbu;
  wm_window_init(&front_window, "Front", 120u, 92u, 220u, 160u);
  front_window.style.title_bar_color = 0x00814444u;
  front_window.style.content_color = 0x00f5e6deu;
  wm_compositor_reset(0x0014131au);
  keyboard_reset();
  keyboard_dispatch_reset();
  keyboard_dispatch_set_sink(keyboard_dispatch_record);
  keyboard_dispatch_stats_reset();

  if (wm_compositor_add_window(&back_window) == 0 && wm_compositor_add_window(&front_window) == 0 &&
      keyboard_dispatch_register_window(&back_window, 1u) == 0 &&
      keyboard_dispatch_register_window(&front_window, 2u) == 0) {
    (void)keyboard_handle_scancode(0x23u);
    (void)keyboard_handle_scancode(0x17u);
    (void)keyboard_handle_scancode(0x1cu);
    keyboard_events_front = keyboard_dispatch_pending();

    if (wm_compositor_activate_window(&back_window) == 0) {
      (void)keyboard_handle_scancode(0x18u);
      (void)keyboard_handle_scancode(0x0eu);
      keyboard_events_back = keyboard_dispatch_pending();
    }

    active_window = wm_compositor_active_window();
    keyboard_events_processed = keyboard_events_front + keyboard_events_back;
    keyboard_marker = g_keyboard_dispatch_stats.marker_hash;

    if (keyboard_events_front == 3u && keyboard_events_back == 2u && keyboard_events_processed == 5u &&
        keyboard_pending_count() == 0u && g_keyboard_dispatch_stats.text_count_task_1 == 1u &&
        g_keyboard_dispatch_stats.control_count_task_1 == 1u &&
        g_keyboard_dispatch_stats.text_count_task_2 == 2u &&
        g_keyboard_dispatch_stats.control_count_task_2 == 1u &&
        g_keyboard_dispatch_stats.invalid_endpoint_count == 0u && active_window == &back_window) {
      keyboard_ok = 1;
    }
  }

  if (keyboard_ok != 0) {
    line_io_write("WM: keyboard focus routing marker 0x");
    console_put_hex32(keyboard_marker);
    line_io_write("\n");
  } else {
    line_io_write("WM: keyboard focus routing failed\n");
  }

  if (multi_terminal_test_run(&multi_terminal_marker) == 0) {
    multi_terminal_ok = 1;
  }

  if (multi_terminal_ok != 0) {
    line_io_write("WM: multi terminal isolation marker 0x");
    console_put_hex32(multi_terminal_marker);
    line_io_write("\n");
  } else {
    line_io_write("WM: multi terminal isolation failed\n");
  }

  if (demo_window_app_register() == 0) {
    demo_window_app_run_once();
  } else {
    line_io_write("APP: demo window register failed\n");
  }

  shell_run();
}
