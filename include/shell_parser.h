#ifndef SHELL_PARSER_H
#define SHELL_PARSER_H

#include <stddef.h>

enum {
  SHELL_PARSE_ARGV_CAP = 16,
};

typedef enum {
  SHELL_REDIR_NONE = 0,
  SHELL_REDIR_TRUNC = 1,
  SHELL_REDIR_APPEND = 2,
} shell_redir_mode_t;

typedef struct {
  int argc;
  char *argv[SHELL_PARSE_ARGV_CAP];
} shell_simple_command_t;

typedef struct {
  shell_simple_command_t left;
  shell_simple_command_t right;
  int has_pipe;
  shell_redir_mode_t redir_mode;
  char *redir_path;
} shell_parse_result_t;

int shell_parse_line(char *line, char **argv, unsigned int argv_cap);
int shell_parse_with_redirection(char *line, shell_parse_result_t *out);

#endif
