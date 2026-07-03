#!/usr/bin/env bash
# =============================================================================
# Analysis build (advisory) -- see tools/analysis/README.md
#
# Two independent tracks, NEITHER of which feeds the matching build or the
# produced bytes:
#
#   1. Layout asserts (HARD gate): regenerate per-header _Static_assert TUs from
#      the /* 0xNN */ + /* Size: */ comments and compile them under the PSX ABI.
#      A failure means a struct's documented layout no longer matches what the
#      compiler computes -> a real drift bug. Exits non-zero.
#
#   2. Warning sweep (ADVISORY): compile every src/*.c with -Wall -Wextra under
#      the same ABI, -fsyntax-only. In a faithful decomp most warnings are
#      "working as intended" (reproductions of the original's quirks), so this
#      never fails the build -- the product is the DELTA vs the checked-in
#      baseline (tools/analysis/warnings.baseline.txt).
#
# Requires: clang that can target mipsel (Apple clang or any LLVM works; no
# cross-gcc and no psyq/ headers needed -- common.h gates PSY-Q behind
# __has_include, and no PSX GPU type is used by value in the game structs).
# =============================================================================
set -u

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO"

CLANG="${CLANG:-clang}"
# -ferror-limit=0: don't stop at 20 diagnostics/file, otherwise the sweep is
# truncated and the baseline is unstable. -Wno-unknown-* keeps older clangs quiet.
TARGET_FLAGS=(-target mipsel-unknown-none-elf -std=gnu11 -fno-short-enums -ferror-limit=0)
INCLUDES=(-I include -I psyq -I tools/analysis/asserts)
ASSERT_DIR="tools/analysis/asserts"
BASELINE="tools/analysis/warnings.baseline.txt"

# Warnings tuned for a decomp: high-signal structural checks on, ABI/conversion
# noise off (a faithful reproduction trips those constantly and correctly).
#
# The -Wno-* suppressions below are structural to this decomp, not laziness:
#   implicit-function-declaration / deprecated-non-prototype: functions.h is
#     intentionally incomplete (many callees are still INCLUDE_ASM stubs).
#   incompatible-*-pointer-types / cast-function-type-mismatch: the engine's
#     vtable/callback dispatch reinterprets fn pointers by design.
#   sign-compare / conversion: pervasive and faithful to the original codegen.
# What survives (shadow, fallthrough, sometimes-uninitialized, return-type,
# unused-function) is the high-signal residue worth watching for regressions.
WARN_FLAGS=(
  -Wall -Wextra
  -Wshadow
  -Wimplicit-fallthrough
  -Wno-sign-compare
  -Wno-conversion
  -Wno-unused-parameter
  -Wno-unused-but-set-variable
  -Wno-missing-field-initializers
  -Wno-implicit-function-declaration
  -Wno-deprecated-non-prototype
  -Wno-incompatible-pointer-types
  -Wno-incompatible-function-pointer-types
  -Wno-cast-function-type-mismatch
  -Wno-incompatible-library-redeclaration
)

red()   { printf '\033[31m%s\033[0m\n' "$*"; }
green() { printf '\033[32m%s\033[0m\n' "$*"; }
bold()  { printf '\033[1m%s\033[0m\n' "$*"; }

MODE="${1:-check}"   # check | baseline

# ---------------------------------------------------------------------------
# Track 1: layout asserts (hard gate)
# ---------------------------------------------------------------------------
bold "== layout asserts =="
python3 tools/analysis/gen_layout_asserts.py >/dev/null || {
  red "generator failed"; exit 2; }

assert_fail=0
for tu in "$ASSERT_DIR"/*.asserts.c; do
  out="$("$CLANG" "${TARGET_FLAGS[@]}" "${INCLUDES[@]}" -fsyntax-only "$tu" 2>&1)"
  if [ $? -ne 0 ]; then
    assert_fail=$((assert_fail + 1))
    red "LAYOUT DRIFT: $(basename "$tu")"
    echo "$out" | grep -E 'static assertion|evaluates to' | sed 's/^/    /'
  fi
done
if [ "$assert_fail" -ne 0 ]; then
  red "$assert_fail header(s) have layout drift -- comments disagree with real layout"
else
  green "all layout asserts pass"
fi

# ---------------------------------------------------------------------------
# Track 2: warning sweep (advisory)
# ---------------------------------------------------------------------------
bold "== warning sweep (advisory) =="
tmp="$(mktemp)"
for c in src/*.c; do
  "$CLANG" "${TARGET_FLAGS[@]}" "${INCLUDES[@]}" "${WARN_FLAGS[@]}" \
    -D_LANGUAGE_C -fsyntax-only "$c" 2>>"$tmp"
done
# Normalize: keep only warning/error lines, strip absolute paths for stable diff.
grep -E 'warning:|error:' "$tmp" | sed "s#$REPO/##g" | sort > "$tmp.norm"
count="$(wc -l < "$tmp.norm" | tr -d ' ')"

if [ "$MODE" = "baseline" ]; then
  cp "$tmp.norm" "$BASELINE"
  green "wrote baseline: $count warnings -> $BASELINE"
  rm -f "$tmp" "$tmp.norm"
  exit "$([ "$assert_fail" -ne 0 ] && echo 1 || echo 0)"
fi

if [ -f "$BASELINE" ]; then
  new="$(comm -13 "$BASELINE" "$tmp.norm")"
  if [ -n "$new" ]; then
    red "NEW warnings vs baseline ($(echo "$new" | wc -l | tr -d ' ')):"
    echo "$new" | sed 's/^/    /'
  else
    green "no new warnings vs baseline ($count total, all baselined)"
  fi
else
  echo "  no baseline yet ($count warnings); run: make analyze-baseline"
fi

rm -f "$tmp" "$tmp.norm"
# Only the layout track gates; warnings are advisory.
exit "$([ "$assert_fail" -ne 0 ] && echo 1 || echo 0)"
