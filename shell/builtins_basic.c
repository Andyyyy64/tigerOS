#include <stddef.h>
#include <stdint.h>

#include "console.h"
#include "line_io.h"
#include "page_alloc.h"
#include "shell_builtins.h"

typedef int (*shell_builtin_fn_t)(int argc, char **argv);

typedef struct shell_builtin {
  const char *name;
  const char *help;
  shell_builtin_fn_t fn;
} shell_builtin_t;

static int shell_str_eq(const char *a, const char *b) {
  if (a == 0 || b == 0) {
    return 0;
  }

  while (*a != '\0' && *b != '\0') {
    if (*a != *b) {
      return 0;
    }
    ++a;
    ++b;
  }

  return *a == '\0' && *b == '\0';
}

static void shell_write_u64(uint64_t value) {
  char scratch[20];
  unsigned int count = 0u;

  if (value == 0u) {
    console_putc('0');
    return;
  }

  while (value != 0u && count < sizeof(scratch)) {
    scratch[count++] = (char)('0' + (value % 10u));
    value /= 10u;
  }

  while (count > 0u) {
    console_putc(scratch[--count]);
  }
}

static void shell_write_hex_uintptr(uintptr_t value) {
  static const char digits[] = "0123456789abcdef";
  unsigned int shift = (unsigned int)(sizeof(uintptr_t) * 8u);

  line_io_write("0x");

  while (shift > 0u) {
    shift -= 4u;
    console_putc(digits[(value >> shift) & 0xfu]);
  }
}

static int shell_builtin_help(int argc, char **argv);
static int shell_builtin_echo(int argc, char **argv);
static int shell_builtin_meminfo(int argc, char **argv);

static const shell_builtin_t g_shell_builtins[] = {
    {"help", "show this help", shell_builtin_help},
    {"echo", "print arguments", shell_builtin_echo},
    {"meminfo", "show allocator usage", shell_builtin_meminfo},
};

static int shell_builtin_help(int argc, char **argv) {
  unsigned int i;

  (void)argc;
  (void)argv;

  line_io_write("available commands:\n");
  for (i = 0u; i < (sizeof(g_shell_builtins) / sizeof(g_shell_builtins[0])); ++i) {
    line_io_write("  ");
    line_io_write(g_shell_builtins[i].name);
    line_io_write(" - ");
    line_io_write(g_shell_builtins[i].help);
    line_io_write("\n");
  }

  return SHELL_EXEC_OK;
}

static int shell_builtin_echo(int argc, char **argv) {
  int i;

  line_io_write("echo:");
  if (argc > 1) {
    line_io_write(" ");
  }

  for (i = 1; i < argc; ++i) {
    line_io_write(argv[i]);
    if (i + 1 < argc) {
      line_io_write(" ");
    }
  }

  line_io_write("\n");
  return SHELL_EXEC_OK;
}

static int shell_builtin_meminfo(int argc, char **argv) {
  size_t total_pages;
  size_t free_pages;
  size_t used_pages;
  uintptr_t range_start;
  uintptr_t range_end;

  (void)argc;
  (void)argv;

  total_pages = page_alloc_total_pages();
  free_pages = page_alloc_free_pages();
  used_pages = total_pages - free_pages;
  range_start = page_alloc_range_start();
  range_end = page_alloc_range_end();

  line_io_write("meminfo: range=");
  shell_write_hex_uintptr(range_start);
  line_io_write("-");
  shell_write_hex_uintptr(range_end);
  line_io_write(" page_size=");
  shell_write_u64((uint64_t)PAGE_ALLOC_PAGE_SIZE);
  line_io_write(" total_pages=");
  shell_write_u64((uint64_t)total_pages);
  line_io_write(" free_pages=");
  shell_write_u64((uint64_t)free_pages);
  line_io_write(" used_pages=");
  shell_write_u64((uint64_t)used_pages);
  line_io_write("\n");

  return SHELL_EXEC_OK;
}

int shell_execute_builtin(int argc, char **argv) {
  unsigned int i;

  if (argc <= 0 || argv == 0 || argv[0] == 0) {
    return SHELL_EXEC_OK;
  }

  for (i = 0u; i < (sizeof(g_shell_builtins) / sizeof(g_shell_builtins[0])); ++i) {
    if (shell_str_eq(argv[0], g_shell_builtins[i].name)) {
      return g_shell_builtins[i].fn(argc, argv);
    }
  }

  return SHELL_EXEC_NOT_FOUND;
}
