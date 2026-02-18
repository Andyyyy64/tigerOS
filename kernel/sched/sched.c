#include <stddef.h>
#include <stdint.h>

#include "console.h"
#include "sched.h"
#include "task.h"

enum {
  SCHED_TEST_TASK_COUNT = 2,
  SCHED_TEST_STACK_SIZE = 512,
  SCHED_SWITCH_LOG_LIMIT = 12,
  SCHED_TASK_SLICE_LOG_LIMIT = 4,
};

static kernel_task_t g_tasks[SCHED_TEST_TASK_COUNT];
static uint8_t g_task_stacks[SCHED_TEST_TASK_COUNT][SCHED_TEST_STACK_SIZE];
static kernel_task_t *g_runnable_queue[SCHED_TEST_TASK_COUNT];
static uint32_t g_runnable_count;
static uint32_t g_current_index;
static uint32_t g_switch_log_count;
static int g_scheduler_ready;

static void sched_write_u32(uint32_t value) {
  char digits[10];
  uint32_t tmp = value;
  uint32_t i = 0U;

  if (value == 0U) {
    console_putc('0');
    return;
  }

  while (tmp != 0U && i < (uint32_t)sizeof(digits)) {
    digits[i++] = (char)('0' + (tmp % 10U));
    tmp /= 10U;
  }

  while (i > 0U) {
    i--;
    console_putc(digits[i]);
  }
}

static void sched_write_u64(uint64_t value) {
  char digits[20];
  uint64_t tmp = value;
  uint32_t i = 0U;

  if (value == 0ULL) {
    console_putc('0');
    return;
  }

  while (tmp != 0ULL && i < (uint32_t)sizeof(digits)) {
    digits[i++] = (char)('0' + (tmp % 10ULL));
    tmp /= 10ULL;
  }

  while (i > 0U) {
    i--;
    console_putc(digits[i]);
  }
}

static void sched_test_task_run(kernel_task_t *task) {
  if (task == 0) {
    return;
  }

  task->run_count++;
  if (task->run_count > (uint64_t)SCHED_TASK_SLICE_LOG_LIMIT) {
    return;
  }

  console_write("SCHED_TEST: task");
  sched_write_u32(task->id);
  console_write(" slice ");
  sched_write_u64(task->run_count);
  console_write("\n");
}

static int sched_enqueue(kernel_task_t *task) {
  if (task == 0) {
    return -1;
  }
  if (g_runnable_count >= (uint32_t)SCHED_TEST_TASK_COUNT) {
    return -1;
  }

  task_mark_runnable(task);
  g_runnable_queue[g_runnable_count++] = task;
  return 0;
}

static int sched_select_next_index(void) {
  uint32_t step;
  uint32_t idx;
  kernel_task_t *candidate;

  if (g_runnable_count == 0U) {
    return -1;
  }

  for (step = 1U; step <= g_runnable_count; ++step) {
    idx = (g_current_index + step) % g_runnable_count;
    candidate = g_runnable_queue[idx];
    if (candidate != 0 && candidate->state == TASK_STATE_RUNNABLE) {
      return (int)idx;
    }
  }

  candidate = g_runnable_queue[g_current_index % g_runnable_count];
  if (candidate != 0 && candidate->state == TASK_STATE_RUNNING) {
    return (int)(g_current_index % g_runnable_count);
  }

  return -1;
}

static void sched_log_switch(const kernel_task_t *from, const kernel_task_t *to) {
  if (from == 0 || to == 0) {
    return;
  }
  if (g_switch_log_count >= (uint32_t)SCHED_SWITCH_LOG_LIMIT) {
    return;
  }

  console_write("SCHED_TEST: switch task");
  sched_write_u32(from->id);
  console_write("->task");
  sched_write_u32(to->id);
  console_write("\n");
  g_switch_log_count++;
}

void sched_init(void) {
  uint32_t i;

  g_runnable_count = 0U;
  g_current_index = 0U;
  g_switch_log_count = 0U;
  g_scheduler_ready = 0;

  for (i = 0U; i < (uint32_t)SCHED_TEST_TASK_COUNT; ++i) {
    g_runnable_queue[i] = 0;
    task_init(&g_tasks[i],
              i + 1U,
              (i == 0U) ? "task1" : "task2",
              sched_test_task_run,
              g_task_stacks[i],
              (uint64_t)SCHED_TEST_STACK_SIZE);
    (void)sched_enqueue(&g_tasks[i]);
  }

  if (g_runnable_count >= 2U) {
    g_current_index = g_runnable_count - 1U;
    g_scheduler_ready = 1;
    console_write("SCHED_TEST: initialized 2 runnable tasks\n");
  }
}

void sched_handle_timer_tick(struct trap_frame *frame) {
  int next_index;
  kernel_task_t *current;
  kernel_task_t *next;
  struct trap_frame restore_probe;

  if (!g_scheduler_ready || g_runnable_count < 2U) {
    return;
  }

  current = g_runnable_queue[g_current_index];
  if (current != 0) {
    task_context_save(current, frame);
    if (current->state == TASK_STATE_RUNNING) {
      task_mark_runnable(current);
    }
  }

  next_index = sched_select_next_index();
  if (next_index < 0) {
    return;
  }

  next = g_runnable_queue[(uint32_t)next_index];
  if (next == 0) {
    return;
  }

  task_mark_running(next);
  if (frame != 0) {
    restore_probe = *frame;
    task_context_restore(next, &restore_probe);
  }

  if (current != next) {
    sched_log_switch(current, next);
  }
  g_current_index = (uint32_t)next_index;

  if (next->entry != 0) {
    next->entry(next);
  }
}
