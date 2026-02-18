#include <stdint.h>

#include "framebuffer.h"
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

static void uart_put_hex32(uint32_t value) {
  static const char digits[] = "0123456789ABCDEF";
  int shift;

  for (shift = 28; shift >= 0; shift -= 4) {
    uart_putc(digits[(value >> (uint32_t)shift) & 0x0fu]);
  }
}

void kernel_main(void) {
  uint32_t marker_a;
  uint32_t marker_b;

  uart_init();
  uart_puts("BOOT: kernel entry\n");

  if (framebuffer_init() != 0) {
    uart_puts("GFX: framebuffer init failed\n");
    for (;;) {
      __asm__ volatile("wfi");
    }
  }

  uart_puts("GFX: framebuffer initialized\n");

  marker_a = framebuffer_render_test_pattern();
  marker_b = framebuffer_render_test_pattern();

  if (marker_a == marker_b) {
    uart_puts("GFX: deterministic marker 0x");
    uart_put_hex32(marker_a);
    uart_puts("\n");
  } else {
    uart_puts("GFX: marker mismatch 0x");
    uart_put_hex32(marker_a);
    uart_puts(" != 0x");
    uart_put_hex32(marker_b);
    uart_puts("\n");
  }

  for (;;) {
    __asm__ volatile("wfi");
  }
}
