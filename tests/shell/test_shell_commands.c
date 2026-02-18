#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "page_alloc.h"
#include "shell_builtins.h"
#include "shell_builtins_fs.h"
#include "shell_parser.h"

#define TEST_ASSERT(cond, msg)                       \
  do {                                               \
    if (!(cond)) {                                   \
      fprintf(stderr, "FAIL: %s\n", (msg));       \
      return 1;                                      \
    }                                                \
  } while (0)

enum {
  TEST_OUTPUT_CAPACITY = 32768,
};

static char g_output[TEST_OUTPUT_CAPACITY];
static size_t g_output_len;

static uintptr_t align_up_page(uintptr_t value) {
  uintptr_t mask = (uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u;
  return (value + mask) & ~mask;
}

static void test_output_reset(void) {
  g_output_len = 0u;
  g_output[0] = '\0';
}

static void test_output_append_char(char c) {
  if (g_output_len + 1u >= TEST_OUTPUT_CAPACITY) {
    return;
  }

  g_output[g_output_len++] = c;
  g_output[g_output_len] = '\0';
}

static void test_output_append(const char *s) {
  size_t i = 0u;

  if (s == NULL) {
    return;
  }

  while (s[i] != '\0') {
    test_output_append_char(s[i]);
    ++i;
  }
}

void line_io_write(const char *s) { test_output_append(s); }

void console_putc(char c) { test_output_append_char(c); }

static int test_parser(void) {
  char line[] = " \t  echo   alpha\tbeta  ";
  char *argv[8] = {0};
  int argc;

  argc = shell_parse_line(line, argv, 8u);
  TEST_ASSERT(argc == 3, "parser should extract three arguments");
  TEST_ASSERT(strcmp(argv[0], "echo") == 0, "argv[0] mismatch");
  TEST_ASSERT(strcmp(argv[1], "alpha") == 0, "argv[1] mismatch");
  TEST_ASSERT(strcmp(argv[2], "beta") == 0, "argv[2] mismatch");

  return 0;
}

static int test_basic_commands(void) {
  char *argv_help[] = {"help", NULL};
  char *argv_echo[] = {"echo", "shell", "ok", NULL};
  char *argv_unknown[] = {"not-a-command", NULL};
  static uint8_t alloc_region[(3u * PAGE_ALLOC_PAGE_SIZE) + 128u];
  uintptr_t alloc_start = align_up_page((uintptr_t)&alloc_region[0] + 13u);
  uintptr_t alloc_end = alloc_start + (2u * (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  char *argv_meminfo[] = {"meminfo", NULL};
  int rc;

  page_alloc_init(alloc_start, alloc_end);
  shell_builtins_fs_init();

  test_output_reset();
  rc = shell_execute_builtin(1, argv_help);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "help should execute successfully");
  TEST_ASSERT(strstr(g_output, "available commands:\n") != NULL, "help header missing");
  TEST_ASSERT(strstr(g_output, "help - show this help\n") != NULL, "help command missing");
  TEST_ASSERT(strstr(g_output, "echo - print arguments\n") != NULL, "echo command missing");
  TEST_ASSERT(strstr(g_output, "ls - list files and directories\n") != NULL,
              "ls command missing");
  TEST_ASSERT(strstr(g_output, "cat - print file contents\n") != NULL, "cat command missing");
  TEST_ASSERT(strstr(g_output, "pwd - print current directory\n") != NULL, "pwd command missing");
  TEST_ASSERT(strstr(g_output, "cd - change current directory\n") != NULL, "cd command missing");
  TEST_ASSERT(strstr(g_output, "mkdir - create directory\n") != NULL,
              "mkdir command missing");

  test_output_reset();
  rc = shell_execute_builtin(3, argv_echo);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "echo should execute successfully");
  TEST_ASSERT(strcmp(g_output, "echo: shell ok\n") == 0, "echo output mismatch");

  test_output_reset();
  rc = shell_execute_builtin(1, argv_meminfo);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "meminfo should execute successfully");
  TEST_ASSERT(strstr(g_output, "meminfo: range=0x") != NULL, "meminfo range prefix missing");
  TEST_ASSERT(strstr(g_output, " page_size=4096") != NULL, "meminfo page size missing");
  TEST_ASSERT(strstr(g_output, " total_pages=2") != NULL, "meminfo total page count mismatch");
  TEST_ASSERT(strstr(g_output, " free_pages=2") != NULL, "meminfo free page count mismatch");

  rc = shell_execute_builtin(1, argv_unknown);
  TEST_ASSERT(rc == SHELL_EXEC_NOT_FOUND, "unknown command should be reported as not found");

  return 0;
}

static int test_shell_fs_commands(void) {
  char *argv_pwd[] = {"pwd", NULL};
  char *argv_ls[] = {"ls", NULL};
  char *argv_cat[] = {"cat", "hello.txt", NULL};
  char *argv_mkdir[] = {"mkdir", "projects", NULL};
  char *argv_cd_projects[] = {"cd", "projects", NULL};
  char *argv_mkdir_notes[] = {"mkdir", "notes", NULL};
  char *argv_cd_missing[] = {"cd", "/missing", NULL};
  int rc;

  shell_builtins_fs_init();

  test_output_reset();
  rc = shell_execute_builtin(1, argv_pwd);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "pwd should execute successfully");
  TEST_ASSERT(strcmp(g_output, "/\n") == 0, "pwd should start at root");

  test_output_reset();
  rc = shell_execute_builtin(1, argv_ls);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "ls should execute successfully");
  TEST_ASSERT(strstr(g_output, "etc/\n") != NULL, "ls should include etc directory");
  TEST_ASSERT(strstr(g_output, "home/\n") != NULL, "ls should include home directory");
  TEST_ASSERT(strstr(g_output, "tmp/\n") != NULL, "ls should include tmp directory");
  TEST_ASSERT(strstr(g_output, "hello.txt\n") != NULL, "ls should include hello.txt");

  test_output_reset();
  rc = shell_execute_builtin(2, argv_cat);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "cat should execute successfully");
  TEST_ASSERT(strcmp(g_output, "hello from shell fs\n") == 0, "cat output mismatch");

  test_output_reset();
  rc = shell_execute_builtin(2, argv_mkdir);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "mkdir projects should execute successfully");
  TEST_ASSERT(g_output[0] == '\0', "successful mkdir should be silent");

  test_output_reset();
  rc = shell_execute_builtin(2, argv_cd_projects);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "cd projects should execute successfully");
  TEST_ASSERT(g_output[0] == '\0', "successful cd should be silent");

  test_output_reset();
  rc = shell_execute_builtin(1, argv_pwd);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "pwd in /projects should execute successfully");
  TEST_ASSERT(strcmp(g_output, "/projects\n") == 0, "pwd should reflect updated cwd");

  test_output_reset();
  rc = shell_execute_builtin(2, argv_mkdir_notes);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "mkdir notes should execute successfully");
  TEST_ASSERT(g_output[0] == '\0', "successful mkdir notes should be silent");

  test_output_reset();
  rc = shell_execute_builtin(1, argv_ls);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "ls in /projects should execute successfully");
  TEST_ASSERT(strcmp(g_output, "notes/\n") == 0, "ls should show notes directory in /projects");

  test_output_reset();
  rc = shell_execute_builtin(2, argv_cd_missing);
  TEST_ASSERT(rc == SHELL_EXEC_OK, "cd missing path should still return shell OK");
  TEST_ASSERT(strcmp(g_output, "cd: no such directory\n") == 0,
              "cd missing should emit deterministic error");

  return 0;
}

int main(void) {
  if (test_parser() != 0) {
    return 1;
  }
  if (test_basic_commands() != 0) {
    return 1;
  }
  if (test_shell_fs_commands() != 0) {
    return 1;
  }

  printf("shell command tests passed\n");
  return 0;
}
