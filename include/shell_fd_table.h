#ifndef SHELL_FD_TABLE_H
#define SHELL_FD_TABLE_H

#include <stddef.h>

void shell_fd_reset(void);
void shell_fd_set_stdout_console(void);
void shell_fd_set_stdout_capture(void);
void shell_fd_set_stdin(const char *input);
const char *shell_fd_stdin(void);
int shell_fd_has_stdin(void);

void shell_fd_write(const char *s);
void shell_fd_write_n(const char *s, size_t len);
void shell_fd_putc(char ch);

const char *shell_fd_capture_data(void);
size_t shell_fd_capture_len(void);

#endif
