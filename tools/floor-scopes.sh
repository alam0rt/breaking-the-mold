#!/usr/bin/env bash
# Scope-focused sweep for func_80019F88. Hypothesis (per advisor): the
# v0/v1 swap and la reschedule are register-allocator side effects of
# variable *scopes* / lifetimes. Each variant moves the same set of
# statements between different lexical blocks to see if regalloc/scheduler
# decisions change.
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

# -----------------------------------------------------------------------------
# Baseline: local at function top, everything in one if-block.
# -----------------------------------------------------------------------------
V_BASE='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# -----------------------------------------------------------------------------
# Local moved INSIDE the if.
# -----------------------------------------------------------------------------
V_LOCAL_IN_IF='  if (desc->srcClut) {
    PaddedTickSlot local;
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# -----------------------------------------------------------------------------
# Local declared inside a NESTED block that only contains the slot writes.
# -----------------------------------------------------------------------------
V_LOCAL_NESTED='  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    {
      PaddedTickSlot local;
      local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
      desc->flag39 = flag;
      desc->tick = local.slot;
    }
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# -----------------------------------------------------------------------------
# Early-return inversion (flatten outer scope).
# -----------------------------------------------------------------------------
V_EARLY_RETURN='  PaddedTickSlot local;
  if (!desc->srcClut) return;
  desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
  desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
  local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
  desc->flag39 = flag; desc->tick = local.slot;
  if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
  *desc->workBuf = *desc->srcClut;'

# -----------------------------------------------------------------------------
# Early-return + local declared after the return.
# -----------------------------------------------------------------------------
V_EARLY_RETURN_LATE='  if (!desc->srcClut) return;
  {
    PaddedTickSlot local;
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# -----------------------------------------------------------------------------
# Workbuf path moved into its own nested scope (narrow lifetime).
# -----------------------------------------------------------------------------
V_WORKBUF_NESTED='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    {
      if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
      *desc->workBuf = *desc->srcClut;
    }
  }'

# -----------------------------------------------------------------------------
# Header writes in their own nested block.
# -----------------------------------------------------------------------------
V_HEADER_NESTED='  PaddedTickSlot local;
  if (desc->srcClut) {
    {
      desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
      desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    }
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# -----------------------------------------------------------------------------
# Marker block fully enscoped { local; markerLo/Hi/fn; flag39; copy }.
# -----------------------------------------------------------------------------
V_MARKER_BLOCK='  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    {
      PaddedTickSlot local;
      local.slot.markerLo = 0;
      local.slot.markerHi = -1;
      local.slot.fn = func_80019A14;
      desc->flag39 = flag;
      desc->tick = local.slot;
    }
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# -----------------------------------------------------------------------------
# Each marker assignment in its own block (extreme scoping).
# -----------------------------------------------------------------------------
V_MICRO_BLOCKS='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    { local.slot.markerLo = 0; }
    { local.slot.markerHi = -1; }
    { local.slot.fn = func_80019A14; }
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# -----------------------------------------------------------------------------
# Local shadowed: top-level local + inner-block "local" with marker assignments.
# -----------------------------------------------------------------------------
V_SHADOW='  PaddedTickSlot outerLocal;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    {
      TickSlot local;
      local.markerLo = 0; local.markerHi = -1; local.fn = func_80019A14;
      outerLocal.slot = local;
    }
    desc->flag39 = flag; desc->tick = outerLocal.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# -----------------------------------------------------------------------------
# Local at function top + for-1 wrapper around marker writes (forces block).
# -----------------------------------------------------------------------------
V_DO_WHILE='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    do {
      local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    } while (0);
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

# -----------------------------------------------------------------------------
# Local taken by address in a nested block (force-mem).
# -----------------------------------------------------------------------------
V_ADDRESSED='  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    {
      TickSlot *sl = &local.slot;
      sl->markerLo = 0; sl->markerHi = -1; sl->fn = func_80019A14;
    }
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }'

echo "func_80019F88 scope-shape sweep (lower = closer to target):"
run "baseline (local at top, single if)"                "$V_BASE"
run "local moved INSIDE if"                              "$V_LOCAL_IN_IF"
run "local in nested block around slot writes"           "$V_LOCAL_NESTED"
run "early-return (flatten scope)"                       "$V_EARLY_RETURN"
run "early-return + late local decl"                     "$V_EARLY_RETURN_LATE"
run "workbuf path in own nested block"                   "$V_WORKBUF_NESTED"
run "header writes in own nested block"                  "$V_HEADER_NESTED"
run "marker block fully enscoped"                        "$V_MARKER_BLOCK"
run "each marker in its own micro-block"                 "$V_MICRO_BLOCKS"
run "shadow: outer Padded, inner TickSlot"               "$V_SHADOW"
run "do{...}while(0) around markers"                     "$V_DO_WHILE"
run "address-taken sl pointer to local"                  "$V_ADDRESSED"
