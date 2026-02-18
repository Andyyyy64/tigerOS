#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#include "trap.h"

typedef enum task_state {
  TASK_STATE_UNUSED = 0,
  TASK_STATE_RUNNABLE = 1,
  TASK_STATE_RUNNING = 2,
} task_state_t;

struct task_control_block;
typedef void (*task_entry_fn)(struct task_control_block *task);

typedef struct task_context {
  uint64_t switches_in;
  uint64_t switches_out;
  uint64_t last_mepc;
  uint64_t last_mcause;
} task_context_t;

typedef struct task_control_block {
  uint32_t id;
  const char *name;
  task_state_t state;
  uint64_t run_count;
  task_context_t context;
  task_entry_fn entry;
} task_control_block_t;

enum {
  TASK_MAX_TASKS = 8,
};

void task_system_init(void);
task_control_block_t *task_create(const char *name, task_entry_fn entry);
task_control_block_t *task_find(uint32_t task_id);
void task_context_switch_out(task_control_block_t *task, const struct trap_frame *frame);
void task_context_switch_in(task_control_block_t *task, const struct trap_frame *frame);

#endif
