#ifndef LINE_IO_H
#define LINE_IO_H

#include <stdbool.h>

int line_io_readline(char *out, unsigned int out_len, bool blocking);
void line_io_write(const char *s);

#endif
