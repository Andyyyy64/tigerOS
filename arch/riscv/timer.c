#include <stdint.h>

#include "timer.h"

enum {
  CSR_SIE_STIE = 1u << 5,
  CSR_SSTATUS_SIE = 1u << 1,
  SBI_EXT_LEGACY_SET_TIMER = 0x00,
  SBI_EXT_TIME = 0x54494d45,
  SBI_TIME_SET_TIMER_FID = 0x00,
};

struct sbi_ret {
  long error;
  long value;
};

static inline uint64_t csr_read_sie(void) {
  uint64_t value;
  __asm__ volatile("csrr %0, sie" : "=r"(value));
  return value;
}

static inline void csr_write_sie(uint64_t value) {
  __asm__ volatile("csrw sie, %0" : : "r"(value));
}

static inline uint64_t csr_read_sstatus(void) {
  uint64_t value;
  __asm__ volatile("csrr %0, sstatus" : "=r"(value));
  return value;
}

static inline void csr_write_sstatus(uint64_t value) {
  __asm__ volatile("csrw sstatus, %0" : : "r"(value));
}

static struct sbi_ret sbi_ecall(long extension_id, long function_id, long arg0, long arg1, long arg2,
                                long arg3, long arg4, long arg5) {
  register long a0 __asm__("a0") = arg0;
  register long a1 __asm__("a1") = arg1;
  register long a2 __asm__("a2") = arg2;
  register long a3 __asm__("a3") = arg3;
  register long a4 __asm__("a4") = arg4;
  register long a5 __asm__("a5") = arg5;
  register long a6 __asm__("a6") = function_id;
  register long a7 __asm__("a7") = extension_id;
  struct sbi_ret ret;

  __asm__ volatile("ecall"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                   : "memory");

  ret.error = a0;
  ret.value = a1;
  return ret;
}

uint64_t riscv_timer_read_time(void) {
  uint64_t value;
  __asm__ volatile("csrr %0, 0xc01" : "=r"(value));
  return value;
}

void riscv_timer_set_deadline(uint64_t deadline) {
  struct sbi_ret ret = sbi_ecall(SBI_EXT_TIME, SBI_TIME_SET_TIMER_FID, (long)deadline, 0, 0, 0, 0, 0);
  if (ret.error == 0L) {
    return;
  }

  (void)sbi_ecall(SBI_EXT_LEGACY_SET_TIMER, 0, (long)deadline, 0, 0, 0, 0, 0);
}

void riscv_timer_enable_interrupts(void) {
  uint64_t sie = csr_read_sie();
  uint64_t sstatus = csr_read_sstatus();

  sie |= CSR_SIE_STIE;
  sstatus |= CSR_SSTATUS_SIE;

  csr_write_sie(sie);
  csr_write_sstatus(sstatus);
}
