#include <stdio.h>

int page_alloc_tests_run(void);

int main(void) {
  int rc = page_alloc_tests_run();
  if (rc != 0) {
    return rc;
  }

  printf("all unit tests passed\n");
  return 0;
}
