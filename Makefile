SHELL := /bin/sh

CROSS_COMPILE ?= riscv64-unknown-elf-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

QEMU ?= qemu-system-riscv64
TIMEOUT_BIN ?= timeout
HOST_CC ?= cc
BUILD_DIR := build
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
FS_BUILD_DIR := $(BUILD_DIR)/fs
FS_TEST_BIN := $(FS_BUILD_DIR)/fs_rw_test
FS_MKFS_BIN := $(FS_BUILD_DIR)/mkfs_otfs
FS_TEST_IMAGE := $(FS_BUILD_DIR)/qemu_fs_rw.img
FS_DIR_TEST_BIN := $(FS_BUILD_DIR)/fs_dir_test

CFLAGS := -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany -ffreestanding -fno-pic -O2 -g0 -Wall -Wextra -Werror
ASFLAGS := $(CFLAGS)
LDFLAGS := -nostdlib -nostartfiles -Wl,--build-id=none -Wl,-T,arch/riscv/linker.ld -Wl,-Map,$(BUILD_DIR)/kernel.map
HOST_CFLAGS ?= -std=c11 -O2 -g0 -Wall -Wextra -Werror

SRCS_C := \
	arch/riscv/timer.c \
	drivers/uart/uart.c \
	drivers/input/mouse.c \
	drivers/input/keyboard.c \
	kernel/console.c \
	kernel/clock.c \
	kernel/trap.c \
	kernel/task/task.c \
	kernel/sched/rr.c \
	kernel/mm/init.c \
	kernel/mm/page_alloc.c \
	kernel/input/event_queue.c \
	kernel/input/keyboard_dispatch.c \
	apps/libapp/app_window.c \
	apps/demo_window_app.c \
	apps/terminal/multi_terminal_test.c \
	kernel/main.c \
	shell/line_io.c \
	shell/shell.c \
	shell/parser.c \
	shell/builtins_basic.c \
	shell/builtins_fs.c \
	shell/path_state.c \
	fs/path.c \
	fs/dir.c \
	kernel/gfx/framebuffer.c \
	kernel/tty/terminal_session.c \
	kernel/wm/window.c \
	kernel/wm/layers.c \
	kernel/wm/focus.c \
	kernel/wm/compositor.c \
	kernel/wm/drag.c \
	kernel/wm/terminal_window.c \
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
TEST_SCHED_TIMER_BIN := $(BUILD_DIR)/test-sched-timer
TEST_SHELL_BIN := $(BUILD_DIR)/test-shell

.PHONY: all clean test test-smoke qemu-smoke qemu-gfx-test qemu-wm-single-test qemu-wm-overlap-test qemu-keyboard-focus-test qemu-multi-term-test qemu-mouse-test qemu-app-window-test qemu-serial-echo-test qemu-shell-basic-test qemu-shell-fs-test qemu-trap-test qemu-timer-test qemu-sched-test qemu-fs-rw-test test-page-alloc test-fs-dir test-sched-timer test-shell

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

$(FS_TEST_BIN): fs/fs_rw_test.c fs/otfs.c include/fs.h
	@mkdir -p "$(FS_BUILD_DIR)"
	$(HOST_CC) $(HOST_CFLAGS) -Iinclude fs/fs_rw_test.c fs/otfs.c -o "$@"

$(FS_MKFS_BIN): fs/mkfs_otfs.c fs/otfs.c include/fs.h
	@mkdir -p "$(FS_BUILD_DIR)"
	$(HOST_CC) $(HOST_CFLAGS) -Iinclude fs/mkfs_otfs.c fs/otfs.c -o "$@"

$(FS_DIR_TEST_BIN): tests/fs/test_fs_dir.c fs/dir.c fs/path.c include/fs_dir.h include/fs_path.h
	@mkdir -p "$(FS_BUILD_DIR)"
	$(HOST_CC) $(HOST_CFLAGS) -Iinclude tests/fs/test_fs_dir.c fs/dir.c fs/path.c -o "$@"

test-smoke: $(KERNEL_ELF) scripts/qemu_smoke.sh
	QEMU_BIN="$(QEMU)" ./scripts/qemu_smoke.sh "$(KERNEL_ELF)"

qemu-smoke: test-smoke

qemu-gfx-test: $(KERNEL_ELF) scripts/run_qemu.sh
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "GFX: framebuffer initialized"
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "GFX: deterministic marker 0x"

qemu-wm-single-test: $(KERNEL_ELF) scripts/run_qemu.sh
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "WM: single window composed marker 0x"

qemu-wm-overlap-test: $(KERNEL_ELF) scripts/run_qemu.sh
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "WM: overlap focus activation marker 0x"

qemu-mouse-test: $(KERNEL_ELF) scripts/run_qemu.sh
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "WM: mouse dispatch drag marker 0x"

qemu-keyboard-focus-test: $(KERNEL_ELF) scripts/run_qemu.sh
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "WM: keyboard focus routing marker 0x"

qemu-multi-term-test: $(KERNEL_ELF) scripts/run_qemu.sh
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "WM: multi terminal isolation marker 0x"

qemu-app-window-test: $(KERNEL_ELF) scripts/run_qemu.sh
	QEMU_BIN="$(QEMU)" ./scripts/run_qemu.sh "$(KERNEL_ELF)" "APP: demo window callback marker 0x"

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

qemu-shell-basic-test: $(KERNEL_ELF)
	@set -eu; \
	OUTPUT="$$( \
		{ \
			sleep 1; \
			printf '%s\r\n' "help"; \
			sleep 1; \
			printf '%s\r\n' "echo shell basic"; \
			sleep 1; \
			printf '%s\r\n' "meminfo"; \
		} | \
		"$(TIMEOUT_BIN)" 8s "$(QEMU)" \
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
	printf '%s\n' "$$OUTPUT" | grep -F "available commands:" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "help - show this help" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "echo - print arguments" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "meminfo - show allocator usage" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "echo: shell basic" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "meminfo: range=0x" >/dev/null

qemu-shell-fs-test: $(KERNEL_ELF)
	@set -eu; \
	OUTPUT="$$( \
		{ \
			sleep 1; \
			printf '%s\r\n' "help"; \
			sleep 1; \
			printf '%s\r\n' "pwd"; \
			sleep 1; \
			printf '%s\r\n' "ls"; \
			sleep 1; \
			printf '%s\r\n' "cat hello.txt"; \
			sleep 1; \
			printf '%s\r\n' "mkdir projects"; \
			sleep 1; \
			printf '%s\r\n' "cd projects"; \
			sleep 1; \
			printf '%s\r\n' "pwd"; \
			sleep 1; \
			printf '%s\r\n' "mkdir notes"; \
			sleep 1; \
			printf '%s\r\n' "ls"; \
			sleep 1; \
			printf '%s\r\n' "cd /"; \
			sleep 1; \
			printf '%s\r\n' "ls"; \
			sleep 1; \
			printf '%s\r\n' "cat /etc/motd"; \
			sleep 1; \
			printf '%s\r\n' "cd /projects"; \
			sleep 1; \
			printf '%s\r\n' "cd /missing"; \
			sleep 1; \
			printf '%s\r\n' "pwd"; \
		} | \
		"$(TIMEOUT_BIN)" 16s "$(QEMU)" \
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
	printf '%s\n' "$$OUTPUT" | grep -F "ls - list files and directories" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "cat - print file contents" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "pwd - print current directory" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "cd - change current directory" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "mkdir - create directory" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "etc/" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "hello.txt" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "hello from shell fs" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "/projects" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "notes/" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "projects/" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "openTiger shell filesystem" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "cd: no such directory" >/dev/null

qemu-fs-rw-test: $(FS_TEST_BIN) $(FS_MKFS_BIN) scripts/gen_fs_image.sh
	./scripts/gen_fs_image.sh "$(FS_TEST_IMAGE)" "$(FS_MKFS_BIN)"
	"$(FS_TEST_BIN)" "$(FS_TEST_IMAGE)"

qemu-trap-test: $(KERNEL_ELF)
	@set -eu; \
	OUTPUT="$$( \
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
	printf '%s\n' "$$OUTPUT" | grep -F "TRAP_TEST: mcause=0x0000000000000003 mepc=0x" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "TRAP_TEST: handled" >/dev/null

qemu-timer-test: $(KERNEL_ELF)
	@set -eu; \
	OUTPUT="$$( \
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
	printf '%s\n' "$$OUTPUT" | grep -F "TICK: periodic interrupt" >/dev/null

qemu-sched-test: $(KERNEL_ELF)
	@set -eu; \
	OUTPUT="$$( \
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
	printf '%s\n' "$$OUTPUT" | grep -F "SCHED: policy=round-robin runnable=2" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "SCHED: switch 1 -> 2" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "SCHED: switch 2 -> 1" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "TASK: 1 running" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "TASK: 2 running" >/dev/null; \
	printf '%s\n' "$$OUTPUT" | grep -F "SCHED_TEST: alternating tasks confirmed" >/dev/null

$(TEST_PAGE_ALLOC_BIN): $(TEST_PAGE_ALLOC_SRCS) include/page_alloc.h
	@mkdir -p "$(BUILD_DIR)"
	$(HOST_CC) $(HOST_CFLAGS) -Iinclude $(TEST_PAGE_ALLOC_SRCS) -o "$@"

test-page-alloc: $(TEST_PAGE_ALLOC_BIN) scripts/run_unit_tests.sh
	./scripts/run_unit_tests.sh "$(TEST_PAGE_ALLOC_BIN)"

test-fs-dir: $(FS_DIR_TEST_BIN) scripts/run_unit_tests.sh
	./scripts/run_unit_tests.sh "$(FS_DIR_TEST_BIN)"

$(TEST_SCHED_TIMER_BIN): tests/kernel/test_sched_timer.c kernel/sched/rr.c kernel/task/task.c kernel/clock.c include/sched.h include/task.h include/clock.h include/trap.h include/line_io.h include/console.h include/riscv_timer.h
	@mkdir -p "$(BUILD_DIR)"
	$(HOST_CC) $(HOST_CFLAGS) -Iinclude tests/kernel/test_sched_timer.c kernel/sched/rr.c kernel/task/task.c kernel/clock.c -o "$@"

test-sched-timer: $(TEST_SCHED_TIMER_BIN) scripts/run_unit_tests.sh
	./scripts/run_unit_tests.sh "$(TEST_SCHED_TIMER_BIN)"

$(TEST_SHELL_BIN): tests/shell/test_shell_commands.c shell/parser.c shell/builtins_basic.c shell/builtins_fs.c shell/path_state.c fs/path.c fs/dir.c kernel/mm/page_alloc.c include/shell_builtins.h include/shell_builtins_fs.h include/shell_parser.h include/path_state.h include/fs_dir.h include/fs_path.h include/page_alloc.h include/line_io.h include/console.h
	@mkdir -p "$(BUILD_DIR)"
	$(HOST_CC) $(HOST_CFLAGS) -Iinclude tests/shell/test_shell_commands.c shell/parser.c shell/builtins_basic.c shell/builtins_fs.c shell/path_state.c fs/path.c fs/dir.c kernel/mm/page_alloc.c -o "$@"

test-shell: $(TEST_SHELL_BIN) scripts/run_unit_tests.sh
	./scripts/run_unit_tests.sh "$(TEST_SHELL_BIN)"

test: scripts/run_tests.sh
	./scripts/run_tests.sh "$(MAKE)" test-page-alloc test-sched-timer test-fs-dir test-shell

clean:
	rm -rf "$(BUILD_DIR)"
