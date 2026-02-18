#include <stdint.h>

#include "clock.h"
#include "console.h"
#include "riscv_timer.h"

enum {
  CLOCK_INTERVAL_TICKS = 1000000ULL,
  CLOCK_LOG_LIMIT = 4U,
  SIE_STIE = 1ULL << 5,
  SSTATUS_SIE = 1ULL << 1,
};

static volatile uint64_t g_tick_count;
static uint64_t g_next_deadline;
static uint32_t g_tick_log_count;

static inline void csr_set_sie(uint64_t mask) {
  __asm__ volatile("csrs sie, %0" : : "r"(mask));
}

static inline void csr_set_sstatus(uint64_t mask) {
  __asm__ volatile("csrs sstatus, %0" : : "r"(mask));
}

static void clock_program_deadline(void) {
  uint64_t now = riscv_timer_now();

  if (g_next_deadline <= now) {
    uint64_t missed = ((now - g_next_deadline) / CLOCK_INTERVAL_TICKS) + 1ULL;
    g_next_deadline += missed * CLOCK_INTERVAL_TICKS;
  }

  riscv_timer_set_deadline(g_next_deadline);
}

void clock_init(void) {
  g_tick_count = 0ULL;
  g_tick_log_count = 0U;
  g_next_deadline = riscv_timer_now() + CLOCK_INTERVAL_TICKS;

  riscv_timer_set_deadline(g_next_deadline);
  csr_set_sie(SIE_STIE);
  csr_set_sstatus(SSTATUS_SIE);
}

void clock_handle_timer_interrupt(void) {
  g_tick_count++;
  g_next_deadline += CLOCK_INTERVAL_TICKS;
  clock_program_deadline();

  if (g_tick_log_count < CLOCK_LOG_LIMIT) {
    console_write("TICK: periodic interrupt\n");
    g_tick_log_count++;
  }
}

uint64_t clock_ticks(void) {
  return g_tick_count;
}
