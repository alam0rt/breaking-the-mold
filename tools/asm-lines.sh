#!/usr/bin/env bash
# Disassemble a C file with source-line interleaving, so you can see which
# C statement each MIPS instruction came from. Pairs cc1 -gstabs with
# objdump -dl to emulate decomp.me's "Show source" view.
#
# Usage: tools/asm-lines.sh path/to/file.c [function_name]
#
# Notes:
#   * Uses the same cc1/as flags as the build (-O2 -G8 -mcpu=3000).
#   * -gstabs is required because cc1 2.7.2-psx's default -g emits COFF
#     directives (.def/.val/.scl/.endef) that modern GNU as rejects.
#   * Skips maspsx; line-info works on raw as output.
set -euo pipefail

cd "$(dirname "$0")/.."

if [ $# -lt 1 ]; then
  echo "Usage: $0 file.c [function_name]" >&2
  exit 1
fi

SRC="$1"
FUNC="${2:-}"

WORK=$(mktemp -d)
trap "rm -rf $WORK" EXIT

cpp -nostdinc -D_LANGUAGE_C "$SRC" > "$WORK/out.i" 2>/dev/null

tools/gcc-2.7.2-psx/cc1 -O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000 -gstabs \
    -o "$WORK/out.s" "$WORK/out.i" 2>/dev/null

mipsel-unknown-linux-gnu-as -EL -G8 -march=r3000 -mtune=r3000 \
    -no-pad-sections -g -o "$WORK/out.o" "$WORK/out.s" 2>/dev/null

if [ -n "$FUNC" ]; then
  mipsel-unknown-linux-gnu-objdump -dl "$WORK/out.o" \
    | sed -n "/<${FUNC}>:/,/^$/p"
else
  mipsel-unknown-linux-gnu-objdump -dl "$WORK/out.o"
fi
