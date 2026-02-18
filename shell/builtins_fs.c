#include <stddef.h>

#include "line_io.h"
#include "path_state.h"
#include "shell_builtins.h"
#include "shell_builtins_fs.h"

enum {
  SHELL_FS_LS_MAX = 64,
  SHELL_FS_PATH_MAX = 256,
};

static size_t shell_strlen(const char *s) {
  size_t len = 0u;
  if (s == NULL) {
    return 0u;
  }
  while (s[len] != '\0') {
    ++len;
  }
  return len;
}

static int shell_write_path(const char *path) {
  if (path == NULL) {
    return -1;
  }
  line_io_write(path);
  return 0;
}

void shell_builtins_fs_init(void) { path_state_init(); }

int shell_builtin_pwd(int argc, char **argv) {
  char cwd[SHELL_FS_PATH_MAX];

  (void)argc;
  (void)argv;

  if (path_state_pwd(cwd, sizeof(cwd)) != 0) {
    line_io_write("pwd: error\n");
    return SHELL_EXEC_OK;
  }

  (void)shell_write_path(cwd);
  line_io_write("\n");
  return SHELL_EXEC_OK;
}

int shell_builtin_cd(int argc, char **argv) {
  const char *target = "/";

  if (argc >= 2 && argv[1] != NULL) {
    target = argv[1];
  }

  if (path_state_cd(target) != 0) {
    line_io_write("cd: no such directory\n");
  }

  return SHELL_EXEC_OK;
}

int shell_builtin_mkdir(int argc, char **argv) {
  int i;

  if (argc < 2) {
    line_io_write("mkdir: missing path\n");
    return SHELL_EXEC_OK;
  }

  for (i = 1; i < argc; ++i) {
    if (argv[i] == NULL || path_state_mkdir(argv[i]) != 0) {
      line_io_write("mkdir: failed: ");
      if (argv[i] != NULL) {
        line_io_write(argv[i]);
      }
      line_io_write("\n");
    }
  }

  return SHELL_EXEC_OK;
}

int shell_builtin_ls(int argc, char **argv) {
  path_state_entry_t entries[SHELL_FS_LS_MAX];
  size_t count = 0u;
  size_t i = 0u;
  const char *target = ".";

  if (argc >= 2 && argv[1] != NULL) {
    target = argv[1];
  }

  if (path_state_ls(target, entries, SHELL_FS_LS_MAX, &count) != 0) {
    line_io_write("ls: cannot access\n");
    return SHELL_EXEC_OK;
  }

  for (i = 0u; i < count; ++i) {
    line_io_write(entries[i].name);
    if (entries[i].kind == PATH_STATE_ENTRY_DIR) {
      line_io_write("/");
    }
    line_io_write("\n");
  }

  return SHELL_EXEC_OK;
}

int shell_builtin_cat(int argc, char **argv) {
  int i;

  if (argc < 2) {
    line_io_write("cat: missing path\n");
    return SHELL_EXEC_OK;
  }

  for (i = 1; i < argc; ++i) {
    const char *content = NULL;
    size_t content_len;

    if (argv[i] == NULL || path_state_cat(argv[i], &content) != 0) {
      line_io_write("cat: not found: ");
      if (argv[i] != NULL) {
        line_io_write(argv[i]);
      }
      line_io_write("\n");
      continue;
    }

    line_io_write(content);
    content_len = shell_strlen(content);
    if (content_len == 0u || content[content_len - 1u] != '\n') {
      line_io_write("\n");
    }
  }

  return SHELL_EXEC_OK;
}
