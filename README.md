# RISC-V Kernel Bootstrap

Minimal RISC-V kernel bootstrap for QEMU `virt` with UART boot logging.

## Prerequisites

- `riscv64-unknown-elf-gcc` toolchain (or compatible prefix via `CROSS_COMPILE`)
- `qemu-system-riscv64`
- `timeout`
- `make`

## Build

```sh
make
```

Build outputs:

- `build/kernel.elf`
- `build/kernel.bin`
- `build/kernel.dis`
- `build/kernel.map`

## Smoke Boot Test

```sh
make test-smoke
```

The smoke test runs `scripts/qemu_smoke.sh`, boots the kernel on QEMU `virt`, and checks for:

```text
BOOT: kernel entry
TRAP_TEST: handled (or TICK: periodic interrupt)
```

With timer interrupts enabled during `clock_init()`, boot output may include up to
four `TICK: periodic interrupt` lines as periodic timer interrupts are serviced.

## Trap Handler Test

```sh
make qemu-trap-test
```

The trap test boots the kernel, triggers an `ebreak`, and verifies trap handling
flow:

```text
BOOT: kernel entry
TRAP_TEST: mcause=0x0000000000000003 mepc=0x...
TRAP_TEST: handled
```

Unexpected traps are logged with cause and fault context before halting:

```text
TRAP: unexpected mcause=0x... mepc=0x... mtval=0x...
```

## Framebuffer Graphics Test

```sh
make qemu-gfx-test
```

The graphics test boots the kernel and verifies framebuffer initialization plus deterministic
pixel rendering output:

```text
GFX: framebuffer initialized
GFX: deterministic marker 0x...
```

## Window Manager Single-Window Composition Test

```sh
make qemu-wm-single-test
```

Boots the kernel and validates initial window creation/composition behavior:

- initializes a window frame/title with default style values
- draws border, title bar, and content regions through the compositor
- emits a deterministic composition marker for verification

```text
WM: single window composed marker 0x...
```

## Window Manager Overlap/Focus Layer Test

```sh
make qemu-wm-overlap-test
```

Boots the kernel with overlapping windows and verifies overlap hit-testing plus focus/layer activation:

- hit-test on overlap initially resolves to the front-most window
- activating the back window moves it to the front of the layer stack
- active window tracking updates and composed output marker changes

Expected output includes:

```text
WM: overlap focus activation marker 0x...
```

## Window Manager Mouse Input/Drag Dispatch Test

```sh
make qemu-mouse-test
```

Boots the kernel with two draggable windows and validates the mouse input pipeline end-to-end:

- mouse move/button events are queued
- click-down dispatch targets the hit-tested front window/task
- left-button title-bar drag updates the dragged window frame
- click-up ends drag tracking and dispatch flow

Expected output includes:

```text
WM: mouse dispatch drag marker 0x...
```

## UART Line I/O Echo Test

```sh
make qemu-serial-echo-test
```

The serial echo test boots the kernel, sends a line over UART, and verifies:

```text
BOOT: kernel entry
echo: uart line echo test
```

## Physical Page Allocator Unit Tests

```sh
make test-page-alloc
```

Builds the host-side allocator unit test binary (`build/test-page-alloc`) and runs it through
the unit test harness script (`scripts/run_unit_tests.sh`).

The tests cover allocation exhaustion, double-free protection, free-then-reuse behavior, and
page-range alignment assumptions.

Expected output:

```text
page allocator unit tests passed
all unit tests passed
```

## Filesystem Mount/Read/Write Test

```sh
make qemu-fs-rw-test
```

Builds host-side OTFS utilities, generates a deterministic filesystem image, and runs the
read/write validation flow. The test covers mount/unmount, create+truncate semantics,
offset writes via `fs_seek`, multi-block reads/writes, sparse writes with zero-filled gaps,
and remount persistence checks.

Expected output includes:

```text
PASS: mount/open/read/write/close checks completed
```

## Directory Traversal and Path Resolution Unit Test

```sh
make test-fs-dir
```

Builds and runs the host-side directory/path unit tests (`build/fs/fs_dir_test`) that validate:

- path normalization (`fs_path_normalize`)
- absolute/relative path resolution with root clamping (`fs_path_resolve`)
- directory traversal and navigation (`fs_dir_walk`, `fs_dir_cd`, `fs_dir_pwd`)
- deterministic directory listing order and count reporting (`fs_dir_readdir`)
- directory creation semantics (`fs_dir_mkdir`, `fs_dir_mkdir_p`)

Expected output includes:

```text
fs dir tests passed
```

## Useful Variables

- `CROSS_COMPILE` (default: `riscv64-unknown-elf-`)
- `QEMU` (default: `qemu-system-riscv64`)
- `TIMEOUT_BIN` (default: `timeout`)

Example:

```sh
CROSS_COMPILE=riscv64-linux-gnu- make
QEMU=qemu-system-riscv64 make qemu-smoke
```
