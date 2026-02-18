#include <stdint.h>

#include "mm_init.h"
#include "page_alloc.h"

enum {
  KERNEL_PHYS_MEM_END = 0x88000000u,
};

extern char __bss_end[];

void mm_init(void) {
  uintptr_t alloc_start = (uintptr_t)__bss_end;
  page_alloc_init(alloc_start, (uintptr_t)KERNEL_PHYS_MEM_END);
}
