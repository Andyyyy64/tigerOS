#ifndef PAGE_ALLOC_H
#define PAGE_ALLOC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
  PAGE_ALLOC_PAGE_SIZE = 4096u,
};

void page_alloc_init(uintptr_t range_start, uintptr_t range_end);
void *page_alloc(void);
bool page_free(void *page);
bool page_alloc_owns(const void *page);

uintptr_t page_alloc_range_start(void);
uintptr_t page_alloc_range_end(void);
size_t page_alloc_total_pages(void);
size_t page_alloc_free_pages(void);

#endif
