#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

void console_init(void);
void console_putc(char c);
void console_write(const char *s);
int console_getc_nonblocking(void);
uint8_t console_getc_blocking(void);

#endif
