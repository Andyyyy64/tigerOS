#include <stdbool.h>
#include <stdint.h>

#include "console.h"
#include "trap.h"

extern void trap_entry(void);

enum {
  TRAP_INTERRUPT_BIT = 1ull << 63,
  TRAP_CAUSE_BREAKPOINT = 3u,
  TRAP_INSN_SIZE = 4u,
};

static char trap_hex_digit(uint8_t value) {
  if (value < 10u) {
    return (char)('0' + value);
  }

  return (char)('a' + (value - 10u));
}

static void trap_write_hex64(uint64_t value) {
  char out[17];
  unsigned int i;

  for (i = 0u; i < 16u; ++i) {
    uint8_t nibble = (uint8_t)((value >> ((15u - i) * 4u)) & 0x0fu);
    out[i] = trap_hex_digit(nibble);
  }
  out[16] = '\0';
  console_write(out);
}

static void trap_log_test_markers(const struct trap_frame *frame) {
  console_write("TRAP_TEST: mcause=0x");
  trap_write_hex64(frame->cause);
  console_write("\n");

  console_write("TRAP_TEST: mepc=0x");
  trap_write_hex64(frame->epc);
  console_write("\n");
}

static void trap_log_unexpected(const struct trap_frame *frame) {
  console_write("TRAP_UNEXPECTED: mcause=0x");
  trap_write_hex64(frame->cause);
  console_write(" mepc=0x");
  trap_write_hex64(frame->epc);
  console_write(" mtval=0x");
  trap_write_hex64(frame->tval);
  console_write("\n");
}

void trap_init(void) {
  uintptr_t vector = (uintptr_t)&trap_entry;

  __asm__ volatile("csrw stvec, %0" : : "r"(vector));
}

void trap_trigger_test(void) {
  __asm__ volatile(".4byte 0x00100073");
}

void trap_handle(struct trap_frame *frame) {
  uint64_t raw_cause;
  uint64_t cause_code;
  bool is_interrupt;

  if (frame == 0) {
    for (;;) {
      __asm__ volatile("wfi");
    }
  }

  raw_cause = frame->cause;
  is_interrupt = (raw_cause & TRAP_INTERRUPT_BIT) != 0u;
  cause_code = raw_cause & ~TRAP_INTERRUPT_BIT;

  if (!is_interrupt && cause_code == TRAP_CAUSE_BREAKPOINT) {
    trap_log_test_markers(frame);
    frame->epc += TRAP_INSN_SIZE;
    return;
  }

  trap_log_unexpected(frame);
  for (;;) {
    __asm__ volatile("wfi");
  }
}
