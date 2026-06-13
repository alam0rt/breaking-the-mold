#!/usr/bin/env bash
# fdiff.sh <rom_offset_hex> <size_hex>
# Disassemble the same byte range from the original ROM (bin/SLES_010.90) and the
# freshly built binary (build/SLES_010.90.bin) and diff them. Prints MATCH on
# success. Much faster than a full asm-differ run for checking one function.
# Run inside `nix develop` (needs mipsel-unknown-linux-gnu-objdump).
set -euo pipefail
off=$((16#$1)); size=$((16#$2)); vram=$((off - 0x800 + 0x80010000))
for f in bin/SLES_010.90 build/SLES_010.90.bin; do
  dd if="$f" of="/tmp/$(basename "$f").chunk" bs=1 skip=$off count=$size 2>/dev/null
done
d() { mipsel-unknown-linux-gnu-objdump -D -b binary -m mips:3000 -EL \
        --adjust-vma=$vram "/tmp/$1.chunk" | tail -n +8 | awk '{$1=$1};1'; }
if diff <(d SLES_010.90) <(d SLES_010.90.bin) >/dev/null; then
  echo MATCH
else
  diff <(d SLES_010.90) <(d SLES_010.90.bin)
fi
