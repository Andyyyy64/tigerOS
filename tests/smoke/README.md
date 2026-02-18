# QEMU Smoke Test Contract

`make test-smoke` runs `scripts/qemu_smoke.sh` and passes only when serial output contains:

- `BOOT: kernel entry`
- at least one of:
  - `TRAP_TEST: handled`
  - `TICK: periodic interrupt`

Behavior:

- boots QEMU in non-interactive mode (`-nographic -monitor none -serial stdio`)
- enforces a bounded timeout (`SMOKE_TIMEOUT_SECONDS`, default `8`)
- fails clearly on timeout or missing markers
- writes captured serial output to `build/qemu-smoke.log`
