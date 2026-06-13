#!/usr/bin/env bash
# Sub-140 attempts: keep v0=-1 alive past the marker stores so cc1 is forced
# to materialize `la func_80019A14` into v1 (the target's choice) instead of
# stealing v0. Each variant introduces a downstream use of the value -1 or
# of markerHi that keeps v0 pinned.
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

PRELUDE='typedef signed short s16; typedef signed int s32; typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef void (*TickCallback)(void);
typedef struct TickSlot { s16 markerLo; s16 markerHi; TickCallback fn; } TickSlot;
typedef struct PaddedTickSlot { s32 pad; TickSlot slot; } PaddedTickSlot;
typedef struct CLUT256 { u16 entries[256]; } CLUT256;
typedef struct CLUTEffectDesc { TickSlot tick; u8 pad08[0x14]; CLUT256 *srcClut; CLUT256 *workBuf; u32 targetClut;
  u8 pad28[6]; u16 totalFrames; u16 currentFrame; u16 intermediate; u8 pad34; u8 channelMask; u8 easing; u8 pad37; u8 flag38; u8 flag39;
} CLUTEffectDesc;
extern s32 D_800A5954[]; extern void func_80019A14(void); extern void *func_800143F0(void *heap, s32 align, s32 size, s32 arg3);
void func_80019F88(CLUTEffectDesc *desc, u32 targetClut, u16 totalFrames, u8 flag, u8 channelMask, u8 easing) {'

run() {
  local label="$1" body="$2"
  cat > "$WORK/t.c" <<EOF
$PRELUDE
$body
}
EOF
  cpp -nostdinc -D_LANGUAGE_C "$WORK/t.c" > "$WORK/t.i" 2>/dev/null || { printf "  %-55s CPP-FAIL\n" "$label"; return; }
  tools/gcc-2.7.2-psx/cc1 -O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000 -o "$WORK/t.s" "$WORK/t.i" 2>/dev/null || { printf "  %-55s CC-FAIL\n" "$label"; return; }
  python3 tools/maspsx/maspsx.py --aspsx-version=2.86 --run-assembler \
    --gnu-as-path=mipsel-unknown-linux-gnu-as -G8 -o "$WORK/t.o" \
    -march=r3000 -mtune=r3000 -no-pad-sections -Iinclude "$WORK/t.s" 2>/dev/null || { printf "  %-55s AS-FAIL\n" "$label"; return; }
  local sc
  sc=$(python3 "$WORK/score.py" nonmatchings/func_80019F88-rand/target.o "$WORK/t.o" 2>/dev/null)
  printf "  %-55s score=%s\n" "$label" "${sc:-FAIL}"
}

# Baseline reference.
V_BASE='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Capture -1 in a named local and use it for markerHi.
V_HOIST_M1='  PaddedTickSlot local;
  if (desc->srcClut) {
    s16 negone = -1;
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = negone; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Use markerHi value again after the slot is written so the regalloc keeps it.
V_REREAD='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = (u8)(flag & (u8)(local.slot.markerHi & 0xFF));
    desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Initialize PaddedTickSlot.pad to -1 to introduce another -1 user.
V_PAD_M1='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.pad = -1;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Write 0xFFFF0000 as a single sw via union (collapses sh+sh).
V_UNION_SW='  union { u32 w; struct { s16 lo, hi; } h; } u;
  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    u.h.lo = 0; u.h.hi = -1; *(u32*)&local.slot.markerLo = u.w;
    local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Put markerHi into channelMask path (use -1 twice) — keeps -1 live longer.
V_M1_TWICE='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag;
    desc->tick = local.slot;
    desc->tick.markerHi = -1;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Re-store markerHi after the tick copy — forces -1 to stay live.
V_STORE_AGAIN='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->tick = local.slot;
    desc->tick.markerHi = local.slot.markerHi;
    desc->flag39 = flag;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# A fully ternary'd flag39 derived from markerHi result.
V_F39_DERIVED='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->tick = local.slot;
    desc->flag39 = (local.slot.markerHi < 0) ? flag : 0;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Force-mem the function pointer.
V_FUNC_PTR_GLOBAL='  static TickCallback const kFn = func_80019A14;
  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = kFn;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

echo "func_80019F88 sub-140 attempts (v0=-1 liveness pinning):"
run "baseline (140)"                              "$V_BASE"
run "negone local hoist"                          "$V_HOIST_M1"
run "flag39 derived from markerHi"                "$V_REREAD"
run "PaddedTickSlot.pad = -1"                     "$V_PAD_M1"
run "single sw via union"                         "$V_UNION_SW"
run "rewrite markerHi after tick copy"            "$V_M1_TWICE"
run "store markerHi from local after copy"        "$V_STORE_AGAIN"
run "flag39 = (markerHi < 0) ? flag : 0"          "$V_F39_DERIVED"
run "fn via static const local"                   "$V_FUNC_PTR_GLOBAL"
