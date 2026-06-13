#!/usr/bin/env bash
# Liveness-driven floor-search for func_80019F88.
#
# The current 140 floor comes down to gcc scheduling `la func_80019A14`
# (lui+addiu) BETWEEN the two sh stores for markerLo/markerHi, where the
# target instead materializes the address first:
#
#   target:  lui/addiu, sh markerLo, sh markerHi, sw fn
#   ours:    sh markerLo, lui/addiu, sh markerHi, sw fn
#
# The recommended angle of attack is liveness: change *when* the function
# pointer first becomes live so the scheduler is forced to hoist the
# materialization above the sh stores. This script enumerates several
# variants where the func_80019A14 address is computed at different
# program points (locals, casts, struct-init shorthand, etc.) and prints
# the resulting score plus the relevant la/sh window from the disassembly.
#
# Usage:  tools/floor-liveness.sh [VARIANT...]
#         (no args -> run all)

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

PRELUDE='typedef signed short s16;
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
  if [[ "${DUMP:-}" == "1" ]]; then
    mipsel-unknown-linux-gnu-objdump -dr "$WORK/t.o" \
      | awk '/<func_80019F88>:/{p=1} p{print} /jr.*ra/&&p{exit}' \
      | grep -E 'lui|addiu.*func_80019A14|^\s+[0-9a-f]+:.*sh |sw .*func_80019A14|sh .*0x[01]\(' \
      | head -10
    echo "  ---"
  fi
}

# ---- variant table ----
# Each block is a complete function body. The marker assignments differ in
# how the fn pointer is materialized; everything else (flag39 placement,
# desc->tick copy, workBuf path) is held constant at the 140-floor shape.

V_140='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Hoist fn pointer into a local declared at function top.
V_FN_TOP='  PaddedTickSlot local; TickCallback fn = func_80019A14;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = fn;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Hoist fn pointer just before the marker block (inside the if).
V_FN_IF='  PaddedTickSlot local;
  if (desc->srcClut) {
    TickCallback fn = func_80019A14;
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = fn;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Hoist fn into a local right before markers.
V_FN_NEAR='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    { TickCallback fn = func_80019A14;
      local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = fn; }
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Store fn FIRST then markers (lets gcc see the address use first).
V_FN_FIRST='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.fn = func_80019A14; local.slot.markerLo = 0; local.slot.markerHi = -1;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Struct initializer assignment.
V_INIT='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    { TickSlot tmp = { 0, -1, func_80019A14 }; local.slot = tmp; }
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Take address into a void* and cast, forcing earlier use.
V_VOIDP='  PaddedTickSlot local;
  if (desc->srcClut) {
    void *fnp = (void*)func_80019A14;
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = (TickCallback)fnp;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Combine: fn local + flag39 before markers (old 225 shape but with hoist).
V_FN_TOP_F39_BEFORE='  PaddedTickSlot local; TickCallback fn = func_80019A14;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing;
    desc->flag38 = flag; desc->flag39 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = fn;
    desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Both flags via fn local + flag39-after pattern from 140 floor.
V_PTR_BOUNCE='  PaddedTickSlot local;
  TickSlot *sp = &local.slot;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    sp->markerLo = 0; sp->markerHi = -1; sp->fn = func_80019A14;
    desc->flag39 = flag; desc->tick = *sp;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

echo "func_80019F88 liveness sweep (lower = better; target=0):"
run "baseline (best=140)"                "$V_140"
run "fn local at func top"               "$V_FN_TOP"
run "fn local inside if"                 "$V_FN_IF"
run "fn local in nested block"           "$V_FN_NEAR"
run "fn assigned FIRST in slot"          "$V_FN_FIRST"
run "struct initializer"                 "$V_INIT"
run "void* cast bounce"                  "$V_VOIDP"
run "fn local + flag39 before markers"   "$V_FN_TOP_F39_BEFORE"
run "TickSlot* pointer bounce"           "$V_PTR_BOUNCE"

# --- newer variants chasing the "keep sb flag39 with the flag38/sb group" theory ---

V_F39_BEFORE_HI='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0;
    desc->flag39 = flag;
    local.slot.markerHi = -1;
    local.slot.fn = func_80019A14;
    desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

V_F39_AFTER_F38='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing;
    desc->flag38 = flag; desc->flag39 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Same as 140 but write desc->tick fields directly (no local).
V_DIRECT_DESC='  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    desc->tick.markerLo = 0; desc->tick.markerHi = -1; desc->tick.fn = func_80019A14;
    desc->flag39 = flag;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Use a TickSlot local (not PaddedTickSlot) and assign directly.
V_NO_PAD='  TickSlot localslot;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    localslot.markerLo = 0; localslot.markerHi = -1; localslot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = localslot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# flag39 grouped with flag38 + write markers via desc->tick.
V_GROUP_FLAGS_DIRECT='  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing;
    desc->flag38 = flag; desc->flag39 = flag;
    desc->tick.markerLo = 0; desc->tick.markerHi = -1; desc->tick.fn = func_80019A14;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# Volatile markerHi forces a strict-order store.
V_VOLATILE='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames;
    desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0;
    *(volatile s16*)&local.slot.markerHi = -1;
    local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

echo
echo "--- second wave (scheduler/v0-liveness hacks) ---"
run "flag39 between markerLo and markerHi"    "$V_F39_BEFORE_HI"
run "flag39 immediately after flag38"          "$V_F39_AFTER_F38"
run "direct desc->tick writes + flag39 after"  "$V_DIRECT_DESC"
run "TickSlot local (no pad)"                  "$V_NO_PAD"
run "flag38+39 grouped + direct desc->tick"    "$V_GROUP_FLAGS_DIRECT"
run "volatile markerHi"                        "$V_VOLATILE"
