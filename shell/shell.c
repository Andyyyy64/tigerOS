#include "line_io.h"
#include "shell.h"
#include "shell_builtins_fs.h"
#include "shell_exec_pipeline.h"
#include "shell_fd_table.h"

enum {
  SHELL_LINE_BUFFER_SIZE = 128,
};

static void shell_copy_line(char *dst, unsigned int dst_cap, const char *src) {
  unsigned int i = 0u;

  if (dst == 0 || src == 0 || dst_cap == 0u) {
    return;
  }

  while (i + 1u < dst_cap && src[i] != '\0') {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = '\0';
}

void shell_run(void) {
  char line[SHELL_LINE_BUFFER_SIZE];
  char raw_line[SHELL_LINE_BUFFER_SIZE];

  shell_builtins_fs_init();
  shell_fd_reset();

  for (;;) {
    int line_len;

    line_io_write("shell> ");
    line_len = line_io_readline(line, sizeof(line), true);
    if (line_len <= 0) {
      continue;
    }

    shell_copy_line(raw_line, sizeof(raw_line), line);
    (void)shell_execute_line(line, raw_line);
  }
}
