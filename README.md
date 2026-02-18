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
make qemu-smoke
```

The smoke test runs `scripts/run_qemu.sh`, boots the kernel on QEMU `virt`, and checks for:

```text
BOOT: kernel entry
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

## Useful Variables

- `CROSS_COMPILE` (default: `riscv64-unknown-elf-`)
- `QEMU` (default: `qemu-system-riscv64`)
- `TIMEOUT_BIN` (default: `timeout`)

Example:

```sh
CROSS_COMPILE=riscv64-linux-gnu- make
QEMU=qemu-system-riscv64 make qemu-smoke
```
