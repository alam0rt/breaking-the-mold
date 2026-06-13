#!/usr/bin/env bash
# check-diff.sh
# When the built ROM no longer byte-matches the original, report *which*
# functions / source files diverged instead of just a SHA1 mismatch.
#
# It compares build/SLES_010.90.bin against bin/SLES_010.90, groups the
# differing bytes into contiguous regions, and uses the linker map
# (build/SLES_010.90.map) to attribute each region to the enclosing
# function symbol and the source object that produced it. For the first few
# regions it also prints an objdump side-by-side of the affected range.
#
# Run inside `nix develop` (needs mipsel-unknown-linux-gnu-objdump).
# Exit status: 0 if the binaries match, 1 if they differ (or inputs missing).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ORIG=bin/SLES_010.90
BUILT=build/SLES_010.90.bin
MAP=build/SLES_010.90.map

VRAM_BASE=0x80010000
ROM_HDR=0x800            # ROM offset = VRAM - VRAM_BASE + ROM_HDR
MAX_REGIONS_DISASM=${MAX_REGIONS_DISASM:-5}

for f in "$ORIG" "$BUILT"; do
  if [ ! -f "$f" ]; then
    echo "check-diff: missing $f" >&2
    exit 1
  fi
done

if cmp -s "$ORIG" "$BUILT"; then
  echo "check-diff: binaries are identical"
  exit 0
fi

# --- collect differing ROM offsets, grouped into contiguous regions ---------
# cmp -l prints 1-based byte offsets in decimal. Coalesce runs of consecutive
# offsets into "start length" pairs (ROM offsets, 0-based).
mapfile -t REGIONS < <(
  cmp -l "$ORIG" "$BUILT" 2>/dev/null | awk '
    { off = $1 - 1 }                       # 1-based -> 0-based ROM offset
    NR == 1 { start = off; prev = off; next }
    off == prev + 1 { prev = off; next }
    { print start, prev - start + 1; start = off; prev = off }
    END { if (NR) print start, prev - start + 1 }
  '
)

echo "check-diff: ✗ $BUILT differs from $ORIG"
echo "check-diff: ${#REGIONS[@]} differing region(s)"
echo

if [ ! -f "$MAP" ]; then
  echo "check-diff: no $MAP — cannot attribute regions to symbols." >&2
  printf '  %-12s %-10s\n' "ROM_OFF" "LEN"
  for r in "${REGIONS[@]}"; do
    set -- $r
    printf '  0x%-10x %-10d\n' "$1" "$2"
  done
  exit 1
fi

# --- parse the linker map ----------------------------------------------------
# SYMS:  "<hexaddr> <name>"      (function/label addresses)
# FILES: "<hexstart> <hexsize> <objpath>"  (per-object .text/.rodata ranges)
SYMS=$(mktemp); FILES=$(mktemp)
CHUNK_PREFIX=$(mktemp -u)        # unique per-run prefix for dd chunks
trap 'rm -f "$SYMS" "$FILES" "$CHUNK_PREFIX".*.chunk' EXIT

awk '
  # object section line: ".text  0x8001....  0x....  build/....o"
  /^[[:space:]]*\.[A-Za-z._]+[[:space:]]+0x8[0-9a-f]{7}[[:space:]]+0x[0-9a-f]+[[:space:]]+build\// {
    print $2, $3, $4 > "'"$FILES"'"
    next
  }
  # bare symbol line: "0x8001....  Name"  (skip "= ." assignments, NON_MATCHING)
  /^[[:space:]]*0x8[0-9a-f]{7}[[:space:]]+[A-Za-z_]/ {
    if ($0 ~ /=/) next
    if ($2 ~ /\.NON_MATCHING$/) next
    print strtonum($1), $2 > "'"$SYMS"'"
  }
' "$MAP"

sort -n -k1,1 "$SYMS" -o "$SYMS"

# obj path (build/src/...o / build/asm/...o) -> source file
obj_to_src() {
  local o="$1"
  o="${o#build/}"
  case "$o" in
    src/*) echo "${o%.o}.c" ;;
    asm/*) echo "${o%.o}.s" ;;
    *)     echo "$o" ;;
  esac
}

# greatest symbol whose addr <= target
sym_for() {
  local v=$1
  awk -v v="$v" '$1 <= v { name=$2; addr=$1 } END { if (name) printf "%s+0x%x", name, v-addr }' "$SYMS"
}

# object file range containing target
file_for() {
  local v=$1
  awk -v v="$v" '
    { start=strtonum($1); size=strtonum($2);
      if (size>0 && v>=start && v<start+size) { print $3; exit } }
  ' "$FILES"
}

printf '  %-12s %-6s %-28s %s\n' "ROM_OFF" "LEN" "FUNCTION" "SOURCE"
printf '  %-12s %-6s %-28s %s\n' "-------" "---" "--------" "------"
for r in "${REGIONS[@]}"; do
  set -- $r
  off=$1; len=$2
  vram=$(( off - ROM_HDR + VRAM_BASE ))
  fn=$(sym_for "$vram"); [ -n "$fn" ] || fn="(?)"
  obj=$(file_for "$vram")
  src="(?)"; [ -n "$obj" ] && src=$(obj_to_src "$obj")
  printf '  0x%-10x %-6d %-28s %s\n' "$off" "$len" "$fn" "$src"
done
echo

# --- disassembly diff for the first few regions ------------------------------
disasm() { # file vram size
  mipsel-unknown-linux-gnu-objdump -D -b binary -m mips:3000 -EL \
    --adjust-vma="$2" "$1" | tail -n +8 | awk '{$1=$1};1'
}

i=0
for r in "${REGIONS[@]}"; do
  [ "$i" -ge "$MAX_REGIONS_DISASM" ] && { echo "… more regions omitted (set MAX_REGIONS_DISASM higher)"; break; }
  set -- $r
  off=$1; len=$2
  vram=$(( off - ROM_HDR + VRAM_BASE ))
  # pad the window so the diff has surrounding context / whole instructions
  pad=16
  woff=$(( off > pad ? off - pad : 0 ))
  wlen=$(( len + 2*pad ))
  wvram=$(( woff - ROM_HDR + VRAM_BASE ))
  fn=$(sym_for "$vram"); [ -n "$fn" ] || fn="(?)"
  echo "=== region @ 0x$(printf '%x' "$off") ($len bytes) — $fn ==="
  for f in "$ORIG" "$BUILT"; do
    dd if="$f" of="$CHUNK_PREFIX.$(basename "$f").chunk" bs=1 skip="$woff" count="$wlen" 2>/dev/null
  done
  diff <(disasm "$CHUNK_PREFIX.$(basename "$ORIG").chunk" "$wvram" "$wlen") \
       <(disasm "$CHUNK_PREFIX.$(basename "$BUILT").chunk" "$wvram" "$wlen") \
    || true
  echo
  i=$((i+1))
done

exit 1
