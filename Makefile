SHELL := /bin/sh

CROSS_COMPILE ?= riscv64-unknown-elf-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

QEMU ?= qemu-system-riscv64
BUILD_DIR := build
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

CFLAGS := -march=rv64imac -mabi=lp64 -mcmodel=medany -ffreestanding -fno-pic -O2 -g0 -Wall -Wextra -Werror
ASFLAGS := $(CFLAGS)
LDFLAGS := -nostdlib -nostartfiles -Wl,--build-id=none -Wl,-T,arch/riscv/linker.ld -Wl,-Map,$(BUILD_DIR)/kernel.map

SRCS_C := kernel/main.c
SRCS_S := arch/riscv/start.S
OBJS := \
	$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS_C)) \
	$(patsubst %.S,$(BUILD_DIR)/%.o,$(SRCS_S))

.PHONY: all clean qemu-smoke

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

clean:
	rm -rf "$(BUILD_DIR)"
