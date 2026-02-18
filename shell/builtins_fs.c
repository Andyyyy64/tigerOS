#include <stddef.h>

#include "path_state.h"
#include "shell_builtins.h"
#include "shell_builtins_fs.h"
#include "shell_fd_table.h"

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
  shell_fd_write(path);
  return 0;
}

void shell_builtins_fs_init(void) { path_state_init(); }

int shell_builtin_pwd(int argc, char **argv) {
  char cwd[SHELL_FS_PATH_MAX];

  (void)argc;
  (void)argv;

  if (path_state_pwd(cwd, sizeof(cwd)) != 0) {
    shell_fd_write("pwd: error\n");
    return SHELL_EXEC_OK;
  }

  (void)shell_write_path(cwd);
  shell_fd_write("\n");
  return SHELL_EXEC_OK;
}

int shell_builtin_cd(int argc, char **argv) {
  const char *target = "/";

  if (argc >= 2 && argv[1] != NULL) {
    target = argv[1];
  }

  if (path_state_cd(target) != 0) {
    shell_fd_write("cd: no such directory\n");
  }

  return SHELL_EXEC_OK;
}

int shell_builtin_mkdir(int argc, char **argv) {
  int i;

  if (argc < 2) {
    shell_fd_write("mkdir: missing path\n");
    return SHELL_EXEC_OK;
  }

  for (i = 1; i < argc; ++i) {
    if (argv[i] == NULL || path_state_mkdir(argv[i]) != 0) {
      shell_fd_write("mkdir: failed: ");
      if (argv[i] != NULL) {
        shell_fd_write(argv[i]);
      }
      shell_fd_write("\n");
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
    shell_fd_write("ls: cannot access\n");
    return SHELL_EXEC_OK;
  }

  for (i = 0u; i < count; ++i) {
    shell_fd_write(entries[i].name);
    if (entries[i].kind == PATH_STATE_ENTRY_DIR) {
      shell_fd_write("/");
    }
    shell_fd_write("\n");
  }

  return SHELL_EXEC_OK;
}

int shell_builtin_cat(int argc, char **argv) {
  int i;

  if (argc < 2) {
    if (shell_fd_has_stdin() != 0) {
      const char *input = shell_fd_stdin();
      size_t input_len = shell_strlen(input);

      if (input != NULL) {
        shell_fd_write(input);
      }
      if (input == NULL || input_len == 0u || input[input_len - 1u] != '\n') {
        shell_fd_write("\n");
      }
      return SHELL_EXEC_OK;
    }

    shell_fd_write("cat: missing path\n");
    return SHELL_EXEC_OK;
  }

  for (i = 1; i < argc; ++i) {
    const char *content = NULL;
    size_t content_len;

    if (argv[i] == NULL || path_state_cat(argv[i], &content) != 0) {
      shell_fd_write("cat: not found: ");
      if (argv[i] != NULL) {
        shell_fd_write(argv[i]);
      }
      shell_fd_write("\n");
      continue;
    }

    shell_fd_write(content);
    content_len = shell_strlen(content);
    if (content_len == 0u || content[content_len - 1u] != '\n') {
      shell_fd_write("\n");
    }
  }

  return SHELL_EXEC_OK;
}
