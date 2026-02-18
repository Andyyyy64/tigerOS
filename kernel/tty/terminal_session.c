#include <stddef.h>
#include <stdint.h>

#include "shell_parser.h"
#include "terminal_session.h"

enum {
  TERMINAL_ARGV_CAP = 16u,
  TERMINAL_LS_MAX = 64u,
  TERMINAL_HASH_BASIS = 2166136261u,
};

static int ts_str_eq(const char *a, const char *b) {
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

  return a[i] == '\0' && b[i] == '\0';
}

static int ts_str_copy(char *dst, size_t dst_cap, const char *src) {
  size_t i = 0u;

  if (dst == NULL || src == NULL || dst_cap == 0u) {
    return -1;
  }

  while (src[i] != '\0') {
    if (i + 1u >= dst_cap) {
      return -1;
    }
    dst[i] = src[i];
    ++i;
  }

  dst[i] = '\0';
  return 0;
}

static void ts_hash_byte(terminal_session_t *session, uint8_t byte) {
  if (session == NULL) {
    return;
  }
  session->marker_hash ^= (uint32_t)byte;
  session->marker_hash *= 16777619u;
}

static void ts_hash_text(terminal_session_t *session, const char *text) {
  size_t i = 0u;

  if (session == NULL || text == NULL) {
    return;
  }

  while (text[i] != '\0') {
    ts_hash_byte(session, (uint8_t)text[i]);
    ++i;
  }

  ts_hash_byte(session, 0u);
}

static void ts_hash_u32(terminal_session_t *session, uint32_t value) {
  ts_hash_byte(session, (uint8_t)(value & 0xffu));
  ts_hash_byte(session, (uint8_t)((value >> 8u) & 0xffu));
  ts_hash_byte(session, (uint8_t)((value >> 16u) & 0xffu));
  ts_hash_byte(session, (uint8_t)((value >> 24u) & 0xffu));
}

static void ts_store_history(terminal_session_t *session, const char *line) {
  uint32_t idx = 0u;

  if (session == NULL || line == NULL) {
    return;
  }

  if (session->history_count < TERMINAL_SESSION_HISTORY_CAP) {
    idx = session->history_count;
    session->history_count += 1u;
  } else {
    idx = session->history_head;
    session->history_head += 1u;
    if (session->history_head >= TERMINAL_SESSION_HISTORY_CAP) {
      session->history_head = 0u;
    }
  }

  (void)ts_str_copy(session->history[idx], sizeof(session->history[idx]), line);
}

static void ts_refresh_cwd_cache(terminal_session_t *session) {
  if (session == NULL) {
    return;
  }

  if (path_state_context_pwd(&session->path_state, session->cwd_cache, sizeof(session->cwd_cache)) != 0) {
    (void)ts_str_copy(session->cwd_cache, sizeof(session->cwd_cache), "/");
  }
}

void terminal_session_init(terminal_session_t *session, uint32_t endpoint_id,
                           const wm_window_t *window) {
  uint32_t i;

  if (session == NULL || endpoint_id == 0u) {
    return;
  }

  session->endpoint_id = endpoint_id;
  session->window = window;
  path_state_context_init(&session->path_state);
  session->input_len = 0u;
  session->input_buffer[0] = '\0';
  session->history_count = 0u;
  session->history_head = 0u;
  session->lines_executed = 0u;
  session->marker_hash = TERMINAL_HASH_BASIS;
  session->cwd_cache[0] = '/';
  session->cwd_cache[1] = '\0';

  for (i = 0u; i < TERMINAL_SESSION_HISTORY_CAP; ++i) {
    session->history[i][0] = '\0';
  }

  ts_hash_u32(session, endpoint_id);
  ts_refresh_cwd_cache(session);
}

static void execute_help(terminal_session_t *session) {
  ts_hash_text(session, "help");
  ts_hash_text(session, "echo");
  ts_hash_text(session, "pwd");
  ts_hash_text(session, "cd");
  ts_hash_text(session, "mkdir");
  ts_hash_text(session, "ls");
  ts_hash_text(session, "cat");
}

static void execute_echo(terminal_session_t *session, int argc, char **argv) {
  int i;

  if (session == NULL || argv == NULL) {
    return;
  }

  for (i = 1; i < argc; ++i) {
    if (argv[i] != NULL) {
      ts_hash_text(session, argv[i]);
    }
  }
}

static void execute_pwd(terminal_session_t *session) {
  char cwd[FS_PATH_MAX];

  if (session == NULL) {
    return;
  }

  if (path_state_context_pwd(&session->path_state, cwd, sizeof(cwd)) == 0) {
    ts_hash_text(session, cwd);
  } else {
    ts_hash_text(session, "pwd:error");
  }
}

static void execute_cd(terminal_session_t *session, int argc, char **argv) {
  const char *target = "/";

  if (session == NULL || argv == NULL) {
    return;
  }

  if (argc >= 2 && argv[1] != NULL) {
    target = argv[1];
  }

  if (path_state_context_cd(&session->path_state, target) != 0) {
    ts_hash_text(session, "cd:error");
  } else {
    ts_hash_text(session, "cd:ok");
  }
}

static void execute_mkdir(terminal_session_t *session, int argc, char **argv) {
  int i;

  if (session == NULL || argv == NULL) {
    return;
  }

  if (argc < 2) {
    ts_hash_text(session, "mkdir:missing");
    return;
  }

  for (i = 1; i < argc; ++i) {
    if (argv[i] == NULL) {
      continue;
    }

    ts_hash_text(session, argv[i]);
    if (path_state_context_mkdir(&session->path_state, argv[i]) != 0) {
      ts_hash_text(session, "mkdir:error");
    } else {
      ts_hash_text(session, "mkdir:ok");
    }
  }
}

static void execute_ls(terminal_session_t *session, int argc, char **argv) {
  path_state_entry_t entries[TERMINAL_LS_MAX];
  size_t count = 0u;
  size_t i;
  const char *target = ".";

  if (session == NULL || argv == NULL) {
    return;
  }

  if (argc >= 2 && argv[1] != NULL) {
    target = argv[1];
  }

  if (path_state_context_ls(&session->path_state, target, entries, TERMINAL_LS_MAX, &count) != 0) {
    ts_hash_text(session, "ls:error");
    return;
  }

  ts_hash_u32(session, (uint32_t)count);
  for (i = 0u; i < count; ++i) {
    ts_hash_text(session, entries[i].name);
    ts_hash_byte(session, (entries[i].kind == PATH_STATE_ENTRY_DIR) ? (uint8_t)'/' : (uint8_t)'f');
  }
}

static void execute_cat(terminal_session_t *session, int argc, char **argv) {
  int i;

  if (session == NULL || argv == NULL) {
    return;
  }

  if (argc < 2) {
    ts_hash_text(session, "cat:missing");
    return;
  }

  for (i = 1; i < argc; ++i) {
    const char *content = NULL;

    if (argv[i] == NULL) {
      continue;
    }

    if (path_state_context_cat(&session->path_state, argv[i], &content) != 0) {
      ts_hash_text(session, "cat:error");
      ts_hash_text(session, argv[i]);
      continue;
    }

    ts_hash_text(session, content);
  }
}

int terminal_session_execute_line(terminal_session_t *session, const char *line) {
  char parse_line[TERMINAL_SESSION_INPUT_CAP];
  char raw_line[TERMINAL_SESSION_INPUT_CAP];
  char *argv[TERMINAL_ARGV_CAP];
  int argc;

  if (session == NULL || line == NULL) {
    return -1;
  }

  if (ts_str_copy(parse_line, sizeof(parse_line), line) != 0 ||
      ts_str_copy(raw_line, sizeof(raw_line), line) != 0) {
    return -1;
  }

  argc = shell_parse_line(parse_line, argv, TERMINAL_ARGV_CAP);
  if (argc <= 0) {
    return 0;
  }

  ts_store_history(session, raw_line);
  session->lines_executed += 1u;
  ts_hash_text(session, raw_line);

  if (ts_str_eq(argv[0], "help")) {
    execute_help(session);
  } else if (ts_str_eq(argv[0], "echo")) {
    execute_echo(session, argc, argv);
  } else if (ts_str_eq(argv[0], "pwd")) {
    execute_pwd(session);
  } else if (ts_str_eq(argv[0], "cd")) {
    execute_cd(session, argc, argv);
  } else if (ts_str_eq(argv[0], "mkdir")) {
    execute_mkdir(session, argc, argv);
  } else if (ts_str_eq(argv[0], "ls")) {
    execute_ls(session, argc, argv);
  } else if (ts_str_eq(argv[0], "cat")) {
    execute_cat(session, argc, argv);
  } else {
    ts_hash_text(session, "unknown");
  }

  ts_refresh_cwd_cache(session);
  return 0;
}

int terminal_session_handle_event(terminal_session_t *session, const keyboard_event_t *event) {
  if (session == NULL || event == NULL) {
    return -1;
  }

  if (event->type == KEYBOARD_EVENT_TEXT) {
    if (event->text < 0x20 || event->text > 0x7e) {
      return 0;
    }

    if (session->input_len + 1u >= TERMINAL_SESSION_INPUT_CAP) {
      return -1;
    }

    session->input_buffer[session->input_len] = event->text;
    session->input_len += 1u;
    session->input_buffer[session->input_len] = '\0';
    return 0;
  }

  if (event->type == KEYBOARD_EVENT_CONTROL) {
    if (event->control == KEYBOARD_CONTROL_BACKSPACE) {
      if (session->input_len > 0u) {
        session->input_len -= 1u;
        session->input_buffer[session->input_len] = '\0';
      }
      return 0;
    }

    if (event->control == KEYBOARD_CONTROL_ENTER) {
      int status = terminal_session_execute_line(session, session->input_buffer);
      session->input_len = 0u;
      session->input_buffer[0] = '\0';
      return status;
    }
  }

  return 0;
}

uint32_t terminal_session_endpoint_id(const terminal_session_t *session) {
  if (session == NULL) {
    return 0u;
  }
  return session->endpoint_id;
}

const wm_window_t *terminal_session_window(const terminal_session_t *session) {
  if (session == NULL) {
    return (const wm_window_t *)0;
  }
  return session->window;
}

uint32_t terminal_session_input_len(const terminal_session_t *session) {
  if (session == NULL) {
    return 0u;
  }
  return session->input_len;
}

uint32_t terminal_session_history_count(const terminal_session_t *session) {
  if (session == NULL) {
    return 0u;
  }
  return session->history_count;
}

uint32_t terminal_session_lines_executed(const terminal_session_t *session) {
  if (session == NULL) {
    return 0u;
  }
  return session->lines_executed;
}

uint32_t terminal_session_marker(const terminal_session_t *session) {
  if (session == NULL) {
    return 0u;
  }
  return session->marker_hash;
}

const char *terminal_session_cwd(const terminal_session_t *session) {
  if (session == NULL) {
    return "";
  }
  return session->cwd_cache;
}
