#!/bin/sh
set -eu

KERNEL_ELF="${1:-build/kernel.elf}"
QEMU_BIN="${QEMU_BIN:-qemu-system-riscv64}"
BOOT_MARKER="${BOOT_MARKER:-BOOT: kernel entry}"
TRAP_MARKER="${TRAP_MARKER:-TRAP_TEST: handled}"
TIMER_MARKER="${TIMER_MARKER:-TICK: periodic interrupt}"
SMOKE_TIMEOUT_SECONDS="${SMOKE_TIMEOUT_SECONDS:-8}"
LOG_FILE="${SMOKE_LOG_FILE:-build/qemu-smoke.log}"
POLL_INTERVAL_SECONDS="${SMOKE_POLL_INTERVAL_SECONDS:-1}"

case "$SMOKE_TIMEOUT_SECONDS" in
  ''|*[!0-9]*)
    echo "error: SMOKE_TIMEOUT_SECONDS must be a positive integer" >&2
    exit 2
    ;;
esac

if [ "$SMOKE_TIMEOUT_SECONDS" -le 0 ]; then
  echo "error: SMOKE_TIMEOUT_SECONDS must be greater than zero" >&2
  exit 2
fi

if [ ! -f "$KERNEL_ELF" ]; then
  echo "error: missing kernel image: $KERNEL_ELF" >&2
  exit 1
fi

if ! command -v "$QEMU_BIN" >/dev/null 2>&1; then
  echo "error: missing QEMU binary: $QEMU_BIN" >&2
  exit 1
fi

mkdir -p "$(dirname "$LOG_FILE")"
: > "$LOG_FILE"

QEMU_PID=""
cleanup() {
  if [ -n "$QEMU_PID" ] && kill -0 "$QEMU_PID" >/dev/null 2>&1; then
    kill "$QEMU_PID" >/dev/null 2>&1 || true
    wait "$QEMU_PID" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

"$QEMU_BIN" \
  -machine virt \
  -cpu rv64 \
  -m 128M \
  -smp 1 \
  -nographic \
  -monitor none \
  -serial stdio \
  -kernel "$KERNEL_ELF" \
  >"$LOG_FILE" 2>&1 &
QEMU_PID=$!

start_epoch="$(date +%s)"
boot_seen=0
aux_seen=0

while :; do
  if [ "$boot_seen" -eq 0 ] && grep -F "$BOOT_MARKER" "$LOG_FILE" >/dev/null 2>&1; then
    boot_seen=1
  fi

  if [ "$aux_seen" -eq 0 ]; then
    if grep -F "$TRAP_MARKER" "$LOG_FILE" >/dev/null 2>&1 || \
       grep -F "$TIMER_MARKER" "$LOG_FILE" >/dev/null 2>&1; then
      aux_seen=1
    fi
  fi

  if [ "$boot_seen" -eq 1 ] && [ "$aux_seen" -eq 1 ]; then
    echo "smoke: required boot markers observed" >&2
    cat "$LOG_FILE"
    kill "$QEMU_PID" >/dev/null 2>&1 || true
    wait "$QEMU_PID" 2>/dev/null || true
    QEMU_PID=""
    exit 0
  fi

  if ! kill -0 "$QEMU_PID" >/dev/null 2>&1; then
    if wait "$QEMU_PID"; then
      qemu_status=0
    else
      qemu_status=$?
    fi
    QEMU_PID=""
    echo "error: qemu exited before required markers were observed (status=$qemu_status)" >&2
    echo "error: required markers: '$BOOT_MARKER' and one of '$TRAP_MARKER'/'$TIMER_MARKER'" >&2
    cat "$LOG_FILE"
    exit 1
  fi

  now_epoch="$(date +%s)"
  elapsed_seconds=$((now_epoch - start_epoch))
  if [ "$elapsed_seconds" -ge "$SMOKE_TIMEOUT_SECONDS" ]; then
    echo "error: smoke timeout after ${SMOKE_TIMEOUT_SECONDS}s" >&2
    if [ "$boot_seen" -eq 0 ]; then
      echo "error: missing marker: $BOOT_MARKER" >&2
    fi
    if [ "$aux_seen" -eq 0 ]; then
      echo "error: missing both markers: $TRAP_MARKER | $TIMER_MARKER" >&2
    fi
    cat "$LOG_FILE"
    exit 1
  fi

  sleep "$POLL_INTERVAL_SECONDS"
done
