#include "console.h"
#include "line_io.h"
#include "trap.h"
#include "mm_init.h"

void kernel_main(void) {
  char line[128];

  console_init();
  mm_init();
  line_io_write("BOOT: kernel entry\n");
  trap_init();
  trap_trigger_test();
  line_io_write("console: line io ready\n");

  for (;;) {
    line_io_write("shell> ");
    if (line_io_readline(line, sizeof(line), true) > 0) {
      line_io_write("echo: ");
      line_io_write(line);
      line_io_write("\n");
    }
  }
}
