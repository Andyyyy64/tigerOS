#include "line_io.h"
#include "shell.h"
#include "shell_builtins.h"
#include "shell_builtins_fs.h"
#include "shell_parser.h"

enum {
  SHELL_LINE_BUFFER_SIZE = 128,
  SHELL_ARGV_CAP = 16,
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
  char *argv[SHELL_ARGV_CAP];

  shell_builtins_fs_init();

  for (;;) {
    int line_len;
    int argc;
    int status;

    line_io_write("shell> ");
    line_len = line_io_readline(line, sizeof(line), true);
    if (line_len <= 0) {
      continue;
    }

    shell_copy_line(raw_line, sizeof(raw_line), line);
    argc = shell_parse_line(line, argv, SHELL_ARGV_CAP);
    if (argc <= 0) {
      continue;
    }

    status = shell_execute_builtin(argc, argv);
    if (status == SHELL_EXEC_NOT_FOUND) {
      line_io_write("echo: ");
      line_io_write(raw_line);
      line_io_write("\n");
    }
  }
}
