#include <stdint.h>

#include "console.h"
#include "framebuffer.h"
#include "line_io.h"
#include "mm_init.h"
#include "trap.h"

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
  char line[128];

  console_init();
  trap_init();
  mm_init();
  line_io_write("BOOT: kernel entry\n");
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

  for (;;) {
    line_io_write("shell> ");
    if (line_io_readline(line, sizeof(line), true) > 0) {
      line_io_write("echo: ");
      line_io_write(line);
      line_io_write("\n");
    }
  }
}
