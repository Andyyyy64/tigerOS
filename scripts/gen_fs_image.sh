#!/bin/sh
set -eu

IMAGE_PATH="${1:-build/fs/qemu_fs_rw.img}"
MKFS_BIN="${2:-build/fs/mkfs_otfs}"

if [ ! -x "$MKFS_BIN" ]; then
  echo "error: mkfs binary not executable: $MKFS_BIN" >&2
  exit 1
fi

mkdir -p "$(dirname "$IMAGE_PATH")"
rm -f "$IMAGE_PATH"

"$MKFS_BIN" "$IMAGE_PATH"
echo "image-ready: $IMAGE_PATH"
