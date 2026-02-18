#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#include "trap.h"

typedef enum task_state {
  TASK_STATE_UNUSED = 0,
  TASK_STATE_RUNNABLE = 1,
  TASK_STATE_RUNNING = 2,
} task_state_t;

struct kernel_task;
typedef void (*task_entry_t)(struct kernel_task *task);

typedef struct kernel_task {
  uint32_t id;
  const char *name;
  task_state_t state;
  struct trap_frame context;
  task_entry_t entry;
  uint8_t *stack_base;
  uint64_t stack_size;
  uint64_t run_count;
} kernel_task_t;

void task_init(kernel_task_t *task,
               uint32_t id,
               const char *name,
               task_entry_t entry,
               uint8_t *stack_base,
               uint64_t stack_size);
void task_mark_runnable(kernel_task_t *task);
void task_mark_running(kernel_task_t *task);
void task_context_save(kernel_task_t *task, const struct trap_frame *frame);
void task_context_restore(const kernel_task_t *task, struct trap_frame *frame);

#endif
