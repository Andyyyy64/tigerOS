#include <stdint.h>

#include "clock.h"
#include "console.h"
#include "timer.h"

enum {
  SSTATUS_SIE = 1ULL << 1,
  SIE_STIE = 1ULL << 5,
  CLOCK_TICK_INTERVAL = 100000ULL,
  CLOCK_TICK_LOG_LIMIT = 4ULL,
};

static volatile uint64_t g_tick_count;

static inline void csr_set_sstatus(uint64_t bits) {
  __asm__ volatile("csrs sstatus, %0" : : "r"(bits));
}

static inline void csr_set_sie(uint64_t bits) {
  __asm__ volatile("csrs sie, %0" : : "r"(bits));
}

static void clock_program_next_event(void) {
  uint64_t next_deadline = timer_now() + CLOCK_TICK_INTERVAL;
  timer_set_deadline(next_deadline);
}

void clock_init(void) {
  g_tick_count = 0;
  clock_program_next_event();
  csr_set_sie(SIE_STIE);
  csr_set_sstatus(SSTATUS_SIE);
}

void clock_handle_timer_interrupt(void) {
  g_tick_count++;
  if (g_tick_count <= CLOCK_TICK_LOG_LIMIT) {
    console_write("TIMER: tick\n");
    if (g_tick_count == CLOCK_TICK_LOG_LIMIT) {
      console_write("TIMER: tick log capped\n");
    }
  }
  clock_program_next_event();
}

uint64_t clock_get_ticks(void) {
  return g_tick_count;
}
