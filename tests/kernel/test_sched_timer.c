#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "clock.h"
#include "line_io.h"
#include "riscv_timer.h"
#include "sched.h"
#include "task.h"
#include "trap.h"

#define TEST_ASSERT(cond, msg)                       \
  do {                                               \
    if (!(cond)) {                                   \
      fprintf(stderr, "FAIL: %s\n", (msg));       \
      return 1;                                      \
    }                                                \
  } while (0)

enum {
  TEST_LOG_CAPACITY = 32768,
  TEST_DEADLINE_CAPACITY = 64,
  CLOCK_INTERVAL_TICKS = 1000000ULL,
};

static char g_log[TEST_LOG_CAPACITY];
static size_t g_log_len;

static uint64_t g_fake_now;
static uint64_t g_deadlines[TEST_DEADLINE_CAPACITY];
static size_t g_deadline_count;
static unsigned int g_interrupt_enable_count;

static void test_log_reset(void) {
  g_log_len = 0u;
  g_log[0] = '\0';
}

static void test_log_append_char(char c) {
  if (g_log_len + 1u >= TEST_LOG_CAPACITY) {
    return;
  }

  g_log[g_log_len++] = c;
  g_log[g_log_len] = '\0';
}

static void test_log_append(const char *s) {
  size_t i = 0u;

  if (s == NULL) {
    return;
  }

  while (s[i] != '\0') {
    test_log_append_char(s[i]);
    ++i;
  }
}

static size_t count_substring(const char *haystack, const char *needle) {
  size_t count = 0u;
  size_t needle_len;
  const char *cursor;

  if (haystack == NULL || needle == NULL) {
    return 0u;
  }

  needle_len = strlen(needle);
  if (needle_len == 0u) {
    return 0u;
  }

  cursor = haystack;
  while ((cursor = strstr(cursor, needle)) != NULL) {
    ++count;
    cursor += needle_len;
  }

  return count;
}

static void reset_timer_stubs(void) {
  size_t i = 0u;

  g_fake_now = 0ULL;
  g_deadline_count = 0u;
  g_interrupt_enable_count = 0u;
  for (i = 0u; i < TEST_DEADLINE_CAPACITY; ++i) {
    g_deadlines[i] = 0ULL;
  }
}

void line_io_write(const char *s) { test_log_append(s); }

void console_write(const char *s) { test_log_append(s); }

void console_putc(char c) { test_log_append_char(c); }

uint64_t riscv_timer_now(void) { return g_fake_now; }

uint64_t riscv_timer_read_time(void) { return g_fake_now; }

void riscv_timer_set_deadline(uint64_t deadline) {
  if (g_deadline_count < TEST_DEADLINE_CAPACITY) {
    g_deadlines[g_deadline_count] = deadline;
    g_deadline_count += 1u;
  }
}

void riscv_timer_enable_interrupts(void) { g_interrupt_enable_count += 1u; }

static int test_clock_behavior(void) {
  size_t i;

  test_log_reset();
  reset_timer_stubs();

  g_fake_now = 100ULL;
  clock_init();

  TEST_ASSERT(clock_ticks() == 0ULL, "clock should start at zero ticks");
  TEST_ASSERT(g_interrupt_enable_count == 1u, "clock should enable timer interrupts once");
  TEST_ASSERT(g_deadline_count == 1u, "clock_init should program first deadline");
  TEST_ASSERT(g_deadlines[0] == g_fake_now + CLOCK_INTERVAL_TICKS,
              "initial deadline should be now + interval");

  g_fake_now = g_deadlines[0] - 1ULL;
  clock_handle_timer_interrupt();

  TEST_ASSERT(clock_ticks() == 1ULL, "tick count should increment after timer interrupt");
  TEST_ASSERT(g_deadline_count == 2u, "first tick should schedule next deadline");
  TEST_ASSERT(g_deadlines[1] == g_deadlines[0] + CLOCK_INTERVAL_TICKS,
              "next deadline should advance by one interval");

  g_fake_now = g_deadlines[1] + (2ULL * CLOCK_INTERVAL_TICKS) + 123ULL;
  clock_handle_timer_interrupt();

  TEST_ASSERT(clock_ticks() == 2ULL, "tick count should increment on delayed timer interrupt");
  TEST_ASSERT(g_deadline_count == 3u, "delayed tick should still schedule a deadline");
  TEST_ASSERT(g_deadlines[2] == g_deadlines[1] + (3ULL * CLOCK_INTERVAL_TICKS),
              "deadline should skip missed intervals and land in the future");

  for (i = 0u; i < 4u; ++i) {
    g_fake_now = g_deadlines[g_deadline_count - 1u] + CLOCK_INTERVAL_TICKS;
    clock_handle_timer_interrupt();
  }

  TEST_ASSERT(clock_ticks() == 6ULL, "tick count should match number of handled interrupts");
  TEST_ASSERT(count_substring(g_log, "TICK: periodic interrupt\n") == 4u,
              "tick log output should be capped at four messages");

  return 0;
}

static int test_scheduler_round_robin_timer_flow(void) {
  size_t i;
  task_control_block_t *task_1;
  task_control_block_t *task_2;
  struct trap_frame frame;

  test_log_reset();
  reset_timer_stubs();

  g_fake_now = 5000ULL;
  clock_init();
  sched_bootstrap_test_tasks();

  TEST_ASSERT(sched_runnable_count() == 2u, "scheduler should bootstrap two runnable tasks");

  task_1 = task_find(1u);
  task_2 = task_find(2u);
  TEST_ASSERT(task_1 != NULL, "task 1 should exist after scheduler bootstrap");
  TEST_ASSERT(task_2 != NULL, "task 2 should exist after scheduler bootstrap");

  for (i = 0u; i < 6u; ++i) {
    memset(&frame, 0, sizeof(frame));
    frame.mepc = 0x80000000ULL + (uint64_t)(i * 4u);
    frame.mcause = 0x8000000000000007ULL;

    g_fake_now += CLOCK_INTERVAL_TICKS;
    clock_handle_timer_interrupt();
    sched_handle_timer_interrupt(&frame);
  }

  TEST_ASSERT(clock_ticks() == 6ULL, "timer flow should invoke six clock ticks");

  TEST_ASSERT(task_1->run_count == 3ULL, "task 1 should run on alternating slices");
  TEST_ASSERT(task_2->run_count == 3ULL, "task 2 should run on alternating slices");
  TEST_ASSERT(task_1->context.switches_in == 3ULL, "task 1 switch-in count mismatch");
  TEST_ASSERT(task_2->context.switches_in == 3ULL, "task 2 switch-in count mismatch");
  TEST_ASSERT(task_1->context.switches_out == 3ULL, "task 1 switch-out count mismatch");
  TEST_ASSERT(task_2->context.switches_out == 2ULL, "task 2 switch-out count mismatch");
  TEST_ASSERT(task_1->context.last_mcause == 0x8000000000000007ULL,
              "task 1 should observe timer interrupt cause");
  TEST_ASSERT(task_2->context.last_mcause == 0x8000000000000007ULL,
              "task 2 should observe timer interrupt cause");

  TEST_ASSERT(strstr(g_log, "SCHED: policy=round-robin runnable=2") != NULL,
              "scheduler policy log missing");
  TEST_ASSERT(strstr(g_log, "TASK: 1 running") != NULL, "task 1 run log missing");
  TEST_ASSERT(strstr(g_log, "TASK: 2 running") != NULL, "task 2 run log missing");
  TEST_ASSERT(strstr(g_log, "SCHED_TEST: alternating tasks confirmed") != NULL,
              "alternation confirmation log missing");

  return 0;
}

int main(void) {
  if (test_clock_behavior() != 0) {
    return 1;
  }
  if (test_scheduler_round_robin_timer_flow() != 0) {
    return 1;
  }

  printf("scheduler/timer integration tests passed\n");
  return 0;
}
