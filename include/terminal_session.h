#ifndef TERMINAL_SESSION_H
#define TERMINAL_SESSION_H

#include <stdint.h>

#include "fs_path.h"
#include "keyboard.h"
#include "path_state.h"
#include "wm_window.h"

enum {
  TERMINAL_SESSION_INPUT_CAP = 128u,
  TERMINAL_SESSION_HISTORY_CAP = 8u,
};

typedef struct terminal_session {
  uint32_t endpoint_id;
  const wm_window_t *window;
  path_state_context_t path_state;
  char input_buffer[TERMINAL_SESSION_INPUT_CAP];
  uint32_t input_len;
  char history[TERMINAL_SESSION_HISTORY_CAP][TERMINAL_SESSION_INPUT_CAP];
  uint32_t history_count;
  uint32_t history_head;
  uint32_t lines_executed;
  uint32_t marker_hash;
  char cwd_cache[FS_PATH_MAX];
} terminal_session_t;

void terminal_session_init(terminal_session_t *session, uint32_t endpoint_id,
                           const wm_window_t *window);
int terminal_session_handle_event(terminal_session_t *session, const keyboard_event_t *event);
int terminal_session_execute_line(terminal_session_t *session, const char *line);

uint32_t terminal_session_endpoint_id(const terminal_session_t *session);
const wm_window_t *terminal_session_window(const terminal_session_t *session);
uint32_t terminal_session_input_len(const terminal_session_t *session);
uint32_t terminal_session_history_count(const terminal_session_t *session);
uint32_t terminal_session_lines_executed(const terminal_session_t *session);
uint32_t terminal_session_marker(const terminal_session_t *session);
const char *terminal_session_cwd(const terminal_session_t *session);

#endif
