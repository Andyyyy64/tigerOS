SHELL := /bin/sh

CROSS_COMPILE ?= riscv64-unknown-elf-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

QEMU ?= qemu-system-riscv64
TIMEOUT_BIN ?= timeout
BUILD_DIR := build
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

CFLAGS := -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany -ffreestanding -fno-pic -O2 -g0 -Wall -Wextra -Werror
ASFLAGS := $(CFLAGS)
LDFLAGS := -nostdlib -nostartfiles -Wl,--build-id=none -Wl,-T,arch/riscv/linker.ld -Wl,-Map,$(BUILD_DIR)/kernel.map
HOST_CC ?= cc
HOST_CFLAGS ?= -std=c11 -O2 -g0 -Wall -Wextra -Werror

SRCS_C := \
	drivers/uart/uart.c \
	kernel/console.c \
	kernel/mm/init.c \
	kernel/mm/page_alloc.c \
	kernel/main.c \
	kernel/trap.c \
	shell/line_io.c \
	kernel/gfx/framebuffer.c \
	drivers/video/qemu_virt_fb.c
SRCS_S := \
	arch/riscv/start.S \
	arch/riscv/trap.S
OBJS := \
	$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS_C)) \
	$(patsubst %.S,$(BUILD_DIR)/%.o,$(SRCS_S))

TEST_PAGE_ALLOC_BIN := $(BUILD_DIR)/test-page-alloc
TEST_PAGE_ALLOC_SRCS := \
	tests/kernel/test_main.c \
	tests/kernel/test_page_alloc.c \
	kernel/mm/page_alloc.c

.PHONY: all clean qemu-smoke qemu-gfx-test qemu-serial-echo-test qemu-trap-test test-page-alloc

all: $(KERNEL_ELF) $(KERNEL_BIN)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) -Iinclude -c "$<" -o "$@"

$(BUILD_DIR)/%.o: %.S
	@mkdir -p "$(dir $@)"
	$(CC) $(ASFLAGS) -Iinclude -c "$<" -o "$@"

$(KERNEL_ELF): $(OBJS) arch/riscv/linker.ld
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o "$@"
	$(OBJDUMP) -d "$@" > "$(BUILD_DIR)/kernel.dis"

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary "$<" "$@"

qemu-smoke: $(KERNEL_ELF) scripts/run_qemu.sh
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "BOOT: kernel entry"

qemu-gfx-test: $(KERNEL_ELF) scripts/run_qemu.sh
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "GFX: framebuffer initialized"
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "GFX: deterministic marker 0x"

qemu-serial-echo-test: $(KERNEL_ELF)
	@set -eu; \
	TEST_LINE="uart line echo test"; \
	OUTPUT="$$( \
		{ sleep 1; printf '%s\r\n' "$$TEST_LINE"; } | \
		"$(TIMEOUT_BIN)" 6s "$(QEMU)" \
			-machine virt \
			-cpu rv64 \
			-m 128M \
			-smp 1 \
			-nographic \
			-monitor none \
			-serial stdio \
			-kernel "$(KERNEL_ELF)" \
			2>&1 || true \
	)"; \
	printf '%s\n' "$$OUTPUT"; \
	printf '%s\n' "$$OUTPUT" | grep -F "BOOT: kernel entry" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "echo: $$TEST_LINE" >/dev/null

qemu-trap-test: $(KERNEL_ELF)
	@set -eu; \
	OUTPUT="$$( \
		"$(TIMEOUT_BIN)" 5s "$(QEMU)" \
			-machine virt \
			-cpu rv64 \
			-m 128M \
			-smp 1 \
			-nographic \
			-monitor none \
			-serial stdio \
			-kernel "$(KERNEL_ELF)" \
			2>&1 || true \
	)"; \
	printf '%s\n' "$$OUTPUT"; \
	printf '%s\n' "$$OUTPUT" | grep -F "BOOT: kernel entry" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "TRAP_TEST: mcause=0x0000000000000003" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "TRAP_TEST: mepc=0x" >/dev/null

$(TEST_PAGE_ALLOC_BIN): $(TEST_PAGE_ALLOC_SRCS) include/page_alloc.h
	@mkdir -p "$(BUILD_DIR)"
	$(HOST_CC) $(HOST_CFLAGS) -Iinclude $(TEST_PAGE_ALLOC_SRCS) -o "$@"

test-page-alloc: $(TEST_PAGE_ALLOC_BIN) scripts/run_unit_tests.sh
	./scripts/run_unit_tests.sh "$(TEST_PAGE_ALLOC_BIN)"

clean:
	rm -rf "$(BUILD_DIR)"
