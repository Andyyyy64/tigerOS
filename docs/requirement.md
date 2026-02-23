Welcome to openTiger. What would you like to work on today?

I can help with code tasks, research, bug fixes, and feature development.

RISC-V + Rust + QEMU でミニマルOSを構築するプランです。

## 技術選定

| 項目 | 選定 |
|------|------|
| ターゲット | `riscv64gc-unknown-none-elf` (64-bit) |
| Rust toolchain | nightly（`#![no_std]`, inline asm, カスタムリンカスクリプト必須） |
| QEMU | `qemu-system-riscv64` / `virt` マシン |
| ファームウェア | OpenSBI（QEMU同梱） |
| コンソールI/O | UART NS16550A（QEMU virt: `0x1000_0000`） |
| ビルド | `cargo` + カスタムリンカスクリプト |

---

## タスクプラン

### 1. 開発環境セットアップ
Rust nightly, `riscv64gc-unknown-none-elf` ターゲット, QEMU, binutils をインストール  
**リスク: 低** | 順序: 最初

### 2. プロジェクト骨格の作成
`#![no_std]` / `#![no_main]` のRustプロジェクト、`.cargo/config.toml` でターゲット・リンカ設定、`linker.ld`（メモリレイアウト: RAM `0x8020_0000`〜）  
**リスク: 低** | 順序: 2番目

### 3. ブートコード（アセンブリエントリポイント）
`_start` でスタックポインタ設定 → Rustの `kmain` にジャンプ。BSS初期化、hartid == 0 以外を待機ループへ  
**リスク: 中** | 順序: 3番目

### 4. UART ドライバ実装
NS16550A レジスタ直接操作で `putchar` / `print!` マクロ実装。デバッグ出力の基盤  
**リスク: 低** | 順序: 4番目

### 5. ページテーブル・メモリ管理
Sv39（3レベルページテーブル）のセットアップ。カーネル空間のアイデンティティマッピング、簡易フレームアロケータ  
**リスク: 高** | 順序: 5番目

### 6. トラップ・割り込みハンドラ
`stvec` 設定、トラップベクタ実装。タイマー割り込み（CLINT）、外部割り込み（PLIC）、例外ハンドリング  
**リスク: 高** | 順序: 6番目

### 7. プロセス管理（基礎）
コンテキスト構造体、コンテキストスイッチ（レジスタ保存・復元）、簡易ラウンドロビンスケジューラ  
**リスク: 高** | 順序: 7番目

### 8. システムコールインターフェース
`ecall` による S-mode ↔ U-mode のシステムコール。`write`, `exit` を最低限実装  
**リスク: 中** | 順序: 8番目

### 9. Makefile / 実行スクリプト
`cargo build` → ELFバイナリ生成 → `qemu-system-riscv64 -machine virt -kernel ...` でワンコマンド起動  
**リスク: 低** | 順序: 並行可

---

## 実行順序

```
[1] 環境セットアップ
 ↓
[2] プロジェクト骨格
 ↓
[3] ブートコード → [4] UART → "Hello, RISC-V OS!" 出力確認
 ↓
[5] メモリ管理 → [6] トラップハンドラ
 ↓
[7] プロセス管理 → [8] システムコール
```

ステップ4完了時点で QEMU 上に文字が表示され、動作確認できる最初のマイルストーンとなります。
