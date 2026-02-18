#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

void uart_init(void);
void uart_write_byte(uint8_t byte);
void uart_write(const char *s);
int uart_read_byte_nonblocking(void);
uint8_t uart_read_byte_blocking(void);
bool uart_can_read(void);
void uart_putc(char c);
void uart_puts(const char *s);

#endif
