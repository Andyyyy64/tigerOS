#include "shell_parser.h"

typedef enum {
  PARSE_TOKEN_END = 0,
  PARSE_TOKEN_WORD = 1,
  PARSE_TOKEN_PIPE = 2,
  PARSE_TOKEN_REDIR_TRUNC = 3,
  PARSE_TOKEN_REDIR_APPEND = 4,
  PARSE_TOKEN_ERROR = 5,
} shell_parse_token_t;

static int is_space(char ch) {
  return ch == ' ' || ch == '\t';
}

static void shell_parse_result_init(shell_parse_result_t *out) {
  unsigned int i = 0u;

  out->left.argc = 0;
  out->right.argc = 0;
  out->has_pipe = 0;
  out->redir_mode = SHELL_REDIR_NONE;
  out->redir_path = (char *)0;

  for (i = 0u; i < SHELL_PARSE_ARGV_CAP; ++i) {
    out->left.argv[i] = (char *)0;
    out->right.argv[i] = (char *)0;
  }
}

static shell_parse_token_t parse_next_token(char **cursor_io,
                                            int *pending_special,
                                            char **out_word) {
  char *cursor = *cursor_io;

  while (*cursor != '\0' && is_space(*cursor)) {
    *cursor = '\0';
    ++cursor;
  }

  if (*pending_special != 0) {
    int special = *pending_special;
    *pending_special = 0;
    *cursor_io = cursor;

    if (special == '|') {
      return PARSE_TOKEN_PIPE;
    }
    if (special == '>') {
      if (*cursor == '>') {
        *cursor = '\0';
        ++cursor;
        *cursor_io = cursor;
        return PARSE_TOKEN_REDIR_APPEND;
      }
      return PARSE_TOKEN_REDIR_TRUNC;
    }
    return PARSE_TOKEN_ERROR;
  }

  if (*cursor == '\0') {
    *cursor_io = cursor;
    return PARSE_TOKEN_END;
  }

  if (*cursor == '|') {
    *cursor = '\0';
    ++cursor;
    *cursor_io = cursor;
    return PARSE_TOKEN_PIPE;
  }

  if (*cursor == '>') {
    *cursor = '\0';
    ++cursor;
    if (*cursor == '>') {
      *cursor = '\0';
      ++cursor;
      *cursor_io = cursor;
      return PARSE_TOKEN_REDIR_APPEND;
    }
    *cursor_io = cursor;
    return PARSE_TOKEN_REDIR_TRUNC;
  }

  *out_word = cursor;

  while (*cursor != '\0' && !is_space(*cursor) && *cursor != '|' && *cursor != '>') {
    ++cursor;
  }

  if (*cursor == '\0') {
    *cursor_io = cursor;
    return PARSE_TOKEN_WORD;
  }

  if (*cursor == '|' || *cursor == '>') {
    *pending_special = (int)(unsigned char)(*cursor);
    *cursor = '\0';
    ++cursor;
    *cursor_io = cursor;
    return PARSE_TOKEN_WORD;
  }

  *cursor = '\0';
  ++cursor;
  *cursor_io = cursor;
  return PARSE_TOKEN_WORD;
}

int shell_parse_with_redirection(char *line, shell_parse_result_t *out) {
  shell_simple_command_t *current;
  char *cursor;
  int pending_special = 0;
  int need_redir_path = 0;

  if (line == (char *)0 || out == (shell_parse_result_t *)0) {
    return -1;
  }

  shell_parse_result_init(out);
  current = &out->left;
  cursor = line;

  for (;;) {
    char *word = (char *)0;
    shell_parse_token_t token = parse_next_token(&cursor, &pending_special, &word);

    if (token == PARSE_TOKEN_END) {
      break;
    }

    if (token == PARSE_TOKEN_ERROR) {
      return -1;
    }

    if (token == PARSE_TOKEN_WORD) {
      if (need_redir_path != 0) {
        out->redir_path = word;
        need_redir_path = 0;
        continue;
      }

      if (out->redir_mode != SHELL_REDIR_NONE) {
        return -1;
      }

      if (current->argc >= SHELL_PARSE_ARGV_CAP) {
        return -1;
      }

      current->argv[(unsigned int)current->argc++] = word;
      continue;
    }

    if (need_redir_path != 0) {
      return -1;
    }

    if (token == PARSE_TOKEN_PIPE) {
      if (out->has_pipe != 0 || out->redir_mode != SHELL_REDIR_NONE || current->argc == 0) {
        return -1;
      }
      out->has_pipe = 1;
      current = &out->right;
      continue;
    }

    if (token == PARSE_TOKEN_REDIR_TRUNC || token == PARSE_TOKEN_REDIR_APPEND) {
      if (out->redir_mode != SHELL_REDIR_NONE) {
        return -1;
      }
      out->redir_mode =
          (token == PARSE_TOKEN_REDIR_APPEND) ? SHELL_REDIR_APPEND : SHELL_REDIR_TRUNC;
      need_redir_path = 1;
      continue;
    }
  }

  if (need_redir_path != 0) {
    return -1;
  }

  if (out->left.argc == 0) {
    return -1;
  }

  if (out->has_pipe != 0 && out->right.argc == 0) {
    return -1;
  }

  return 0;
}
