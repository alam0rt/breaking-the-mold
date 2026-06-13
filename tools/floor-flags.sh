#!/usr/bin/env bash
# Try the 140-floor source with various compiler-flag tweaks. The advisor's
# theory is that the lui/addiu reschedule (which causes the v0/v1 swap) is
# a scheduler decision; if so, toggling -fschedule-insns / -fschedule-insns2
# or trying alternate optimization levels should expose it.
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

cat > "$WORK/t.c" <<'EOF'
typedef signed short s16; typedef signed int s32; typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef void (*TickCallback)(void);
typedef struct TickSlot { s16 markerLo; s16 markerHi; TickCallback fn; } TickSlot;
typedef struct PaddedTickSlot { s32 pad; TickSlot slot; } PaddedTickSlot;
typedef struct CLUT256 { u16 entries[256]; } CLUT256;
typedef struct CLUTEffectDesc { TickSlot tick; u8 pad08[0x14]; CLUT256 *srcClut; CLUT256 *workBuf; u32 targetClut;
  u8 pad28[6]; u16 totalFrames; u16 currentFrame; u16 intermediate; u8 pad34; u8 channelMask; u8 easing; u8 pad37; u8 flag38; u8 flag39;
} CLUTEffectDesc;
extern s32 D_800A5954[]; extern void func_80019A14(void); extern void *func_800143F0(void *heap, s32 align, s32 size, s32 arg3);
void func_80019F88(CLUTEffectDesc *desc, u32 targetClut, u16 totalFrames, u8 flag, u8 channelMask, u8 easing) {
  PaddedTickSlot local;
  if (desc->srcClut) {
    desc->targetClut = targetClut; desc->totalFrames = totalFrames; desc->currentFrame = 0; desc->intermediate = 0;
    desc->channelMask = channelMask; desc->easing = easing; desc->flag38 = flag;
    local.slot.markerLo = 0; local.slot.markerHi = -1; local.slot.fn = func_80019A14;
    desc->flag39 = flag; desc->tick = local.slot;
    if (desc->workBuf == 0) desc->workBuf = func_800143F0((void*)D_800A5954[0], 2, 0x100, 0);
    *desc->workBuf = *desc->srcClut;
  }
}
EOF
cpp -nostdinc -D_LANGUAGE_C "$WORK/t.c" > "$WORK/t.i" 2>/dev/null

build() {
  local label="$1" flags="$2"
  tools/gcc-2.7.2-psx/cc1 $flags -mno-abicalls -mcpu=3000 -o "$WORK/t.s" "$WORK/t.i" 2>/dev/null || { printf "  %-55s CC-FAIL\n" "$label"; return; }
  python3 tools/maspsx/maspsx.py --aspsx-version=2.86 --run-assembler \
    --gnu-as-path=mipsel-unknown-linux-gnu-as -G8 -o "$WORK/t.o" \
    -march=r3000 -mtune=r3000 -no-pad-sections -Iinclude "$WORK/t.s" 2>/dev/null || { printf "  %-55s AS-FAIL\n" "$label"; return; }
  local sc
  sc=$(python3 "$WORK/score.py" nonmatchings/func_80019F88-rand/target.o "$WORK/t.o" 2>/dev/null)
  printf "  %-55s score=%s\n" "$label" "${sc:-FAIL}"
}

echo "func_80019F88 compiler-flag sweep (baseline 140-shape source):"
build "baseline -O2 -G8 -fno-builtin"                      "-O2 -G8 -fno-builtin"
build "-O2 -G8 -fno-builtin -fno-schedule-insns"           "-O2 -G8 -fno-builtin -fno-schedule-insns"
build "-O2 -G8 -fno-builtin -fno-schedule-insns2"          "-O2 -G8 -fno-builtin -fno-schedule-insns2"
build "-O2 -G8 -fno-builtin -fno-schedule-insns{,2}"       "-O2 -G8 -fno-builtin -fno-schedule-insns -fno-schedule-insns2"
build "-O2 -G8 -fno-builtin -fno-delayed-branch"           "-O2 -G8 -fno-builtin -fno-delayed-branch"
build "-O2 -G8 -fno-builtin -fno-peephole"                 "-O2 -G8 -fno-builtin -fno-peephole"
build "-O2 -G8 -fno-builtin -fno-strength-reduce"          "-O2 -G8 -fno-builtin -fno-strength-reduce"
build "-O2 -G8 -fno-builtin -fno-function-cse"             "-O2 -G8 -fno-builtin -fno-function-cse"
build "-O2 -G8 -fno-builtin -fforce-mem"                   "-O2 -G8 -fno-builtin -fforce-mem"
build "-O2 -G8 -fno-builtin -fforce-addr"                  "-O2 -G8 -fno-builtin -fforce-addr"
build "-O1 -G8 -fno-builtin"                               "-O1 -G8 -fno-builtin"
build "-Os -G8 -fno-builtin"                               "-Os -G8 -fno-builtin"
build "-O2 -G8 -fno-builtin -mips1"                        "-O2 -G8 -fno-builtin -mips1"
build "-O2 -G8 -fno-builtin -msoft-float"                  "-O2 -G8 -fno-builtin -msoft-float"
