#include <stdint.h>

#include "uart.h"

enum {
  UART_BASE = 0x10000000u,
  UART_RBR = 0x00,
  UART_THR = 0x00,
  UART_IER = 0x01,
  UART_FCR = 0x02,
  UART_LCR = 0x03,
  UART_LSR = 0x05,
};

enum {
  LSR_DATA_READY = 1u << 0,
  LSR_TX_EMPTY = 1u << 5,
};

static inline void uart_reg_write(uint32_t offset, uint8_t value) {
  volatile uint8_t *reg = (volatile uint8_t *)(uintptr_t)(UART_BASE + offset);
  *reg = value;
}

static inline uint8_t uart_reg_read(uint32_t offset) {
  volatile uint8_t *reg = (volatile uint8_t *)(uintptr_t)(UART_BASE + offset);
  return *reg;
}

void uart_init(void) {
  uart_reg_write(UART_IER, 0x00);
  uart_reg_write(UART_LCR, 0x80);
  uart_reg_write(UART_RBR, 0x03);
  uart_reg_write(UART_IER, 0x00);
  uart_reg_write(UART_LCR, 0x03);
  uart_reg_write(UART_FCR, 0x01);
}

bool uart_can_read(void) {
  return (uart_reg_read(UART_LSR) & LSR_DATA_READY) != 0u;
}

void uart_write_byte(uint8_t byte) {
  while ((uart_reg_read(UART_LSR) & LSR_TX_EMPTY) == 0u) {
  }
  uart_reg_write(UART_THR, byte);
}

void uart_write(const char *s) {
  while (*s != '\0') {
    uart_write_byte((uint8_t)*s++);
  }
}

int uart_read_byte_nonblocking(void) {
  if (!uart_can_read()) {
    return -1;
  }

  return (int)uart_reg_read(UART_RBR);
}

uint8_t uart_read_byte_blocking(void) {
  while (!uart_can_read()) {
  }

  return uart_reg_read(UART_RBR);
}

void uart_putc(char c) {
  uart_write_byte((uint8_t)c);
}

void uart_puts(const char *s) {
  uart_write(s);
}
