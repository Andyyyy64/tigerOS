#include <stdint.h>

#include "task.h"

static task_control_block_t g_tasks[TASK_MAX_TASKS];

static void task_reset(task_control_block_t *task, uint32_t id) {
  if (task == (task_control_block_t *)0) {
    return;
  }

  task->id = id;
  task->name = (const char *)0;
  task->state = TASK_STATE_UNUSED;
  task->run_count = 0ULL;
  task->context.switches_in = 0ULL;
  task->context.switches_out = 0ULL;
  task->context.last_mepc = 0ULL;
  task->context.last_mcause = 0ULL;
  task->entry = (task_entry_fn)0;
}

void task_system_init(void) {
  uint32_t i;

  for (i = 0u; i < TASK_MAX_TASKS; ++i) {
    task_reset(&g_tasks[i], i + 1u);
  }
}

task_control_block_t *task_create(const char *name, task_entry_fn entry) {
  uint32_t i;

  if (entry == (task_entry_fn)0) {
    return (task_control_block_t *)0;
  }

  for (i = 0u; i < TASK_MAX_TASKS; ++i) {
    if (g_tasks[i].state != TASK_STATE_UNUSED) {
      continue;
    }

    g_tasks[i].name = name;
    g_tasks[i].state = TASK_STATE_RUNNABLE;
    g_tasks[i].run_count = 0ULL;
    g_tasks[i].context.switches_in = 0ULL;
    g_tasks[i].context.switches_out = 0ULL;
    g_tasks[i].context.last_mepc = 0ULL;
    g_tasks[i].context.last_mcause = 0ULL;
    g_tasks[i].entry = entry;
    return &g_tasks[i];
  }

  return (task_control_block_t *)0;
}

task_control_block_t *task_find(uint32_t task_id) {
  uint32_t i;

  for (i = 0u; i < TASK_MAX_TASKS; ++i) {
    if (g_tasks[i].id == task_id && g_tasks[i].state != TASK_STATE_UNUSED) {
      return &g_tasks[i];
    }
  }

  return (task_control_block_t *)0;
}

void task_context_switch_out(task_control_block_t *task, const struct trap_frame *frame) {
  if (task == (task_control_block_t *)0) {
    return;
  }

  task->context.switches_out += 1ULL;
  if (frame != (const struct trap_frame *)0) {
    task->context.last_mepc = frame->mepc;
    task->context.last_mcause = frame->mcause;
  }

  if (task->state == TASK_STATE_RUNNING) {
    task->state = TASK_STATE_RUNNABLE;
  }
}

void task_context_switch_in(task_control_block_t *task, const struct trap_frame *frame) {
  if (task == (task_control_block_t *)0) {
    return;
  }

  task->context.switches_in += 1ULL;
  if (frame != (const struct trap_frame *)0) {
    task->context.last_mepc = frame->mepc;
    task->context.last_mcause = frame->mcause;
  }

  task->state = TASK_STATE_RUNNING;
}
