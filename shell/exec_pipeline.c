#include <stddef.h>

#include "path_state.h"
#include "shell_builtins.h"
#include "shell_exec_pipeline.h"
#include "shell_fd_table.h"
#include "shell_parser.h"

enum {
  SHELL_FALLBACK_TEXT_CAP = 128,
  SHELL_PIPE_INPUT_CAP = 2048,
};

static void shell_build_fallback_text(int argc, char **argv, char *out, size_t out_cap) {
  size_t used = 0u;
  int i;

  if (out == (char *)0 || out_cap == 0u) {
    return;
  }

  out[0] = '\0';

  for (i = 0; i < argc; ++i) {
    const char *arg = argv[i];

    if (arg == (const char *)0) {
      continue;
    }

    if (used + 1u >= out_cap) {
      break;
    }

    if (i > 0) {
      out[used++] = ' ';
    }

    while (*arg != '\0') {
      if (used + 1u >= out_cap) {
        out[used] = '\0';
        return;
      }
      out[used++] = *arg;
      ++arg;
    }
  }

  out[used] = '\0';
}

static int shell_execute_or_fallback(int argc, char **argv, const char *fallback_text) {
  int status = shell_execute_builtin(argc, argv);

  if (status == SHELL_EXEC_NOT_FOUND) {
    shell_fd_write("echo: ");
    if (fallback_text != (const char *)0) {
      shell_fd_write(fallback_text);
    }
    shell_fd_write("\n");
  }

  return status;
}

static int shell_write_redirection(const shell_parse_result_t *parse_result) {
  int append = (parse_result->redir_mode == SHELL_REDIR_APPEND) ? 1 : 0;

  if (path_state_write_file(parse_result->redir_path, shell_fd_capture_data(), shell_fd_capture_len(),
                            append) != 0) {
    shell_fd_set_stdout_console();
    shell_fd_write("redir: write failed\n");
    return -1;
  }

  return 0;
}

static int shell_execute_single(shell_parse_result_t *parse_result, const char *raw_line) {
  if (parse_result->redir_mode == SHELL_REDIR_NONE) {
    shell_fd_set_stdout_console();
    shell_fd_set_stdin((const char *)0);
    (void)shell_execute_or_fallback(parse_result->left.argc, parse_result->left.argv, raw_line);
    return 0;
  }

  shell_fd_set_stdout_capture();
  shell_fd_set_stdin((const char *)0);
  (void)shell_execute_or_fallback(parse_result->left.argc, parse_result->left.argv, raw_line);

  return shell_write_redirection(parse_result);
}

static int shell_execute_pipe(shell_parse_result_t *parse_result) {
  char left_text[SHELL_FALLBACK_TEXT_CAP];
  char right_text[SHELL_FALLBACK_TEXT_CAP];
  char pipe_input[SHELL_PIPE_INPUT_CAP];
  const char *left_output = (const char *)0;
  size_t left_output_len = 0u;
  size_t i = 0u;

  shell_build_fallback_text(parse_result->left.argc, parse_result->left.argv, left_text,
                            sizeof(left_text));
  shell_build_fallback_text(parse_result->right.argc, parse_result->right.argv, right_text,
                            sizeof(right_text));

  shell_fd_set_stdout_capture();
  shell_fd_set_stdin((const char *)0);
  (void)shell_execute_or_fallback(parse_result->left.argc, parse_result->left.argv, left_text);

  left_output = shell_fd_capture_data();
  left_output_len = shell_fd_capture_len();
  if (left_output_len + 1u > sizeof(pipe_input)) {
    left_output_len = sizeof(pipe_input) - 1u;
  }
  while (i < left_output_len) {
    pipe_input[i] = left_output[i];
    ++i;
  }
  pipe_input[i] = '\0';

  shell_fd_set_stdin(pipe_input);

  if (parse_result->redir_mode == SHELL_REDIR_NONE) {
    shell_fd_set_stdout_console();
    (void)shell_execute_or_fallback(parse_result->right.argc, parse_result->right.argv, right_text);
    shell_fd_set_stdin((const char *)0);
    return 0;
  }

  shell_fd_set_stdout_capture();
  (void)shell_execute_or_fallback(parse_result->right.argc, parse_result->right.argv, right_text);
  shell_fd_set_stdin((const char *)0);

  return shell_write_redirection(parse_result);
}

int shell_execute_line(char *line, const char *raw_line) {
  shell_parse_result_t parse_result;

  if (line == (char *)0 || raw_line == (const char *)0) {
    return -1;
  }

  if (shell_parse_with_redirection(line, &parse_result) != 0) {
    shell_fd_set_stdout_console();
    shell_fd_write("parse: invalid command\n");
    return -1;
  }

  if (parse_result.has_pipe != 0) {
    return shell_execute_pipe(&parse_result);
  }

  return shell_execute_single(&parse_result, raw_line);
}
