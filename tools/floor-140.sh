#!/usr/bin/env bash
# Reproduces the 140-floor result for func_80019F88 (CLUT tick-slot install).
#
# Sweeps a few key source-shape variants and reports the score of each so it
# is easy to verify the 140 floor still holds after toolchain changes.
#
# Usage:  tools/floor-140.sh
#
# Background: see docs/compiler-quirks.md Quirk 5.
set -uo pipefail
cd "$(dirname "$0")/.."
WORK=$(mktemp -d)
trap "rm -rf $WORK" EXIT

cat > "$WORK/score.py" <<'PY'
import sys
sys.path.insert(0, "tools/decomp-permuter")
from src.scorer import Scorer
target, cand = sys.argv[1], sys.argv[2]
s = Scorer(target, stack_differences=False, algorithm="levenshtein",
           debug_mode=False, ign_branch_targets=True,
           objdump_command="mipsel-unknown-linux-gnu-objdump -dr")
print(s.score(cand)[0])
PY

build() {
  local label="$1" body="$2"
  cat > "$WORK/t.c" <<EOF
typedef signed short s16;
typedef signed int s32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef void (*TickCallback)(void);
typedef struct TickSlot { s16 markerLo; s16 markerHi; TickCallback fn; } TickSlot;
typedef struct PaddedTickSlot { s32 pad; TickSlot slot; } PaddedTickSlot;
typedef struct CLUT256 { u16 entries[256]; } CLUT256;
typedef struct CLUTEffectDesc {
  TickSlot tick; u8 pad08[0x14]; CLUT256 *srcClut; CLUT256 *workBuf; u32 targetClut;
  u8 pad28[6]; u16 totalFrames; u16 currentFrame; u16 intermediate;
  u8 pad34; u8 channelMask; u8 easing; u8 pad37; u8 flag38; u8 flag39;
} CLUTEffectDesc;
extern s32 D_800A5954[];
extern void func_80019A14(void);
extern void *func_800143F0(void *heap, s32 align, s32 size, s32 arg3);
void func_80019F88(CLUTEffectDesc *desc, u32 targetClut, u16 totalFrames, u8 flag, u8 channelMask, u8 easing) {
${body}
}
EOF
  cpp -nostdinc -D_LANGUAGE_C "$WORK/t.c" > "$WORK/t.i" 2>/dev/null
  tools/gcc-2.7.2-psx/cc1 -O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000 -o "$WORK/t.s" "$WORK/t.i" 2>/dev/null
  python3 tools/maspsx/maspsx.py --aspsx-version=2.86 --run-assembler \
    --gnu-as-path=mipsel-unknown-linux-gnu-as -G8 -o "$WORK/t.o" \
    -march=r3000 -mtune=r3000 -no-pad-sections -Iinclude "$WORK/t.s" 2>/dev/null
  local sc
  sc=$(python3 "$WORK/score.py" nonmatchings/func_80019F88-rand/target.o "$WORK/t.o" 2>/dev/null)
  printf "  %-55s score=%s\n" "$label" "${sc:-FAIL}"
}

BLOCK_140='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut;
    desc->totalFrames = totalFrames;
    desc->currentFrame = 0;
    desc->intermediate = 0;
    desc->channelMask = channelMask;
    desc->easing = easing;
    desc->flag38 = flag;
    local.slot.markerLo = 0;
    local.slot.markerHi = -1;
    local.slot.fn = func_80019A14;
    desc->flag39 = flag;
    desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

BLOCK_225='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut;
    desc->totalFrames = totalFrames;
    desc->currentFrame = 0;
    desc->intermediate = 0;
    desc->channelMask = channelMask;
    desc->easing = easing;
    desc->flag38 = flag;
    desc->flag39 = flag;
    local.slot.markerLo = 0;
    local.slot.markerHi = -1;
    local.slot.fn = func_80019A14;
    desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

BLOCK_420='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut;
    desc->totalFrames = totalFrames;
    desc->currentFrame = 0;
    desc->intermediate = 0;
    desc->channelMask = channelMask;
    desc->easing = easing;
    desc->flag38 = flag;
    desc->flag39 = flag;
    local.slot.markerHi = -1;
    local.slot.fn = func_80019A14;
    local.slot.markerLo = 0;
    desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

echo "func_80019F88 floor reproducer (lower = closer to target):"
build "lo,hi,fn + flag39 after markers (best=140)" "$BLOCK_140"
build "lo,hi,fn natural order (mid=225)"            "$BLOCK_225"
build "hi,fn,lo old @hack ordering (worst=420)"     "$BLOCK_420"
