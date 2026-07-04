---
title: "Function families — exemplar-sweep matching queue"
category: analysis
tags: [matching, clustering, mipsmatch, families]
generated: 2026-07-04
---

# Function families — exemplar-sweep matching queue

Structural clusters among the **723 still-shelved** functions, for the
"match one exemplar the hard way, then sweep the rest" strategy. Regenerate as
matching progresses (families shrink as members land):

```bash
python3 tools/analysis/cluster_families.py          # console
python3 tools/analysis/cluster_families.py --md      # markdown table (this doc)
```

## Two clustering methods (and what each is good for)

**Strict — `mipsmatch` exact fingerprint.** Byte-identical bodies modulo masked
global addresses (Rabin-Karp over instruction bytes; only operands that could be
global refs are masked). Members are *verbatim* duplicates: the matched C of one
member matches every other member with only global symbol/constant refs changed.
Needs `build/<PROJECT>.{map,elf}` and a built `tools/mipsmatch`:

```bash
(cd tools/mipsmatch && cargo build --release)
tools/mipsmatch/target/release/mipsmatch \
    --output /tmp/fp.yaml fingerprint build/SLES_010.90.map build/SLES_010.90.elf
# then group functions by identical `fingerprint:` value
```

**Skeleton — opcode-sequence hash (`tools/analysis/cluster_families.py`).** Each
instruction reduced to *mnemonic + register-operand shape* with immediates,
symbols, and offsets blanked, then hashed as a sequence. Catches "same opcode
sequence, different constants" twins the strict pass misses. This is the default;
it needs no build inputs (reads `asm/nonmatchings/*.s`, which are exactly the
shelved functions).

Both passes are **conservative** — they require the instruction sequence to align
position-for-position. They find *structural twins*, not merely *behaviourally
similar* functions.

## Headline findings

1. **The exemplar-sweep opportunity is concentrated in `playst` and `menu`, not
   the enemy/effect/boss callback tiers.** 20 of the 45 clustered game functions
   are player-state machine functions written from a shared template. The
   enemies/effects/bosses callbacks are behaviourally similar but *not*
   structurally twin at the instruction level — they diverge in branch count and
   field access, so exemplar C does **not** mechanically transfer to them. Match
   those the normal way; do not expect a sweep.
2. **`libcd` is riddled with internal duplication** (21+ strict-fingerprint
   families, mostly tiny thunks; `strcmp`-shaped bodies appear ×25). This is
   PSY-Q 4.0 library code — the right move is to **port** reference libcd
   implementations, not decompile each copy. See the toolchain note in
   `docs/compiler-quirks.md` (libraries pinned to PSY-Q 4.0).
3. **Verified twin (workflow proof).** `PlayerState_WalkingRight` and
   `PlayerState_WalkingLeft` (0x160, 88 instrs) have an **empty** normalized-
   skeleton diff. They differ only in: two callback symbols
   (`PlayerCallback_EventHandlerWithQueue` ↔ `PlayerCallback_WalkToRunTransition`),
   one 32-bit asset-hash constant (`0x292E8480` ↔ `0x18298210`, an opaque
   sprite/anim id — see the asset-hash note), and one state-transition target
   (`PlayerState_Falling` ↔ `PlayerStateInit_Idle`). Matching either one yields
   the twin by swapping four literals.

## Ranked queue (skeleton pass, game code)

`payoff` = members × instruction-count (total mechanical work unlocked). All
members below are currently shelved (`INCLUDE_ASM`).

<!-- BEGIN generated table: tools/analysis/cluster_families.py --md -->
| payoff | instrs | members | segments | functions (exemplar first) |
|-------:|-------:|--------:|----------|----------------------------|
| 348 | 87 | 4 | playst×4 | PlayerEnterLandingState, PlayerStateInit_Falling, PlayerStateInit_TeleportLanding, PlayerState_IdleLookAround |
| 342 | 114 | 3 | enemies×1,player×1,vehicle×1 | CalcEntityYFromTileHeight, PlayerApplyPositionWithCollision, CalcEntityYFromTileCollision |
| 226 | 113 | 2 | anim×2 | UpdateParallaxScrollWithWrap_Medium, UpdateParallaxScrollWithWrap_Small |
| 218 | 109 | 2 | player×1,vehicle×1 | IsEntityNearSoundTrigger, CheckEntityNearTileY |
| 210 | 105 | 2 | decor×1,enemies×1 | UpdateDecorEntityTriggerColors, UpdateCollectibleTriggerZone |
| 192 | 96 | 2 | playst×2 | PlayerStateInit_TeleportFalling, PlayerStateInit_TeleportWalking |
| 192 | 96 | 2 | playst×2 | PlayerStateInit_GrowFromShrink, PlayerStateInit_ShrinkAndFall |
| 192 | 96 | 2 | entity×2 | EntityBroadcastPointCollision, CheckEntityPointCollisionWithOffsets |
| 190 | 95 | 2 | pickups×2 | TriggerCollectible100CTickCallback, CollectibleScaleResetTickCallback |
| 188 | 94 | 2 | bosses×2 | CollectibleRiseState, CollectibleFloatState |
| 176 | 88 | 2 | playst×2 | PlayerState_WalkingRight, PlayerState_WalkingLeft |
| 176 | 88 | 2 | playst×2 | PlayerSetupIdleState, PlayerStateInit_FallFromLedge |
| 170 | 85 | 2 | playst×2 | PlayerStateInit_BounceJumpAnimation, PlayerStateInit_BounceActive |
| 156 | 78 | 2 | playst×2 | PlayerState_SpecialMove1, PlayerStateInit_TeleportEntry |
| 148 | 74 | 2 | playst×2 | PlayerStateInit_FallingWithInput, PlayerCallback_SetIdleStateCallbacks |
| 138 | 69 | 2 | playst×2 | PlayerEvent_ZoneTriggerHandlerV2, PlayerCallback_BaseEventHandler |
| 129 | 43 | 3 | menu×3 | MenuActivateSkullIconButton, MenuActivateButtonWithReset, MenuActivateLevelSelectButton |
| 94 | 47 | 2 | sprite×2 | FreeResourceType2, FreeMultiAllocResource |
| 75 | 25 | 3 | menu×3 | MenuDeactivateSkullIconButton, MenuDeactivateLevelSelectButton, FINN_ClearSubentityState |
| 64 | 32 | 2 | blbacc×2 | GetCurrentSectorCount, GetCurrentSectorOffset |

skeleton families (>=2 members, >=12 instrs): 20
shelved funcs inside families: 45 / 723
<!-- END generated table -->

## How to sweep a family

1. Pick the **top unstarted family**. Confirm the twin is real:
   ```bash
   norm() { sed -E 's#^.*\*/\s+##; s/\$[0-9a-z]+/$R/g; s/-?0x[0-9a-fA-F]+/#/g; s/\b[A-Za-z_][A-Za-z0-9_]{2,}\b/S/g' "$1" \
            | grep -vE '^(glabel|endlabel|nonmatching|\.)'; }
   diff <(norm asm/nonmatchings/<seg>/<A>.s) <(norm asm/nonmatchings/<seg>/<B>.s)
   ```
   An empty diff ⇒ pure exemplar-sweep. A small diff ⇒ still a strong base, but
   the members need per-member edits at the divergent instructions.
2. Match the **exemplar** the hard way (`/decompile`; consult
   `docs/matching-idioms.md` for the residual). playst state functions are the
   FSM-slot installer family — expect the `do {} while (0)` fence + marker-order
   idioms (compiler-quirks.md Quirk 6k) and, for the coloring walls, the
   hard-reg-pin cse-across-call barrier.
3. For each remaining member, copy the exemplar's C and swap only the differing
   literals (callback symbols, asset-hash constants, state targets). `fdiff` each;
   `make check` after the batch.

## Worked example: PlayerState_WalkingRight/_WalkingLeft (2026-07-04)

First exemplar-sweep attempt on the verified twin pair. Outcome refines the
strategy:

- The C body is **fully recovered and verified** — every real instruction
  matches (slot-install body, the `interactEntity` render conditional, the
  `SetEntitySpriteId` call, the trailing fall-slot). Load-bearing idioms found:
  `markerHi=-1` pinned to `$s1` (survives the sprite call for the fall slot),
  a register temp for the render conditional (one store not two), per-slot
  `do{}while(0)` fences (stop store batching), `PadSlot` scratch/temp, and a
  `volatile s32 pad` to reach frame size 0x68.
- **But the exemplar does not cleanly byte-match**: the same stack-frame slot-
  origin coin-flip (+4 leading offset) that shelves the siblings
  `PlayerState_IdleLookAround` and `PlayerStateInit_FallingWithInput`. Frame
  *size* is reachable; the +4 slot origin is not (tried pad decl first/last,
  `PadSlot` on the first slot, six layout combinations). It's a decomp-permuter
  job. Both functions stay shelved with the verified C (see src/playst.c).

**Lesson for the queue:** a structural-twin cluster only becomes clean *matches*
if its exemplar is itself fully matchable. This whole playst FSM-state family
(frame 0x68 installers) shares one frame coin-flip, so it yields **verified-C
shelves** until the permuter cracks the frame — at which point the fix likely
transfers across the family at once. Prefer clusters whose exemplar has no known
frame residual for the first clean sweeps; batch the playst FSM family behind a
single permuter push on the frame layout.

## Caveats

- **Strict-pass duplicates in `libcd`/`libc`** (`strcmp`×25, etc.) are best
  handled by the libcd porting effort, not this queue.
- These are *structural* twins. Two functions with the same skeleton can still
  need different armor if surrounding register pressure differs — re-verify every
  member under a clean build, never trust a stale incremental MATCH.
- Regenerate after each sweep; the ranking shifts as families empty out.
