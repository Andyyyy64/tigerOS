#include <stdint.h>

#include "console.h"
#include "uart.h"

void console_init(void) {
  uart_init();
}

void console_putc(char c) {
  if (c == '\n') {
    uart_write_byte((uint8_t)'\r');
  }
  uart_write_byte((uint8_t)c);
}

void console_write(const char *s) {
  while (*s != '\0') {
    console_putc(*s++);
  }
}

int console_getc_nonblocking(void) {
  return uart_read_byte_nonblocking();
}

uint8_t console_getc_blocking(void) {
  return uart_read_byte_blocking();
}
