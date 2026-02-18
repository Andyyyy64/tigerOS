#include <stdint.h>

#include "console.h"
#include "line_io.h"

enum {
  LINE_IO_BUFFER_SIZE = 256,
};

static char g_line_buffer[LINE_IO_BUFFER_SIZE];
static unsigned int g_line_len;
static bool g_skip_lf_after_cr;

static unsigned int line_io_copy_out(char *out, unsigned int out_len) {
  unsigned int copy_len = g_line_len;
  unsigned int i;

  if (copy_len >= out_len) {
    copy_len = out_len - 1u;
  }

  for (i = 0u; i < copy_len; ++i) {
    out[i] = g_line_buffer[i];
  }

  out[copy_len] = '\0';
  return copy_len;
}

static void line_io_handle_backspace(void) {
  if (g_line_len == 0u) {
    return;
  }

  g_line_len--;
  console_write("\b \b");
}

static void line_io_handle_byte(uint8_t byte) {
  if (g_line_len + 1u >= LINE_IO_BUFFER_SIZE) {
    return;
  }

  g_line_buffer[g_line_len++] = (char)byte;
  console_putc((char)byte);
}

void line_io_write(const char *s) {
  console_write(s);
}

int line_io_readline(char *out, unsigned int out_len, bool blocking) {
  unsigned int line_len;

  if (out == 0 || out_len == 0u) {
    return -1;
  }

  for (;;) {
    int byte = blocking ? (int)console_getc_blocking() : console_getc_nonblocking();
    if (byte < 0) {
      return 0;
    }

    if (g_skip_lf_after_cr) {
      g_skip_lf_after_cr = false;
      if (byte == '\n') {
        continue;
      }
    }

    if (byte == '\r' || byte == '\n') {
      g_skip_lf_after_cr = (byte == '\r');
      console_write("\n");
      line_len = line_io_copy_out(out, out_len);
      g_line_len = 0u;
      return (int)line_len;
    }

    if (byte == '\b' || byte == 0x7f) {
      line_io_handle_backspace();
      continue;
    }

    if (byte >= 0x20 && byte <= 0x7e) {
      line_io_handle_byte((uint8_t)byte);
    }
  }
}
