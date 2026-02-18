#!/bin/sh
set -eu

KERNEL_ELF="${1:-build/kernel.elf}"
MARKER="${2:-BOOT: kernel entry}"
QEMU_BIN="${QEMU_BIN:-qemu-system-riscv64}"
TIMEOUT_BIN="${TIMEOUT_BIN:-timeout}"

if [ ! -f "$KERNEL_ELF" ]; then
  echo "error: missing kernel image: $KERNEL_ELF" >&2
  exit 1
fi

if ! command -v "$QEMU_BIN" >/dev/null 2>&1; then
  echo "error: missing QEMU binary: $QEMU_BIN" >&2
  exit 1
fi

if ! command -v "$TIMEOUT_BIN" >/dev/null 2>&1; then
  echo "error: missing timeout binary: $TIMEOUT_BIN" >&2
  exit 1
fi

OUTPUT="$("$TIMEOUT_BIN" 5s "$QEMU_BIN" \
  -machine virt \
  -cpu rv64 \
  -m 128M \
  -smp 1 \
  -nographic \
  -monitor none \
  -serial stdio \
  -kernel "$KERNEL_ELF" \
  2>&1 || true)"

printf '%s\n' "$OUTPUT"
printf '%s' "$OUTPUT" | grep -F "$MARKER" >/dev/null
