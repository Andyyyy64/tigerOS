#include <stddef.h>

#include "shell_parser.h"
#include "tty_session.h"

static int tty_streq(const char *a, const char *b) {
  size_t i = 0u;

  if (a == NULL || b == NULL) {
    return 0;
  }

  while (a[i] != '\0' && b[i] != '\0') {
    if (a[i] != b[i]) {
      return 0;
    }
    ++i;
  }

  return a[i] == b[i];
}

static void tty_copy(char *dst, size_t dst_cap, const char *src) {
  size_t i = 0u;

  if (dst == NULL || dst_cap == 0u) {
    return;
  }

  if (src == NULL) {
    dst[0] = '\0';
    return;
  }

  while (src[i] != '\0' && (i + 1u) < dst_cap) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = '\0';
}

static int tty_execute_command(tty_session_t *session, int argc, char **argv) {
  int i;

  if (session == (tty_session_t *)0 || argc <= 0 || argv == (char **)0 || argv[0] == (char *)0) {
    return -1;
  }

  if (tty_streq(argv[0], "pwd") != 0) {
    char cwd[FS_PATH_MAX];
    return path_state_pwd_ctx(&session->path_context, cwd, sizeof(cwd));
  }

  if (tty_streq(argv[0], "cd") != 0) {
    const char *target = "/";
    if (argc >= 2 && argv[1] != (char *)0) {
      target = argv[1];
    }
    return path_state_cd_ctx(&session->path_context, target);
  }

  if (tty_streq(argv[0], "mkdir") != 0) {
    if (argc < 2) {
      return -1;
    }
    for (i = 1; i < argc; ++i) {
      if (argv[i] == (char *)0 || path_state_mkdir_ctx(&session->path_context, argv[i]) != 0) {
        return -1;
      }
    }
    return 0;
  }

  if (tty_streq(argv[0], "ls") != 0) {
    path_state_entry_t entries[FS_DIR_MAX_NODES];
    size_t count = 0u;
    const char *target = ".";

    if (argc >= 2 && argv[1] != (char *)0) {
      target = argv[1];
    }

    return path_state_ls_ctx(&session->path_context, target, entries, FS_DIR_MAX_NODES, &count);
  }

  if (tty_streq(argv[0], "cat") != 0) {
    int cat_status;
    const char *content = (const char *)0;

    if (argc < 2 || argv[1] == (char *)0) {
      return -1;
    }

    cat_status = path_state_cat_ctx(&session->path_context, argv[1], &content);
    if (cat_status != 0 || content == (const char *)0) {
      return -1;
    }
    return 0;
  }

  if (tty_streq(argv[0], "help") != 0 || tty_streq(argv[0], "echo") != 0) {
    return 0;
  }

  return -1;
}

static void tty_record_history(tty_session_t *session, const char *line) {
  uint32_t index;

  if (session == (tty_session_t *)0 || line == (const char *)0) {
    return;
  }

  index = session->history_next_index;
  tty_copy(session->history[index], sizeof(session->history[index]), line);

  session->history_next_index += 1u;
  if (session->history_next_index >= TTY_SESSION_HISTORY_DEPTH) {
    session->history_next_index = 0u;
  }

  if (session->history_count < TTY_SESSION_HISTORY_DEPTH) {
    session->history_count += 1u;
  }
}

static int tty_commit_line(tty_session_t *session) {
  char raw_line[TTY_SESSION_INPUT_CAPACITY];
  char parse_line[TTY_SESSION_INPUT_CAPACITY];
  char *argv[16];
  int argc;

  if (session == (tty_session_t *)0) {
    return -1;
  }

  session->input_buffer[session->input_len] = '\0';
  tty_copy(raw_line, sizeof(raw_line), session->input_buffer);
  tty_copy(parse_line, sizeof(parse_line), session->input_buffer);
  session->input_len = 0u;
  session->input_buffer[0] = '\0';

  if (raw_line[0] == '\0') {
    return 0;
  }

  tty_record_history(session, raw_line);

  argc = shell_parse_line(parse_line, argv, (unsigned int)(sizeof(argv) / sizeof(argv[0])));
  if (argc <= 0) {
    return 0;
  }

  session->command_count += 1u;
  return tty_execute_command(session, argc, argv);
}

void tty_session_init(tty_session_t *session, uint32_t session_id) {
  uint32_t i;
  uint32_t j;

  if (session == (tty_session_t *)0) {
    return;
  }

  session->session_id = session_id;
  session->input_len = 0u;
  session->input_buffer[0] = '\0';
  session->history_count = 0u;
  session->history_next_index = 0u;
  session->command_count = 0u;

  for (i = 0u; i < TTY_SESSION_HISTORY_DEPTH; ++i) {
    for (j = 0u; j < TTY_SESSION_INPUT_CAPACITY; ++j) {
      session->history[i][j] = '\0';
    }
  }

  path_state_context_init(&session->path_context);
}

int tty_session_handle_keyboard_event(tty_session_t *session, const keyboard_event_t *event) {
  if (session == (tty_session_t *)0 || event == (const keyboard_event_t *)0) {
    return -1;
  }

  if (event->type == KEYBOARD_EVENT_TEXT) {
    if (event->text == '\0') {
      return -1;
    }

    if (session->input_len + 1u >= TTY_SESSION_INPUT_CAPACITY) {
      return -1;
    }

    session->input_buffer[session->input_len] = event->text;
    session->input_len += 1u;
    session->input_buffer[session->input_len] = '\0';
    return 0;
  }

  if (event->type != KEYBOARD_EVENT_CONTROL) {
    return -1;
  }

  if (event->control == KEYBOARD_CONTROL_BACKSPACE) {
    if (session->input_len == 0u) {
      return 0;
    }

    session->input_len -= 1u;
    session->input_buffer[session->input_len] = '\0';
    return 0;
  }

  if (event->control == KEYBOARD_CONTROL_ENTER) {
    return tty_commit_line(session);
  }

  return 0;
}

int tty_session_get_cwd(tty_session_t *session, char *out, size_t out_len) {
  if (session == (tty_session_t *)0) {
    return -1;
  }
  return path_state_pwd_ctx(&session->path_context, out, out_len);
}

uint32_t tty_session_input_length(const tty_session_t *session) {
  if (session == (const tty_session_t *)0) {
    return 0u;
  }
  return session->input_len;
}

uint32_t tty_session_history_count(const tty_session_t *session) {
  if (session == (const tty_session_t *)0) {
    return 0u;
  }
  return session->history_count;
}

uint32_t tty_session_command_count(const tty_session_t *session) {
  if (session == (const tty_session_t *)0) {
    return 0u;
  }
  return session->command_count;
}
