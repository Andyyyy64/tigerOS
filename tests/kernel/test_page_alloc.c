#include <stdint.h>
#include <stdio.h>

#include "page_alloc.h"

#define TEST_ASSERT(cond, msg)                          \
  do {                                                  \
    if (!(cond)) {                                      \
      fprintf(stderr, "FAIL: %s\n", (msg));            \
      return 1;                                         \
    }                                                   \
  } while (0)

static uintptr_t align_up(uintptr_t addr) {
  return (addr + (PAGE_ALLOC_PAGE_SIZE - 1u)) &
         ~((uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u);
}

static uintptr_t align_down(uintptr_t addr) {
  return addr & ~((uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u);
}

static int test_allocation_exhaustion(void) {
  static uint8_t region[(4u * PAGE_ALLOC_PAGE_SIZE) + 128u];
  uintptr_t start = align_up((uintptr_t)&region[0] + 21u);
  uintptr_t end = start + (3u * (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  void *pages[3];
  size_t i;

  page_alloc_init(start, end);
  TEST_ASSERT(page_alloc_total_pages() == 3u, "expected 3-page test pool");
  TEST_ASSERT(page_alloc_free_pages() == 3u, "pool should start fully free");

  for (i = 0u; i < 3u; ++i) {
    pages[i] = page_alloc();
    TEST_ASSERT(pages[i] != 0, "allocation should succeed while capacity remains");
  }

  TEST_ASSERT(page_alloc() == 0, "allocation should fail when pool is exhausted");
  TEST_ASSERT(page_alloc_free_pages() == 0u, "free count should be zero after exhaustion");
  return 0;
}

static int test_double_free_protection(void) {
  static uint8_t region[(3u * PAGE_ALLOC_PAGE_SIZE) + 128u];
  uintptr_t start = align_up((uintptr_t)&region[0]);
  uintptr_t end = start + (2u * (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  void *page = 0;

  page_alloc_init(start, end);
  page = page_alloc();
  TEST_ASSERT(page != 0, "initial allocation should succeed");
  TEST_ASSERT(page_alloc_free_pages() == 1u, "one page should remain free");

  TEST_ASSERT(page_free(page), "first free should succeed");
  TEST_ASSERT(page_alloc_free_pages() == 2u, "free count should recover after free");

  TEST_ASSERT(!page_free(page), "second free of same page must fail");
  TEST_ASSERT(page_alloc_free_pages() == 2u, "double free must not change free count");
  return 0;
}

static int test_free_then_reuse(void) {
  static uint8_t region[5u * PAGE_ALLOC_PAGE_SIZE];
  uintptr_t start = align_up((uintptr_t)&region[0]);
  uintptr_t end = start + (4u * (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  void *pages[4];
  void *reused = 0;
  size_t i;

  page_alloc_init(start, end);
  for (i = 0u; i < 4u; ++i) {
    pages[i] = page_alloc();
    TEST_ASSERT(pages[i] != 0, "setup allocation should succeed");
  }

  TEST_ASSERT(page_free(pages[2]), "free should succeed for allocated page");
  reused = page_alloc();
  TEST_ASSERT(reused == pages[2], "allocator should reuse the just-freed page");
  return 0;
}

static int test_alignment_assumptions(void) {
  static uint8_t region[(3u * PAGE_ALLOC_PAGE_SIZE) + 257u];
  uintptr_t raw_start = (uintptr_t)&region[0] + 17u;
  uintptr_t raw_end = (uintptr_t)&region[sizeof(region)] - 19u;
  uintptr_t expected_start = align_up(raw_start);
  uintptr_t expected_end = align_down(raw_end);
  size_t expected_pages = 0u;
  void *p = 0;

  if (expected_end > expected_start) {
    expected_pages =
        (size_t)((expected_end - expected_start) / (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  }

  page_alloc_init(raw_start, raw_end);
  TEST_ASSERT(page_alloc_range_start() == expected_start, "range start must be page aligned");
  TEST_ASSERT(page_alloc_range_end() == expected_end, "range end must be page aligned");
  TEST_ASSERT(page_alloc_total_pages() == expected_pages, "page count must respect alignment");
  TEST_ASSERT(page_alloc_free_pages() == expected_pages, "free count must match total pages");

  if (expected_pages > 0u) {
    p = page_alloc();
    TEST_ASSERT(p != 0, "allocation should succeed when aligned range has pages");
    TEST_ASSERT((((uintptr_t)p) & ((uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u)) == 0u,
                "allocation result must be 4KiB-aligned");
    TEST_ASSERT(!page_alloc_owns((void *)((uintptr_t)p + 1u)),
                "misaligned pointers should not be treated as owned pages");
  }

  return 0;
}

int page_alloc_tests_run(void) {
  if (test_allocation_exhaustion() != 0) {
    return 1;
  }
  if (test_double_free_protection() != 0) {
    return 1;
  }
  if (test_free_then_reuse() != 0) {
    return 1;
  }
  if (test_alignment_assumptions() != 0) {
    return 1;
  }

  printf("page allocator unit tests passed\n");
  return 0;
}
