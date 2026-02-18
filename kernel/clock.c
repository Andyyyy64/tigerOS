#include <stdint.h>

#include "clock.h"
#include "console.h"
#include "timer.h"

enum {
  CLOCK_INTERVAL_TICKS = 1000000ULL,
  CLOCK_TICK_LOG_BUDGET = 8U,
};

static volatile uint64_t g_tick_count;
static uint64_t g_next_deadline;
static uint32_t g_tick_log_budget;

void clock_init(void) {
  uint64_t now = riscv_timer_read_time();

  g_tick_count = 0ULL;
  g_tick_log_budget = CLOCK_TICK_LOG_BUDGET;
  g_next_deadline = now + CLOCK_INTERVAL_TICKS;

  riscv_timer_set_deadline(g_next_deadline);
  riscv_timer_enable_interrupts();
}

void clock_handle_timer_interrupt(void) {
  uint64_t now;

  g_tick_count += 1ULL;
  g_next_deadline += CLOCK_INTERVAL_TICKS;
  now = riscv_timer_read_time();
  if ((int64_t)(g_next_deadline - now) <= 0) {
    g_next_deadline = now + CLOCK_INTERVAL_TICKS;
  }
  riscv_timer_set_deadline(g_next_deadline);

  if (g_tick_log_budget > 0U) {
    g_tick_log_budget -= 1U;
    console_write("TICK: timer interrupt\n");
  }
}

uint64_t clock_ticks(void) {
  return g_tick_count;
}
