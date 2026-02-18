#include <stdint.h>
#include <stdio.h>

#include "page_alloc.h"

#define TEST_ASSERT(cond, msg)                      \
  do {                                              \
    if (!(cond)) {                                  \
      fprintf(stderr, "FAIL: %s\n", (msg));       \
      return 1;                                     \
    }                                               \
  } while (0)

static uintptr_t align_up(uintptr_t addr) {
  return (addr + (PAGE_ALLOC_PAGE_SIZE - 1u)) & ~((uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u);
}

static uintptr_t align_down(uintptr_t addr) {
  return addr & ~((uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u);
}

static int test_init_boundaries(void) {
  static uint8_t region[(2u * PAGE_ALLOC_PAGE_SIZE) + 512u];
  uintptr_t start = (uintptr_t)&region[0] + 123u;
  uintptr_t end = (uintptr_t)&region[sizeof(region)] - 77u;
  uintptr_t expected_start = align_up(start);
  uintptr_t expected_end = align_down(end);
  size_t expected_pages = 0u;

  if (expected_end > expected_start) {
    expected_pages =
        (size_t)((expected_end - expected_start) / (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  }

  page_alloc_init(start, end);

  TEST_ASSERT(page_alloc_range_start() == expected_start,
              "init should align range start to 4KiB");
  TEST_ASSERT(page_alloc_range_end() == expected_end, "init should align range end to 4KiB");
  TEST_ASSERT(page_alloc_total_pages() == expected_pages,
              "init should compute expected page count");
  TEST_ASSERT(page_alloc_free_pages() == expected_pages,
              "all pages should be free right after init");

  return 0;
}

static int test_alloc_free_reuse(void) {
  static uint8_t region[5u * PAGE_ALLOC_PAGE_SIZE];
  uintptr_t start = align_up((uintptr_t)&region[0]);
  uintptr_t end = start + (4u * (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  void *pages[4];
  size_t i;

  page_alloc_init(start, end);
  TEST_ASSERT(page_alloc_total_pages() == 4u, "expected four pages in test region");

  for (i = 0u; i < 4u; ++i) {
    pages[i] = page_alloc();
    TEST_ASSERT(pages[i] != 0, "allocation should succeed while free pages remain");
    TEST_ASSERT(((uintptr_t)pages[i] & ((uintptr_t)PAGE_ALLOC_PAGE_SIZE - 1u)) == 0u,
                "allocated pages must be 4KiB-aligned");
  }

  TEST_ASSERT(page_alloc() == 0, "allocation past capacity should fail");
  TEST_ASSERT(page_alloc_free_pages() == 0u, "free page count should hit zero at capacity");

  TEST_ASSERT(page_free(pages[1]), "free on allocated page should succeed");
  TEST_ASSERT(!page_free(pages[1]), "double free must fail");
  TEST_ASSERT(page_alloc_free_pages() == 1u, "free page count should update after free");

  TEST_ASSERT(page_alloc() == pages[1], "allocator should reuse returned page");
  TEST_ASSERT(page_alloc_free_pages() == 0u, "free page count should return to zero");

  return 0;
}

static int test_ownership_and_invalid_free(void) {
  static uint8_t region_a[3u * PAGE_ALLOC_PAGE_SIZE];
  static uint8_t region_b[3u * PAGE_ALLOC_PAGE_SIZE];
  uintptr_t start_a = align_up((uintptr_t)&region_a[0]);
  uintptr_t end_a = start_a + (2u * (uintptr_t)PAGE_ALLOC_PAGE_SIZE);
  uintptr_t foreign_page = align_up((uintptr_t)&region_b[0]);
  void *p;

  page_alloc_init(start_a, end_a);
  p = page_alloc();
  TEST_ASSERT(p != 0, "setup allocation should succeed");

  TEST_ASSERT(page_alloc_owns(p), "allocator should own allocated page");
  TEST_ASSERT(!page_alloc_owns((void *)(foreign_page)),
              "allocator should reject page outside managed range");
  TEST_ASSERT(!page_alloc_owns((void *)((uintptr_t)p + 1u)),
              "allocator should reject misaligned addresses");

  TEST_ASSERT(!page_free(0), "free(NULL) must fail");
  TEST_ASSERT(!page_free((void *)(foreign_page)),
              "free outside managed range must fail");
  TEST_ASSERT(!page_free((void *)((uintptr_t)p + 1u)),
              "free on non-page-aligned address must fail");
  TEST_ASSERT(page_free(p), "free on owned allocated page should succeed");

  return 0;
}

int main(void) {
  if (test_init_boundaries() != 0) {
    return 1;
  }
  if (test_alloc_free_reuse() != 0) {
    return 1;
  }
  if (test_ownership_and_invalid_free() != 0) {
    return 1;
  }

  printf("page_alloc tests passed\n");
  return 0;
}
