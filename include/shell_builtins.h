#ifndef SHELL_BUILTINS_H
#define SHELL_BUILTINS_H

enum {
  SHELL_EXEC_OK = 0,
  SHELL_EXEC_NOT_FOUND = 1,
};

int shell_execute_builtin(int argc, char **argv);

#endif
