#include <stdbool.h>
#include <stdint.h>

#include "console.h"
#include "line_io.h"
#include "trap.h"

extern void trap_vector(void);

static volatile bool trap_test_armed;
static volatile bool trap_test_passed;

static uint64_t trap_instruction_length(uint64_t epc) {
  const volatile uint16_t *instr = (const volatile uint16_t *)(uintptr_t)epc;
  return ((*instr & 0x3u) == 0x3u) ? 4u : 2u;
}

static void trap_put_hex64(uint64_t value) {
  static const char digits[] = "0123456789abcdef";
  int shift;

  for (shift = 60; shift >= 0; shift -= 4) {
    console_putc(digits[(value >> (uint32_t)shift) & 0x0fu]);
  }
}

static void trap_log_mcause_mepc(const char *prefix, uint64_t mcause, uint64_t mepc) {
  line_io_write(prefix);
  line_io_write(" mcause=0x");
  trap_put_hex64(mcause);
  line_io_write(" mepc=0x");
  trap_put_hex64(mepc);
  line_io_write("\n");
}

void trap_init(void) {
  uintptr_t vector_addr = (uintptr_t)&trap_vector;

  __asm__ volatile("csrw stvec, %0" : : "r"(vector_addr));
}

void trap_trigger_self_test(void) {
  trap_test_armed = true;
  line_io_write("TRAP_TEST: trigger ebreak\n");
  __asm__ volatile("ebreak");

  if (trap_test_passed) {
    line_io_write("TRAP_TEST: handled\n");
    return;
  }

  line_io_write("TRAP_TEST: failed\n");
  for (;;) {
    __asm__ volatile("wfi");
  }
}

void trap_dispatch(struct riscv_trap_frame *frame) {
  bool is_interrupt = (frame->mcause & RISCV_MCAUSE_INTERRUPT_BIT) != 0u;
  uint64_t cause_code = frame->mcause & RISCV_MCAUSE_CODE_MASK;

  if (!is_interrupt && trap_test_armed && cause_code == RISCV_CAUSE_BREAKPOINT) {
    trap_test_armed = false;
    trap_test_passed = true;
    trap_log_mcause_mepc("TRAP_TEST:", frame->mcause, frame->mepc);
    frame->mepc += trap_instruction_length(frame->mepc);
    return;
  }

  line_io_write("TRAP: unexpected");
  line_io_write(is_interrupt ? " interrupt" : " exception");
  line_io_write(" mcause=0x");
  trap_put_hex64(frame->mcause);
  line_io_write(" mepc=0x");
  trap_put_hex64(frame->mepc);
  line_io_write(" mtval=0x");
  trap_put_hex64(frame->mtval);
  line_io_write("\n");

  for (;;) {
    __asm__ volatile("wfi");
  }
}
