#include <stdint.h>

#include "riscv_timer.h"

enum {
  SBI_LEGACY_SET_TIMER_EID = 0x00UL,
  SBI_EXT_TIME_EID = 0x54494d45UL,
  SBI_EXT_TIME_SET_TIMER_FID = 0x00UL,
};

struct sbi_ret {
  long error;
  long value;
};

static struct sbi_ret sbi_ecall(uint64_t arg0,
                                uint64_t arg1,
                                uint64_t arg2,
                                uint64_t arg3,
                                uint64_t arg4,
                                uint64_t arg5,
                                uint64_t fid,
                                uint64_t eid) {
  register long a0 __asm__("a0") = (long)arg0;
  register long a1 __asm__("a1") = (long)arg1;
  register long a2 __asm__("a2") = (long)arg2;
  register long a3 __asm__("a3") = (long)arg3;
  register long a4 __asm__("a4") = (long)arg4;
  register long a5 __asm__("a5") = (long)arg5;
  register long a6 __asm__("a6") = (long)fid;
  register long a7 __asm__("a7") = (long)eid;
  struct sbi_ret ret;

  __asm__ volatile("ecall"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                   : "memory");

  ret.error = a0;
  ret.value = a1;
  return ret;
}

static void sbi_legacy_set_timer(uint64_t deadline) {
  register long a0 __asm__("a0") = (long)deadline;
  register long a7 __asm__("a7") = (long)SBI_LEGACY_SET_TIMER_EID;

  __asm__ volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
}

uint64_t riscv_timer_now(void) {
  uint64_t now;
  __asm__ volatile("csrr %0, time" : "=r"(now));
  return now;
}

void riscv_timer_set_deadline(uint64_t deadline) {
  struct sbi_ret ret = sbi_ecall(deadline,
                                 0ULL,
                                 0ULL,
                                 0ULL,
                                 0ULL,
                                 0ULL,
                                 SBI_EXT_TIME_SET_TIMER_FID,
                                 SBI_EXT_TIME_EID);

  if (ret.error == 0) {
    return;
  }

  sbi_legacy_set_timer(deadline);
}
