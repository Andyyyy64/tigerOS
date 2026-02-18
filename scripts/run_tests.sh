#!/bin/sh
set -eu

if [ "$#" -lt 2 ]; then
  echo "usage: $0 <make-bin> <target> [<target> ...]" >&2
  exit 2
fi

MAKE_BIN="$1"
shift

for target in "$@"; do
  echo "==> $target"
  "$MAKE_BIN" --no-print-directory "$target"
done
