#include <stdint.h>

#include "console.h"
#include "framebuffer.h"
#include "line_io.h"
#include "mm_init.h"

static void line_io_write_hex32(uint32_t value) {
  static const char digits[] = "0123456789ABCDEF";
  char out[11];
  int i;

  out[0] = '0';
  out[1] = 'x';
  for (i = 0; i < 8; ++i) {
    unsigned int shift = (unsigned int)(28 - (i * 4));
    out[2 + i] = digits[(value >> shift) & 0xFu];
  }
  out[10] = '\0';
  line_io_write(out);
}

void kernel_main(void) {
  char line[128];
  uint32_t marker;

  console_init();
  mm_init();
  line_io_write("BOOT: kernel entry\n");
  line_io_write("console: line io ready\n");
  if (framebuffer_init()) {
    marker = framebuffer_render_test_pattern();
    line_io_write("GFX: pattern marker=");
    line_io_write_hex32(marker);
    line_io_write("\n");
  } else {
    line_io_write("GFX: framebuffer init failed\n");
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
