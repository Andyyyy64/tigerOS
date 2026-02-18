#include <stdint.h>

#include "timer.h"

enum {
  SBI_LEGACY_SET_TIMER = 0x0,
  SBI_EXT_TIME = 0x54494D45,
  SBI_FID_SET_TIMER = 0x0,
};

struct sbi_ret {
  long error;
  long value;
};

static inline uint64_t csr_read_time(void) {
  uint64_t value;
  __asm__ volatile("csrr %0, time" : "=r"(value));
  return value;
}

static struct sbi_ret sbi_ecall(uint64_t ext,
                                uint64_t fid,
                                uint64_t arg0,
                                uint64_t arg1,
                                uint64_t arg2,
                                uint64_t arg3,
                                uint64_t arg4,
                                uint64_t arg5) {
  register unsigned long a0 __asm__("a0") = (unsigned long)arg0;
  register unsigned long a1 __asm__("a1") = (unsigned long)arg1;
  register unsigned long a2 __asm__("a2") = (unsigned long)arg2;
  register unsigned long a3 __asm__("a3") = (unsigned long)arg3;
  register unsigned long a4 __asm__("a4") = (unsigned long)arg4;
  register unsigned long a5 __asm__("a5") = (unsigned long)arg5;
  register unsigned long a6 __asm__("a6") = (unsigned long)fid;
  register unsigned long a7 __asm__("a7") = (unsigned long)ext;
  struct sbi_ret ret;

  __asm__ volatile("ecall"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                   : "memory");

  ret.error = (long)a0;
  ret.value = (long)a1;
  return ret;
}

static void sbi_legacy_set_timer(uint64_t deadline) {
  register unsigned long a0 __asm__("a0") = (unsigned long)deadline;
  register unsigned long a7 __asm__("a7") = SBI_LEGACY_SET_TIMER;

  __asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
}

uint64_t timer_now(void) {
  return csr_read_time();
}

void timer_set_deadline(uint64_t deadline) {
  struct sbi_ret ret =
      sbi_ecall(SBI_EXT_TIME, SBI_FID_SET_TIMER, deadline, 0, 0, 0, 0, 0);
  if (ret.error != 0) {
    sbi_legacy_set_timer(deadline);
  }
}
