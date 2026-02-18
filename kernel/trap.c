#include <stdbool.h>
#include <stdint.h>

#include "clock.h"
#include "console.h"
#include "trap.h"

enum {
  MCAUSE_INTERRUPT_BIT = 1ULL << 63,
  MCAUSE_CODE_MASK = MCAUSE_INTERRUPT_BIT - 1ULL,
  MCAUSE_INTERRUPT_SUPERVISOR_TIMER = 5ULL,
  MCAUSE_INTERRUPT_MACHINE_TIMER = 7ULL,
  MCAUSE_EXCEPTION_BREAKPOINT = 3ULL,
};

extern void trap_vector(void);

static volatile bool trap_test_armed;
static volatile bool trap_test_passed;

static inline void csr_write_stvec(uint64_t value) {
  __asm__ volatile("csrw stvec, %0" : : "r"(value));
}

static inline bool trap_is_interrupt(uint64_t cause) {
  return (cause & MCAUSE_INTERRUPT_BIT) != 0ULL;
}

static inline uint64_t trap_cause_code(uint64_t cause) {
  return cause & MCAUSE_CODE_MASK;
}

static void console_write_hex_u64(uint64_t value) {
  static const char k_hex_digits[] = "0123456789abcdef";
  char out[18];
  int i;

  out[0] = '0';
  out[1] = 'x';
  for (i = 0; i < 16; ++i) {
    unsigned int shift = (unsigned int)((15 - i) * 4);
    out[2 + i] = k_hex_digits[(value >> shift) & 0xFU];
  }

  for (i = 0; i < (int)sizeof(out); ++i) {
    console_putc(out[i]);
  }
}

static uint64_t trap_instruction_len(uint64_t pc) {
  uint16_t insn = *(const volatile uint16_t *)(uintptr_t)pc;
  return ((insn & 0x3U) == 0x3U) ? 4ULL : 2ULL;
}

static void trap_halt(void) {
  for (;;) {
    __asm__ volatile("wfi");
  }
}

static bool trap_dispatch_exception(struct trap_frame *frame, uint64_t code) {
  switch (code) {
    case MCAUSE_EXCEPTION_BREAKPOINT:
      if (!trap_test_armed) {
        return false;
      }
      trap_test_armed = false;
      trap_test_passed = true;
      console_write("TRAP_TEST: mcause=");
      console_write_hex_u64(frame->mcause);
      console_write(" mepc=");
      console_write_hex_u64(frame->mepc);
      console_write("\n");
      frame->mepc += trap_instruction_len(frame->mepc);
      return true;
    default:
      return false;
  }
}

static bool trap_dispatch_interrupt(uint64_t code) {
  switch (code) {
    case MCAUSE_INTERRUPT_SUPERVISOR_TIMER:
    case MCAUSE_INTERRUPT_MACHINE_TIMER:
      clock_handle_timer_interrupt();
      return true;
    default:
      return false;
  }
}

void trap_init(void) {
  uintptr_t stvec_base = (uintptr_t)&trap_vector;
  /* Force direct mode (MODE=0) to avoid accidental low-bit mode selection. */
  stvec_base &= ~(uintptr_t)0x3U;
  csr_write_stvec((uint64_t)stvec_base);
}

void trap_test_trigger(void) {
  trap_test_armed = true;
  trap_test_passed = false;

  console_write("TRAP_TEST: trigger\n");
  __asm__ volatile("ebreak");

  if (trap_test_passed) {
    console_write("TRAP_TEST: handled\n");
    return;
  }

  console_write("TRAP_TEST: failed\n");
  trap_halt();
}

void trap_handle(struct trap_frame *frame) {
  uint64_t cause = frame->mcause;
  uint64_t code = trap_cause_code(cause);
  bool is_interrupt = trap_is_interrupt(cause);

  if (is_interrupt && trap_dispatch_interrupt(code)) {
    return;
  }

  if (!is_interrupt && trap_dispatch_exception(frame, code)) {
    return;
  }

  console_write("TRAP: unexpected mcause=");
  console_write_hex_u64(frame->mcause);
  console_write(" mepc=");
  console_write_hex_u64(frame->mepc);
  console_write(" mtval=");
  console_write_hex_u64(frame->mtval);
  console_write("\n");
  trap_halt();
}
