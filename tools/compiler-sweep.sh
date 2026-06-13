#!/usr/bin/env bash
# Try every installed old-gcc / gcc-*-psx compiler on a given source file
# and report the permuter score against a target .o.
#
# Usage:
#   tools/compiler-sweep.sh SOURCE.c TARGET.o
#
# Optional env:
#   CFLAGS   extra flags forwarded to cc1 (after -O2 -G8 etc.)
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "usage: $0 SOURCE.c TARGET.o" >&2
    exit 2
fi

SRC="$(realpath "$1")"
TARGET="$(realpath "$2")"
EXTRA_CFLAGS="${CFLAGS:-}"

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# Use the permuter scorer directly for proper scoring.
SCORER_PY="$WORK/score.py"
cat > "$SCORER_PY" <<'PY'
import sys, os
sys.path.insert(0, "tools/decomp-permuter")
try:
    import toml  # noqa: F401
except ImportError:
    print("?:no-toml", file=sys.stderr); sys.exit(2)
from src.scorer import Scorer
target = sys.argv[1]
cand   = sys.argv[2]
s = Scorer(target, stack_differences=False, algorithm="levenshtein",
           debug_mode=False, ign_branch_targets=True,
           objdump_command="mipsel-unknown-linux-gnu-objdump -dr")
score, _ = s.score(cand)
print(score)
PY

# Pick a python with `toml` available. Prefer `nix shell` (no-op if already in
# the nix env), fall back to a plain python3 the caller pre-arranged.
PY_RUNNER=(python3)
if ! python3 -c 'import toml' 2>/dev/null; then
    if command -v nix-shell >/dev/null; then
        PY_RUNNER=(nix-shell -p python3Packages.toml --run "python3")
    fi
fi

printf "%-20s %8s %s\n" "compiler" "score" "notes"
printf "%-20s %8s %s\n" "--------" "-----" "-----"

for d in tools/gcc-*/; do
    name="$(basename "$d")"
    cc1="$d/cc1"
    [[ -x "$cc1" ]] || continue

    obj="$WORK/$name.o"
    asm="$WORK/$name.s"
    log="$WORK/$name.log"

    # Same flags as tools/permuter_cc1_maspsx.sh.
    if ! cpp -I include -I psyq -D_LANGUAGE_C "$SRC" 2>/dev/null \
         | "$cc1" -O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000 $EXTRA_CFLAGS \
                  -o "$asm" 2>"$log"; then
        printf "%-20s %8s %s\n" "$name" "ERR" "cc1 failed"
        continue
    fi
    if ! python3 tools/maspsx/maspsx.py --aspsx-version=2.86 --run-assembler \
            --gnu-as-path=mipsel-unknown-linux-gnu-as -G8 -o "$obj" \
            -march=r3000 -mtune=r3000 -no-pad-sections -G8 -Iinclude \
            "$asm" >>"$log" 2>&1; then
        printf "%-20s %8s %s\n" "$name" "ERR" "maspsx failed"
        continue
    fi

    score="$("${PY_RUNNER[@]}" "$SCORER_PY" "$TARGET" "$obj" 2>&1 || true)"
    printf "%-20s %8s\n" "$name" "$score"
done
