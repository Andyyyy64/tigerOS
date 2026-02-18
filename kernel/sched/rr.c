#include <stdbool.h>
#include <stdint.h>

#include "console.h"
#include "line_io.h"
#include "sched.h"
#include "task.h"

enum {
  SCHED_SWITCH_LOG_LIMIT = 12u,
  SCHED_TASK_LOG_LIMIT = 4u,
  SCHED_ALT_SWITCH_TARGET = 4u,
  SCHED_NO_SLOT = 0xffffffffu,
};

static uint32_t g_runnable_queue[TASK_MAX_TASKS];
static uint32_t g_runnable_count;
static uint32_t g_current_slot;
static uint32_t g_switch_log_count;
static uint32_t g_alternating_switches;
static bool g_alternation_reported;
static bool g_scheduler_running;
static bool g_bootstrapped;

static void sched_write_u32(uint32_t value) {
  char digits[10];
  uint32_t i = 0u;

  if (value == 0u) {
    console_putc('0');
    return;
  }

  while (value > 0u && i < sizeof(digits)) {
    digits[i++] = (char)('0' + (value % 10u));
    value /= 10u;
  }

  while (i > 0u) {
    i -= 1u;
    console_putc(digits[i]);
  }
}

static void sched_log_switch(uint32_t from_id, uint32_t to_id) {
  line_io_write("SCHED: switch ");
  sched_write_u32(from_id);
  line_io_write(" -> ");
  sched_write_u32(to_id);
  line_io_write("\n");
}

static task_control_block_t *sched_task_from_slot(uint32_t slot) {
  if (slot >= g_runnable_count) {
    return (task_control_block_t *)0;
  }

  return task_find(g_runnable_queue[slot]);
}

static uint32_t sched_find_next_slot(void) {
  uint32_t offset;
  uint32_t start_slot;

  if (g_runnable_count == 0u) {
    return SCHED_NO_SLOT;
  }

  if (g_current_slot == SCHED_NO_SLOT) {
    start_slot = 0u;
  } else {
    start_slot = g_current_slot + 1u;
    if (start_slot >= g_runnable_count) {
      start_slot = 0u;
    }
  }

  for (offset = 0u; offset < g_runnable_count; ++offset) {
    uint32_t slot = start_slot + offset;
    task_control_block_t *task;

    if (slot >= g_runnable_count) {
      slot -= g_runnable_count;
    }

    task = sched_task_from_slot(slot);
    if (task != (task_control_block_t *)0 && task->state == TASK_STATE_RUNNABLE) {
      return slot;
    }
  }

  return SCHED_NO_SLOT;
}

static void sched_test_task_1(task_control_block_t *task) {
  if (task == (task_control_block_t *)0) {
    return;
  }

  if (task->run_count <= SCHED_TASK_LOG_LIMIT) {
    line_io_write("TASK: 1 running\n");
  }
}

static void sched_test_task_2(task_control_block_t *task) {
  if (task == (task_control_block_t *)0) {
    return;
  }

  if (task->run_count <= SCHED_TASK_LOG_LIMIT) {
    line_io_write("TASK: 2 running\n");
  }
}

static int sched_enqueue_task(task_control_block_t *task) {
  if (task == (task_control_block_t *)0 || g_runnable_count >= TASK_MAX_TASKS) {
    return -1;
  }

  g_runnable_queue[g_runnable_count++] = task->id;
  return 0;
}

void sched_init(void) {
  uint32_t i;

  task_system_init();

  for (i = 0u; i < TASK_MAX_TASKS; ++i) {
    g_runnable_queue[i] = 0u;
  }

  g_runnable_count = 0u;
  g_current_slot = SCHED_NO_SLOT;
  g_switch_log_count = 0u;
  g_alternating_switches = 0u;
  g_alternation_reported = false;
  g_scheduler_running = false;
  g_bootstrapped = false;
}

void sched_bootstrap_test_tasks(void) {
  task_control_block_t *task_1;
  task_control_block_t *task_2;

  if (g_bootstrapped) {
    return;
  }

  sched_init();

  task_1 = task_create("task-1", sched_test_task_1);
  task_2 = task_create("task-2", sched_test_task_2);
  if (task_1 == (task_control_block_t *)0 || task_2 == (task_control_block_t *)0 ||
      sched_enqueue_task(task_1) != 0 || sched_enqueue_task(task_2) != 0) {
    line_io_write("SCHED: bootstrap failed\n");
    g_bootstrapped = true;
    return;
  }

  g_scheduler_running = true;
  g_bootstrapped = true;

  line_io_write("SCHED: policy=round-robin runnable=2\n");
}

void sched_handle_timer_interrupt(const struct trap_frame *frame) {
  task_control_block_t *prev_task;
  task_control_block_t *next_task;
  uint32_t prev_id = 0u;
  uint32_t next_slot;

  if (!g_scheduler_running || g_runnable_count == 0u) {
    return;
  }

  prev_task = sched_task_from_slot(g_current_slot);
  if (prev_task != (task_control_block_t *)0) {
    prev_id = prev_task->id;
    task_context_switch_out(prev_task, frame);
  }

  next_slot = sched_find_next_slot();
  if (next_slot == SCHED_NO_SLOT) {
    return;
  }

  next_task = sched_task_from_slot(next_slot);
  if (next_task == (task_control_block_t *)0) {
    return;
  }

  g_current_slot = next_slot;
  task_context_switch_in(next_task, frame);

  if (prev_task != (task_control_block_t *)0 && prev_id != next_task->id &&
      g_switch_log_count < SCHED_SWITCH_LOG_LIMIT) {
    sched_log_switch(prev_id, next_task->id);
    g_switch_log_count += 1u;
  }

  if ((prev_id == 1u && next_task->id == 2u) || (prev_id == 2u && next_task->id == 1u)) {
    g_alternating_switches += 1u;
    if (!g_alternation_reported && g_alternating_switches >= SCHED_ALT_SWITCH_TARGET) {
      line_io_write("SCHED_TEST: alternating tasks confirmed\n");
      g_alternation_reported = true;
    }
  }

  next_task->run_count += 1ULL;
  if (next_task->entry != (task_entry_fn)0) {
    next_task->entry(next_task);
  }

  if (next_task->state == TASK_STATE_RUNNING) {
    next_task->state = TASK_STATE_RUNNABLE;
  }
}

uint32_t sched_runnable_count(void) {
  return g_runnable_count;
}
