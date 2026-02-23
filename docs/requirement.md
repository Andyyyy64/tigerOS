Welcome to openTiger. What would you like to work on today?

I can help with code tasks, research, bug fixes, and feature development.

# rustyOS - RISC-V OS in Rust

## Technical Decisions

| Item | Choice |
|------|--------|
| Target | `riscv64gc-unknown-none-elf` (64-bit RISC-V) |
| Rust toolchain | nightly (required for `#![no_std]` OS features) |
| QEMU machine | `qemu-system-riscv64 -machine virt` |
| Firmware | OpenSBI (QEMU built-in) |
| Console | UART NS16550A (MMIO: `0x1000_0000`) |
| Build | cargo + custom linker script + Makefile |

## Task Plan

### 1. Project Scaffolding [Risk: Low]
- `cargo init --name rustyos`
- `rust-toolchain.toml` with nightly + `riscv64gc-unknown-none-elf` target
- `.cargo/config.toml` with target/linker settings
- `linker.ld` - memory layout for QEMU virt machine (RAM starts at `0x8000_0000`)

### 2. Minimal Boot Entry [Risk: Low]
- `src/main.rs` - `#![no_std]`, `#![no_main]`, panic handler
- `src/entry.asm` - RISC-V assembly entry point (`_start`), stack setup, jump to `kmain`
- Boot into `kmain()` and halt

### 3. UART Driver & Console Output [Risk: Low]
- `src/uart.rs` - NS16550A UART driver (MMIO read/write)
- `print!` / `println!` macros
- "rustyOS booted!" message on startup

### 4. QEMU Run Environment [Risk: Low]
- `Makefile` with `build`, `run`, `clean` targets
- `qemu-system-riscv64` launch command with proper flags
- Debug support via `-s -S` flags for GDB

### 5. Page Allocator [Risk: Medium]
- `src/page.rs` - physical page allocator (bitmap-based)
- Parse memory region from linker symbols (`_heap_start`, `_heap_end`)
- `alloc_page()` / `dealloc_page()`

### 6. Virtual Memory (Sv39) [Risk: Medium]
- `src/mmu.rs` - RISC-V Sv39 page table setup
- Identity mapping for kernel
- Enable paging via `satp` CSR

### 7. Trap Handling [Risk: Medium]
- `src/trap.rs` - trap vector, exception/interrupt dispatcher
- `src/trap.asm` - context save/restore
- Timer interrupt handling (CLINT)

### 8. Simple Shell [Risk: Low]
- `src/shell.rs` - UART input reading, basic command parsing
- Commands: `help`, `info`, `echo`, `clear`
- Interactive prompt: `rustyOS> `

## Execution Order

```
1 → 2 → 3 → 4 → 5 → 6 → 7 → 8
 scaffolding → boot → uart → qemu → alloc → mmu → traps → shell
```

Each step produces a runnable (or at least buildable) state. Steps 1-4 give you a booting kernel with console output. Steps 5-7 add core OS primitives. Step 8 adds interactivity.

## Prerequisites (Install)
```
rustup, qemu-system-riscv64, riscv64-unknown-elf-gcc (for linker)
```
