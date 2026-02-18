#include <stdint.h>

#include "console.h"
#include "framebuffer.h"
#include "line_io.h"
#include "mm_init.h"
#include "trap.h"
#include "wm_compositor.h"
#include "wm_window.h"

static void console_put_hex32(uint32_t value) {
  static const char digits[] = "0123456789ABCDEF";
  int shift;

  for (shift = 28; shift >= 0; shift -= 4) {
    console_putc(digits[(value >> (uint32_t)shift) & 0x0fu]);
  }
}

void kernel_main(void) {
  uint32_t marker_a;
  uint32_t marker_b;
  uint32_t wm_marker;
  uint32_t overlap_marker_before;
  uint32_t overlap_marker_after;
  int overlap_ok = 0;
  const wm_window_t *hit_before;
  const wm_window_t *hit_after;
  const wm_window_t *active_window;
  char line[128];
  wm_window_t main_window;
  wm_window_t back_window;
  wm_window_t front_window;

  console_init();
  mm_init();
  line_io_write("BOOT: kernel entry\n");
  trap_init();
  line_io_write("console: line io ready\n");
  trap_test_trigger();

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

  for (;;) {
    line_io_write("shell> ");
    if (line_io_readline(line, sizeof(line), true) > 0) {
      line_io_write("echo: ");
      line_io_write(line);
      line_io_write("\n");
    }
  }
}
