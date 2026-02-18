#include "shell_parser.h"

static int is_space(char ch) {
  return ch == ' ' || ch == '\t';
}

int shell_parse_line(char *line, char **argv, unsigned int argv_cap) {
  unsigned int argc = 0u;
  char *cursor;

  if (line == 0 || argv == 0 || argv_cap == 0u) {
    return 0;
  }

  cursor = line;
  while (*cursor != '\0') {
    while (is_space(*cursor)) {
      *cursor = '\0';
      ++cursor;
    }

    if (*cursor == '\0') {
      break;
    }

    if (argc >= argv_cap) {
      break;
    }

    argv[argc++] = cursor;

    while (*cursor != '\0' && !is_space(*cursor)) {
      ++cursor;
    }

    if (*cursor == '\0') {
      break;
    }

    *cursor = '\0';
    ++cursor;
  }

  return (int)argc;
}
