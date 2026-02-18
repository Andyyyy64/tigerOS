#include "page_alloc.h"

enum {
  PAGE_ALLOC_MAX_PAGES = 131072u,
  PAGE_ALLOC_BITMAP_WORD_BITS = 64u,
  PAGE_ALLOC_BITMAP_WORDS =
      (PAGE_ALLOC_MAX_PAGES + PAGE_ALLOC_BITMAP_WORD_BITS - 1u) / PAGE_ALLOC_BITMAP_WORD_BITS,
};

static uintptr_t g_range_start;
static uintptr_t g_range_end;
static size_t g_total_pages;
static size_t g_free_pages;
static size_t g_next_hint;
static uint64_t g_alloc_bitmap[PAGE_ALLOC_BITMAP_WORDS];

static uintptr_t page_align_up(uintptr_t addr) {
  uintptr_t mask = (uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u;
  return (addr + mask) & ~mask;
}

static uintptr_t page_align_down(uintptr_t addr) {
  uintptr_t mask = (uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u;
  return addr & ~mask;
}

static bool bitmap_test(size_t index) {
  size_t word = index / PAGE_ALLOC_BITMAP_WORD_BITS;
  size_t bit = index % PAGE_ALLOC_BITMAP_WORD_BITS;
  uint64_t mask = 1ull << bit;

  return (g_alloc_bitmap[word] & mask) != 0u;
}

static void bitmap_set(size_t index) {
  size_t word = index / PAGE_ALLOC_BITMAP_WORD_BITS;
  size_t bit = index % PAGE_ALLOC_BITMAP_WORD_BITS;
  g_alloc_bitmap[word] |= 1ull << bit;
}

static void bitmap_clear(size_t index) {
  size_t word = index / PAGE_ALLOC_BITMAP_WORD_BITS;
  size_t bit = index % PAGE_ALLOC_BITMAP_WORD_BITS;
  g_alloc_bitmap[word] &= ~(1ull << bit);
}

static bool page_index_from_addr(uintptr_t addr, size_t *index_out) {
  uintptr_t delta;
  size_t index;

  if (addr < g_range_start || addr >= g_range_end) {
    return false;
  }

  delta = addr - g_range_start;
  if ((delta & ((uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u)) != 0u) {
    return false;
  }

  index = (size_t)(delta / (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  if (index >= g_total_pages) {
    return false;
  }

  *index_out = index;
  return true;
}

void page_alloc_init(uintptr_t range_start, uintptr_t range_end) {
  uintptr_t aligned_start = page_align_up(range_start);
  uintptr_t aligned_end = page_align_down(range_end);
  size_t i;

  for (i = 0u; i < PAGE_ALLOC_BITMAP_WORDS; ++i) {
    g_alloc_bitmap[i] = 0u;
  }

  g_range_start = 0u;
  g_range_end = 0u;
  g_total_pages = 0u;
  g_free_pages = 0u;
  g_next_hint = 0u;

  if (aligned_end <= aligned_start) {
    return;
  }

  g_total_pages =
      (size_t)((aligned_end - aligned_start) / (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  if (g_total_pages > PAGE_ALLOC_MAX_PAGES) {
    g_total_pages = PAGE_ALLOC_MAX_PAGES;
    aligned_end =
        aligned_start + ((uintptr_t)g_total_pages * (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  }

  g_range_start = aligned_start;
  g_range_end = aligned_end;
  g_free_pages = g_total_pages;
}

void *page_alloc(void) {
  size_t scanned;

  if (g_free_pages == 0u) {
    return 0;
  }

  for (scanned = 0u; scanned < g_total_pages; ++scanned) {
    size_t index = (g_next_hint + scanned) % g_total_pages;

    if (bitmap_test(index)) {
      continue;
    }

    bitmap_set(index);
    g_free_pages--;
    g_next_hint = (index + 1u) % g_total_pages;
    return (void *)(g_range_start + ((uintptr_t)index * (uintptr_t)PAGE_ALLOC_PAGE_SIZE));
  }

  return 0;
}

bool page_alloc_owns(const void *page) {
  size_t index;

  if (page == 0) {
    return false;
  }

  return page_index_from_addr((uintptr_t)page, &index);
}

bool page_free(void *page) {
  size_t index;

  if (page == 0 || !page_index_from_addr((uintptr_t)page, &index)) {
    return false;
  }

  if (!bitmap_test(index)) {
    return false;
  }

  bitmap_clear(index);
  g_free_pages++;
  if (index < g_next_hint || g_free_pages == 1u) {
    g_next_hint = index;
  }

  return true;
}

uintptr_t page_alloc_range_start(void) {
  return g_range_start;
}

uintptr_t page_alloc_range_end(void) {
  return g_range_end;
}

size_t page_alloc_total_pages(void) {
  return g_total_pages;
}

size_t page_alloc_free_pages(void) {
  return g_free_pages;
}
