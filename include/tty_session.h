#ifndef TTY_SESSION_H
#define TTY_SESSION_H

#include <stddef.h>
#include <stdint.h>

#include "keyboard.h"
#include "path_state.h"

enum {
  TTY_SESSION_INPUT_CAPACITY = 128u,
  TTY_SESSION_HISTORY_DEPTH = 8u,
};

typedef struct tty_session {
  uint32_t session_id;
  path_state_context_t path_context;
  char input_buffer[TTY_SESSION_INPUT_CAPACITY];
  uint32_t input_len;
  char history[TTY_SESSION_HISTORY_DEPTH][TTY_SESSION_INPUT_CAPACITY];
  uint32_t history_count;
  uint32_t history_next_index;
  uint32_t command_count;
} tty_session_t;

void tty_session_init(tty_session_t *session, uint32_t session_id);
int tty_session_handle_keyboard_event(tty_session_t *session, const keyboard_event_t *event);
int tty_session_get_cwd(tty_session_t *session, char *out, size_t out_len);
uint32_t tty_session_input_length(const tty_session_t *session);
uint32_t tty_session_history_count(const tty_session_t *session);
uint32_t tty_session_command_count(const tty_session_t *session);

#endif
