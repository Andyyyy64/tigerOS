#include <stddef.h>
#include <stdint.h>

#include "page_alloc.h"
#include "shell_builtins.h"
#include "shell_builtins_fs.h"
#include "shell_fd_table.h"

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
    shell_fd_putc('0');
    return;
  }

  while (value != 0u && count < sizeof(scratch)) {
    scratch[count++] = (char)('0' + (value % 10u));
    value /= 10u;
  }

  while (count > 0u) {
    shell_fd_putc(scratch[--count]);
  }
}

static void shell_write_hex_uintptr(uintptr_t value) {
  static const char digits[] = "0123456789abcdef";
  unsigned int shift = (unsigned int)(sizeof(uintptr_t) * 8u);

  shell_fd_write("0x");

  while (shift > 0u) {
    shift -= 4u;
    shell_fd_putc(digits[(value >> shift) & 0xfu]);
  }
}

static int shell_builtin_help(int argc, char **argv);
static int shell_builtin_echo(int argc, char **argv);
static int shell_builtin_meminfo(int argc, char **argv);

static const shell_builtin_t g_shell_builtins[] = {
    {"help", "show this help", shell_builtin_help},
    {"echo", "print arguments", shell_builtin_echo},
    {"meminfo", "show allocator usage", shell_builtin_meminfo},
    {"ls", "list files and directories", shell_builtin_ls},
    {"cat", "print file contents", shell_builtin_cat},
    {"pwd", "print current directory", shell_builtin_pwd},
    {"cd", "change current directory", shell_builtin_cd},
    {"mkdir", "create directory", shell_builtin_mkdir},
};

static int shell_builtin_help(int argc, char **argv) {
  unsigned int i;

  (void)argc;
  (void)argv;

  shell_fd_write("available commands:\n");
  for (i = 0u; i < (sizeof(g_shell_builtins) / sizeof(g_shell_builtins[0])); ++i) {
    shell_fd_write("  ");
    shell_fd_write(g_shell_builtins[i].name);
    shell_fd_write(" - ");
    shell_fd_write(g_shell_builtins[i].help);
    shell_fd_write("\n");
  }

  return SHELL_EXEC_OK;
}

static int shell_builtin_echo(int argc, char **argv) {
  int i;

  shell_fd_write("echo:");
  if (argc > 1) {
    shell_fd_write(" ");
  }

  for (i = 1; i < argc; ++i) {
    shell_fd_write(argv[i]);
    if (i + 1 < argc) {
      shell_fd_write(" ");
    }
  }

  shell_fd_write("\n");
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

  shell_fd_write("meminfo: range=");
  shell_write_hex_uintptr(range_start);
  shell_fd_write("-");
  shell_write_hex_uintptr(range_end);
  shell_fd_write(" page_size=");
  shell_write_u64((uint64_t)PAGE_ALLOC_PAGE_SIZE);
  shell_fd_write(" total_pages=");
  shell_write_u64((uint64_t)total_pages);
  shell_fd_write(" free_pages=");
  shell_write_u64((uint64_t)free_pages);
  shell_fd_write(" used_pages=");
  shell_write_u64((uint64_t)used_pages);
  shell_fd_write("\n");

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
