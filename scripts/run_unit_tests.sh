#!/bin/sh
set -eu

if [ "$#" -lt 1 ]; then
  echo "usage: $0 <test-binary> [<test-binary> ...]" >&2
  exit 2
fi

for test_bin in "$@"; do
  if [ ! -f "$test_bin" ]; then
    echo "error: missing unit test binary: $test_bin" >&2
    exit 1
  fi
  if [ ! -x "$test_bin" ]; then
    echo "error: unit test binary is not executable: $test_bin" >&2
    exit 1
  fi

  "$test_bin"
done
