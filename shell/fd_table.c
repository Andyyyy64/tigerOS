#include <stddef.h>

#include "console.h"
#include "shell_fd_table.h"

enum {
  SHELL_FD_CAPTURE_CAP = 2048,
};

typedef enum {
  SHELL_STDOUT_CONSOLE = 0,
  SHELL_STDOUT_CAPTURE = 1,
} shell_stdout_mode_t;

static shell_stdout_mode_t g_stdout_mode;
static const char *g_stdin_data;
static char g_capture[SHELL_FD_CAPTURE_CAP];
static size_t g_capture_len;

static void shell_fd_capture_clear(void) {
  g_capture_len = 0u;
  g_capture[0] = '\0';
}

void shell_fd_reset(void) {
  g_stdout_mode = SHELL_STDOUT_CONSOLE;
  g_stdin_data = (const char *)0;
  shell_fd_capture_clear();
}

void shell_fd_set_stdout_console(void) { g_stdout_mode = SHELL_STDOUT_CONSOLE; }

void shell_fd_set_stdout_capture(void) {
  g_stdout_mode = SHELL_STDOUT_CAPTURE;
  shell_fd_capture_clear();
}

void shell_fd_set_stdin(const char *input) { g_stdin_data = input; }

const char *shell_fd_stdin(void) { return g_stdin_data; }

int shell_fd_has_stdin(void) {
  if (g_stdin_data == (const char *)0 || g_stdin_data[0] == '\0') {
    return 0;
  }
  return 1;
}

void shell_fd_putc(char ch) {
  if (g_stdout_mode == SHELL_STDOUT_CONSOLE) {
    console_putc(ch);
    return;
  }

  if (g_capture_len + 1u >= SHELL_FD_CAPTURE_CAP) {
    return;
  }

  g_capture[g_capture_len++] = ch;
  g_capture[g_capture_len] = '\0';
}

void shell_fd_write(const char *s) {
  if (s == (const char *)0) {
    return;
  }

  while (*s != '\0') {
    shell_fd_putc(*s);
    ++s;
  }
}

void shell_fd_write_n(const char *s, size_t len) {
  size_t i = 0u;

  if (s == (const char *)0) {
    return;
  }

  while (i < len) {
    shell_fd_putc(s[i]);
    ++i;
  }
}

const char *shell_fd_capture_data(void) { return g_capture; }

size_t shell_fd_capture_len(void) { return g_capture_len; }
