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

void uart_putc(char c) {
  while ((uart_reg_read(UART_LSR) & 0x20u) == 0u) {
  }
  uart_reg_write(UART_THR, (uint8_t)c);
}

void uart_puts(const char *s) {
  while (*s != '\0') {
    uart_putc(*s++);
  }
}

void kernel_main(void) {
  uart_init();
  uart_puts("BOOT: kernel entry\n");

  for (;;) {
    __asm__ volatile("wfi");
  }
}
