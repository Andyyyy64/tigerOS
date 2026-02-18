#include <stdint.h>
#include <stddef.h>

#include "task.h"

static void task_zero_trap_frame(struct trap_frame *frame) {
  uint64_t *words;
  size_t i;
  size_t count;

  if (frame == 0) {
    return;
  }

  words = (uint64_t *)frame;
  count = sizeof(*frame) / sizeof(uint64_t);
  for (i = 0; i < count; ++i) {
    words[i] = 0ULL;
  }
}

void task_init(kernel_task_t *task,
               uint32_t id,
               const char *name,
               task_entry_t entry,
               uint8_t *stack_base,
               uint64_t stack_size) {
  uintptr_t top;

  if (task == 0) {
    return;
  }

  task->id = id;
  task->name = name;
  task->state = TASK_STATE_UNUSED;
  task->entry = entry;
  task->stack_base = stack_base;
  task->stack_size = stack_size;
  task->run_count = 0ULL;
  task_zero_trap_frame(&task->context);

  if (stack_base != 0 && stack_size != 0ULL) {
    top = (uintptr_t)(stack_base + stack_size);
    top &= ~(uintptr_t)0xFULL;
    task->context.sp = (uint64_t)top;
  }
  task->context.mepc = (uint64_t)(uintptr_t)entry;
}

void task_mark_runnable(kernel_task_t *task) {
  if (task == 0) {
    return;
  }
  task->state = TASK_STATE_RUNNABLE;
}

void task_mark_running(kernel_task_t *task) {
  if (task == 0) {
    return;
  }
  task->state = TASK_STATE_RUNNING;
}

void task_context_save(kernel_task_t *task, const struct trap_frame *frame) {
  if (task == 0 || frame == 0) {
    return;
  }

  task->context = *frame;
}

void task_context_restore(const kernel_task_t *task, struct trap_frame *frame) {
  if (task == 0 || frame == 0) {
    return;
  }

  *frame = task->context;
}
