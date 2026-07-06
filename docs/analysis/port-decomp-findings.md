---
title: "Port-Decomp Findings (functional-C harvest)"
category: analysis
tags: [analysis, port, structs, unconfirmed]
---

# Port-Decomp Findings

Knowledge harvested from `port/decomp/` — the PC-port's functional-C
reconstructions of engine functions that are **still `INCLUDE_ASM`** (not
byte-matched) in `src/`. Each port file carries a header (RAM address, byte
size, algorithm) plus inline struct-offset semantics, and several are
runtime-validated (the port boots the menu, title screen, and level-1 with the
player avatar).

> **Status / clean-room caveat.** These are *reconstructions of not-yet-matched
> functions*, so they are strong hypotheses — many trace-validated — not ground
> truth. Symbol names are inferred. Where a fact is marked **high** confidence it
> is corroborated by a second independent source (`symbol_addrs.txt`,
> `include/Game/*.h`, or agreement across multiple port files). Verify against
> disassembly/runtime before relying on any single item for a byte-match.
>
> **Do not trust port-header addresses.** Several port file headers cite
> addresses that disagree with `symbol_addrs.txt` (the ground truth) — see
> [Port-header address errors](#port-header-address-errors). All addresses in
> this document use the `symbol_addrs.txt` values.
>
> **Scope:** documentation only. Nothing here has been folded into `src/`; the
> functions remain `INCLUDE_ASM`. `port/decomp` is the migration staging ground,
> and moving a body to `src/` still requires it to pass `make diff` / the
> permuter to preserve the `make check` byte-match.

Conflicts these files exposed in authoritative docs have already been applied
(see [Applied corrections](#applied-corrections-conflicts-already-fixed)). This
document holds the **net-new structural maps** that were absent from docs.

---

## Data structures (net-new)

### Sprite render context (0x3C bytes)

Resolves what [header-smell-audit-2026-07.md](header-smell-audit-2026-07.md)
called "3 divergent partial views of one struct." Two independent port files
(`sprite/PrepareSpriteVRAMSlotForContext.c`, `sprite/RenderSpriteOrScaledQuad.c`)
agree. Confidence: **high**.

| Off | Field | Notes |
|-----|-------|-------|
| +0x04 | width (u16) | |
| +0x06 | height (u16) | |
| +0x08 | zOrder | |
| +0x0A | active | =1 |
| +0x0C | vtable | `D_80010384` |
| +0x10/+0x12 | VRAM slot x/y | |
| +0x18/+0x1A | CLUT slot x/y | |
| +0x1C/+0x20 | scaleX/Y | =0x10000 (1.0, 16.16) |
| +0x24 | tpage | |
| +0x26 | clut id | |
| +0x28 | rotation **or** shadowFlag | dual-use / one name wrong — see note |
| +0x2A/+0x2C | pivot x/y **or** shadow-coord x/y | see note |
| +0x2E | vram-ok / primitive-enable | |
| +0x30/+0x31 | U / V | |
| +0x32 | mode | 0=4bit, 1=8bit, 2=16bit |
| +0x33 | shade-tex-disable | =1 |
| +0x34-0x36 | tint RGB | default 0x40 each |
| +0x37 | semitrans / palette-cycle index | |
| +0x38 | STP-force flag | |

Note (confidence **low**): `+0x28/+0x2A/+0x2C` are *written* as shadow flag /
shadow screen-xy by `anim/UpdateEntityRender.c:162`, but *read* as rotation /
pivot by `sprite/RenderSpriteOrScaledQuad.c`. Either dual-purpose or one name is
wrong — flag before asserting.

Source: `port/decomp/sprite/PrepareSpriteVRAMSlotForContext.c:14`,
`port/decomp/sprite/RenderSpriteOrScaledQuad.c:13`.

### Layer render context (0x3C bytes)

From `InitLayerRenderContext_{Standard,Medium,Small}`. Three sibling builders
agree; runtime-validated (layers render). Confidence: **high**.

| Off | Field | Notes |
|-----|-------|-------|
| +0x00 | FSM marker | `0xFFFF0000` |
| +0x04 | tick/parallax callback ptr | |
| +0x10 | priority (s16) | default **30000** |
| +0x14 | active (u8) | =1 |
| +0x18 | entity vtable ptr | |
| +0x1C | render sub-object ptr | (this is what the render list stores) |
| +0x20/+0x24 | parallax scale X/Y (16.16) | overwritten by caller with scroll factors |
| +0x28/+0x2C | auto-scroll accumulator X/Y | |
| +0x30/+0x32 | auto-scroll step X/Y | |
| +0x34/+0x36 | wrap width/height (tiles) | |
| +0x38/+0x39 | auto-scroll enable X/Y | |
| +0x3A/+0x3B | reverse X/Y | |

Size-class builders differ:

| Builder | Gate | sub-obj vtable | alloc | tilemap builder | tick callback |
|---------|------|----------------|-------|-----------------|---------------|
| `_Standard` | >128² | ResourceType1 | 0x84 | `InitTilemapLayerRendering` | `UpdateParallaxScrollWithWrap_Standard` |
| `_Medium` | ≤128² | ResourceType2 | 0x70 | `InitTilemapLayer16x16` | `_Medium` |
| `_Small` | ≤64² | ResourceType3 | 0x80 | `InitTileLayerPrimitives` | `_Small` |

Source: `port/decomp/layer/layer_render_ctx.c:13`.

### Parallax scroll algorithm + span constants

Per frame, for each axis: `off = (camera * scale) >> 16` (logical shift);
reverse mode = `(screenSpanTiles - wrapTiles) * -0x10 - scaledCamera`. Auto-scroll
span = `wrapTiles * 0x80`; accumulator decremented by step, wrapped on span, then
`off += accumulator >> 3`. `_Standard` normalizes the accumulator against the
largest multiple of span (`span * (0x7FFFFFFF / span)`); `_Small`/`_Medium` wrap
on span directly. Negated `off` is written into the layer primitive (screenX @
`+0`, screenY @ `+2` off the ptr at ctx `+0x1C`).

**Screen-span constants: X = 0x14 (20) tiles, Y = 0xF (15) tiles.**

Confidence: **high** (three sibling functions self-consistent).
Source: `port/decomp/anim/UpdateParallaxScrollWithWrap_Medium.c:12`,
`port/decomp/anim/parallax_ticks.c:18`.

### Render / tick list mechanics

`addLayerToRenderList` inserts into **two** lists with different payloads and
**opposite** sort directions — nodes are 8-byte `{next@0, payload@4}` from the
BLB heap:

- Tick list (`GameState+0x1C`): payload = the **layer context**; sorted ascending
  by s16 priority @ `ctx+0x10` (new before first node with `newKey <= nodeKey`).
- Render list (`GameState+0x20`): payload = the **render sub-object** `*(ctx+0x1C)`
  (not the context); sorted by s16 z @ `subobj+0x08` with the **reversed**
  comparison (`nodeKey <= newKey`).

All three `AddLayerToRenderList_*` are behaviorally identical. Confidence:
**high**. Corroborated for entity lists by `blb/entity_lists.c` (entity tick list
sorted by s16 `entity+0x10`, render list by s16 `payload+0x08`). Source:
`port/decomp/layer/layer_render_list.c:26`.

### VRAM slab bin-packer + CLUT slot table

Memory map transcribed from the faithful `.s` (the runtime allocator *bodies* in
this file are PC-native shelf-bump replacements — do **not** document those as
the original packing algorithm). Confidence: **high**.

- Slab free-list: 0x58 (88) six-byte records at `heap+0xA08C`, record *i* at
  `base + i*6`: `+0` u16 row-occupancy mask, `+2` u8 heightCap, `+3` u8 next-link
  (0xFF=end, 0xFE=free), `+4` u8 widthBlocks/size-class (0xFE=free).
- Control: `+0xA29C` u8 free-list head, `+0xA29D` u8 colBits (`0x10 - bankBCount`),
  `+0xA29E` s8 availTotal.
- CLUT slot table: 12 × 8-byte records at `+0xA2A0`: `+0` flags (bit1=allocated,
  bit0=4/8-bit), `+2` u16 mask, `+4` s16 x, `+6` s16 y.
- `+0xA304`/`+0xA306` = 4-bit/8-bit CLUT-in-use counters; `+0xA300`/`+0xA302` =
  `cols<<5`/`cols<<4`.
- `InitVRAMSlotTable(bankA,bankB)` emits `(bankA+bankB)*3` CLUT descriptors from
  template `D_8009AE40`, Y descending from 0xF0 (bank A) / 0x1F0 (bank B).

Source: `port/decomp/vram/vram_alloc.c:15`.

### BLB heap free-list

Memory map (transcribed from the faithful path; unit-tested per
`docs/plans/pc-port.md`). Confidence: **high**.

- 100 block descriptors at `heap+0xA320`, stride 8: `{s32 ptr; u16 size16 (16-byte
  units); u16 next}`. `block[0]` describes the whole heap; blocks 1..99 = free-desc
  pool chained `1→…→99→0xFFFF`.
- Header: `+0xA640` void* heapStart, `+0xA644` u32 heapSize (bytes), `+0xA648` u16
  allocated-list head, `+0xA64A` u16 free-desc-pool head (=1 at init), `+0xA64C` u16
  totalBlocks (`heapSize>>4`), `+0xA64E` u16 min-free watermark.
- `AllocateFromHeap`: `need = (align*size + 0x13) >> 4` units, writes a 4-byte size
  header at `ptr[0]`, **returns `ptr+4`** (`flags&1` = raw, skip header).
- `FreeFromHeap` coalesces address-adjacent predecessor/successor; list kept
  address-sorted.

Source: `port/decomp/vram/heap_alloc.c:9`. Note: `docs/systems/game-loop.md`'s
`AllocateFromHeap` stub omits the 4-byte header and the `ptr+4` payload.

### VRAM tile-cell grid geometry

`CalculateVRAMCoordinates`: bump allocator over a grid of **4px-wide × 16px-tall
cells across 176 columns (704px) × 32 rows**; cursor = u16 at `heap+0xA08A`.
Output `X = heap[0] + col*4`, `Y = row*16`. Advances 1/2/4 cells for size 0/1/2;
wraps to next row past the column limit (0xAF size1, 0xAD size2). Returns the
pre-advance cursor. Confidence: **high**. Source: `port/decomp/vram/vram_coords.c:17`.

### Sprite-sheet VRAM cache (20-slot, at `g_pOrderingTableBase`)

`FindOrderingTableSlotById` is a **sprite-sheet cache lookup, not a GPU
ordering-table op** (likely misnomer — flag for a name review). It linear-searches
a 20-entry (0x14) array of 0x18-byte descriptors at `g_pOrderingTableBase`
(`D_800A595C`, seeded at boot as `g_LayerRenderSlots`), the same cache
`LoadSpriteFramesToVRAM` populates.

Cache entry (0x18): `+0x00` u32 hash, `+0x04` per-frame array ptr, `+0x08` u16 maxW,
`+0x0A` maxH, `+0x0C` s16 CLUT x, `+0x0E` s16 CLUT y, `+0x10` u16 total frames,
`+0x12` u16 CLUT id, `+0x14` u8 lookup byte.
Per-frame entry (0x14): `+0x00` frame-metadata ptr (0x24-byte record), `+0x04`
s16 VRAM x, `+0x06` VRAM y, `+0x08` half-width, `+0x0A` height, `+0x0C` u16 tpage,
`+0x0E` u8 U, `+0x0F` u8 V, `+0x10` u8 vram-ok.

Confidence: **high** (structure); **medium** (the "OrderingTable" name).
Source: `port/decomp/layer/FindOrderingTableSlotById.c:5`,
`port/decomp/layer/LoadSpriteFramesToVRAM.c:12`.

### Runtime tile descriptor (8-byte) — same array as `GameState+0x108`

Tilemap builders index a tile table by `cell & 0xFFF`, stride **8 bytes**:
`+0x0` u16 tpageSel, `+0x2` u16 clut, `+0x4` u8 u0, `+0x5` u8 v0, `+0x6` u8
semiTrans. `LoadTileDataToVRAM` builds exactly this array at `gs+0x108`
(`{tpage@0, clut@2, subX@4, y@5, sizeFlag@6}`) — i.e. the per-tile render-state
array *is* the `tileTable` arg passed into the tilemap builders. Tint table =
`colorTable + (cell>>12)*3` → {r,g,b}. Confidence: **high**. Source:
`port/decomp/layer/tilemap_prims.c:21`, `port/decomp/layer/tile_vram.c:158`.

---

## Sprite / asset decode (net-new)

### Sprite-asset header = id-keyed sub-entry table

From `InitSpriteContext` — multiple sprites share one header/atlas, disambiguated
by re-searching an id table. Confidence: **high** (behavioral: the lookup
re-matches `entry[0] == spriteId`).

- `header+0x00` u16 = **entry count** of the id-table (docs called it "animation count").
- `header+0x02` u16 = header_size / frame-meta base: `frame_metadata = header + header[0x2] + entry[0x6]`.
- `header+0x04` u32 = **rle_data base**.
- `header+0x08` u32 = **secondary_ptr** (applied only if non-zero; docs called it "palette offset").
- Id-table at `header+0xC`, **stride 0xC**: `+0` u32 sprite id (matched against the
  requested id), `+0x4` total_frame_cnt, `+0x6` frame-meta offset, `+0x8` byte stored
  to SpriteContext+0x12.

Resulting **SpriteContext (20 bytes)**: `+0x00` frame_metadata, `+0x04`
secondary_ptr, `+0x08` rle_data, `+0x0C` max_width (s16), `+0x0E` max_height,
`+0x10` total_frame_cnt, `+0x12` lookup byte, `+0x13` is_valid. Frame table stride
**0x24**; width=s16@+0xA, height=s16@+0xC. Source:
`port/decomp/blbacc/InitSpriteContext.c:49`.

### RLE decoder — resumable, row-budget-limited, vertical flip

`DecodeRLESpriteCore(ctx, rle_state[6], max_rows)` decodes at most `max_rows`
new-line advances; on budget exhaustion (-1) it saves `{rows, token, pixel,
rowBase}` back into `rle_state[0/3/4/2]` and returns 0 (resumable), else 1.
`rle_state` = `[0]rows_remaining [1]row_stride [2]row_base [3]token_ptr
[4]pixel_ptr [5]flip`. RLE token (16-bit): bit15 = advance row, bits[14:8] =
horizontal skip, bits[7:0] = run length; pixel copy unrolled 8/4/1. `runLen==0`
(low byte 0) → pure skip, no copy (`if (((runLen-1)&0xffff)!=0xffff)` guard).

Vertical flip (`GetFrameMetadata`/`sprite_decode.c`): negates row stride and
offsets output base by `rowStride*(h-1)`; horizontal flip offsets base by `(w-1)`
and emits right-to-left. Confidence: **high**. Source:
`port/decomp/blbacc/DecodeRLESpriteCore.c:23`, `port/decomp/blbacc/sprite_decode.c:26`.

### Secondary sprite bank (`D_800A6060`) record layout

`LookupSpriteById` fallback: count @ `bank+0xC` (u32), id array @ `bank+0x10`
(`count` u32s), record table at `bank + count*4 + 0x10` stride 0x14; matched
header ptr = `bank + *(u32*)(recTable + i*0x14 + 8)`. Confidence: **high**.
Source: `port/decomp/blbacc/LookupSpriteById.c:34`.

### Runtime sprite TOC (level, distinct from Asset-600 sub-TOC)

`FindSpriteInTOC` searches two level TOCs at `LevelDataContext+0x70` then `+0x40`.
Each is a word array: `[0]` = count; entries at stride **3 words** with
`entry[+1]` = sprite id and `entry[+3]` = header byte-offset from TOC base; hit
returns `TOC_base + entry[+3]`. This is the runtime index built by
`SetSpriteTables`, not the on-disk Asset-600 `{id,size,offset}` sub-TOC.
Confidence: **medium-high** (the +1/+3 interleave within a 3-word stride is
unusual — double-check against disassembly). Source:
`port/decomp/blbacc/FindSpriteInTOC.c:13`.

### `LoadAssetContainer` navigation

`LoadAssetContainer(ctx, assetIdx, flag)`: reads dir type byte @ `blb +
current_sequence_index + 0xF36`; **if type < 3 returns**; if `type != 3` and
`!(flag&0xFF)` returns. When `type == 3`, `subIdx = blb[+0xF92]` picks a 0x70-byte
level-metadata entry (`entry = blb + subIdx*0x70 + assetIdx*2`); sector off/cnt =
u16 @ `entry+0x1C`/`+0x2A` (primary/secondary, `flag&0xFF`) else `entry+0x38`/`+0x46`
(tertiary). On failed read it **recursively retries once with `assetIdx=1`**. Sets
`current_sub_block = assetIdx` (sub-dir) or `= type` (primary). Loaded container:
`base[0]` = record count (u32); records stride 0xC = `{type@+4, size@+8,
dataOffset@+0xC}`; asset ptr = `base + dataOffset`. Confidence: **medium-high**.
Source: `port/decomp/blbacc/LoadAssetContainer.c:45`.

Note: the `entry+0x1C/+0x2A/+0x38/+0x46` reads are exactly **2 bytes below** the
`0x1E/0x2C/0x3A/0x48` array bases in [level-metadata.md](../blb/level-metadata.md)
and `docs/blb.hexpat` — reconcile which is the true field base.

### Header directory stage index at `+0xF93`

The boot override writes `buffer[0xF36]=3` (game mode), `buffer[0xF92]=level`,
**`buffer[0xF93]=stage`**. Header read is 2 sectors / 0x1000 bytes via
`CdBLB_ReadSectors(0,2,buffer)`; buffer at `g_pBlbHeapBase + 0xA650`; `gs+0x40`=buffer,
`gs+0x3C`=buffer+0x1000; sentinel `0x1234567` → `gs+0x12C` and `D_801FC400`.
Confidence: **high** (matches `scripts/game_watcher.lua` boot-override addresses).
Source: `port/decomp/blb/LoadBLBHeader.c:56`. `+0xF36`/`+0xF92` are already in
[header.md](../blb/header.md); `+0xF93` is new.

### Animated-tile render state (`GameState+0x108`)

`InitAnimatedTileEntities` lazily creates ONE backing entity (0x100-byte alloc) on
the first animated tile, caches its sprite context, computes `tilesPerRow =
(ctx.width + 0xF) >> 4`, then per animated tile fills an 8-byte entry at
`gs+0x108 + i*8`: `{tpage u16@+0, clut u16@+2, subX s8@+4, subY s8@+5, valid@+6}`,
`subX = ctx[+0x30] + (idx%tpr)*16`, `subY = ctx[+0x31] + (idx/tpr)*16`, `idx =
GetAnimatedTileData()-1`. Confidence: **medium-high**. Source:
`port/decomp/blb/anim_tile_entities.c:29`.

---

## FSM / dispatch (net-new detail)

### Indexed (marker>0) FSM dispatch slot — exact byte layout

The tagged-dispatch idiom shared by the tick, event, render, move-marker, and
anim-state slots. For a packed marker word `{argBase = (s16)lo, index = (s16)hi}`
and a callback/table word:

- `index == 0` → no handler.
- `index < 0` → `fn = callbackWord; fn(entity + argBase)`.
- `index > 0` → `tableBase = *(s32*)(entity + (s16)callbackWord)`; `entry =
  tableBase + index*8`; **`argOff = (s16)*(s32*)(entry-8)`, `fn = *(entry-4)`**;
  `fn(entity + argBase + argOff)`. (Table indexed from 1; each 8-byte entry read
  *backwards* from `index*8`.)

Confidence: **high** (four independent port files agree). Source:
`port/decomp/anim/EntitySetState.c:23`, `anim/UpdateEntityRender.c:54`,
`anim/UpdateSpriteFrameData.c:78`, `anim/TickEntityAnimation.c:44`. Extends
[fsm-callback-patterns.md](../systems/fsm-callback-patterns.md).

### GameState mode dispatch decodes `0xFFFF0000` the same way

`modeId = *(s16*)(gs+0x02)`, `baseOff = *(s16*)(gs+0x00)`. Standard gameplay
`0xFFFF0000` → baseOff **0**, index **−1** → `(*(fn*)(gs+0x04))(gs + 0)` (plain
`GameState*`, **not** `gs - 0x10000`). Only for `modeId > 0` does it index a table
at `*(gs+0x04)` (stride 8) and add `argOff` to `baseOff`. `InitializeAndLoadLevel`
installs two pairs: `+0x00/+0x04 = {0xFFFF0000, GameModeCallback}` and
`+0x08/+0x0C = {0xFFFF0000, EntityDestructCallback}`. Confidence: **high**. Source:
`port/decomp/boot/game_boot.c:190`, `port/decomp/lvlload/InitializeAndLoadLevel.c:254`.

### `EntitySetState(entity, marker, callback)` exit-hook model

Clears active pair `+0xA0/+0xA4`; if an old state existed (`+0xAA != 0`),
snapshots+clears the exit/finalizer slot `+0xA8/+0xAC` and dispatches it; only if
the exit hook did not re-install a state (`+0xA2 == 0`) does it clear
`+0x94/+0x98/+0x9C`, install the new active pair, and dispatch it. (Not the
triple-clear that `animation-framework.md` describes; matches
[fsm-callback-patterns.md](../systems/fsm-callback-patterns.md).) Frame/anim event
callbacks fire through the `+0x08/+0x0C` event slot with a 4-arg signature:
`fn(entity+off, 1, callbackId, entity)` (frame, msg 1) and `fn(entity+off, 2, 0,
entity)` (anim complete, msg 2). Confidence: **high**. Source:
`port/decomp/anim/EntitySetState.c:41`, `anim/UpdateSpriteFrameData.c:72`,
`anim/TickEntityAnimation.c:37`.

### GameState post-render vtable `D_80012100`

Installed at `GameState+0x18` by `ConstructStaticGameState` at boot. The
post-render dispatch `(**(*(gs+0x18)+0x1C))(gs + …)` calls **slot 7 =
`EntityRemoval`** (per-frame texture/CLUT upload; without it sprites never
upload). Four 8-byte `{arg,fn}` pairs: `+0x0C DestroyEntityAndFreeResources`,
`+0x14 MainNoopCallback_80082844`, `+0x1C EntityRemoval`. The clean-room alias
`DEAD_ENTITY_VTABLE` in `src/gstate.c` is a **misnomer** — this is the live
GameState vtable. Confidence: **high**. Source:
`port/decomp/data/gamestate_vtable.c:1`, `port/decomp/boot/game_boot.c:137`.

---

## Data tables (net-new)

### `entity_type_remap` — full authoring→internal dictionary

`RemapEntityTypesForLevel` walks the spawn list (`GameState+0x28`, 24-byte defs),
rewrites `def+0x12` from BLB authoring id to engine internal id, dispatched on the
byte at `def+0x14` (port calls it **category**: 1=generic actor, 2=enemy/boss,
3=decoration). Docs previously called that byte a "layer" (1=fg / 2=bg / 3=special)
— same field/values, opposite semantic label; reconcile.

Category 2 (enemy/boss) special rules: authoring `0xC9..0xE4` pass through to
`def+0xC`, type `0x5F`; `0x3E` forces `def+0xC=0xC9`, type `0x5F`; `0xA7` sets
`GameState[0x18E]=1` if level flag `0x2000`; default type `8`.

The full case tables (cat1 generic-actor, cat2 enemy/boss, cat3 decoration) are
transcribed in `port/decomp/entinit/entity_type_remap.c:24`. Confidence: **high**
(mechanism + rows match the doc spot-checks in
[ENTITY_REMAPPING_CORRECTION.md](../reference/ENTITY_REMAPPING_CORRECTION.md)).
Docs previously said "the full mapping … further analysis is needed" — this file
is that mapping.

### `entity_callback_table` (`D_8009D5F8`) — 121 type→init pairs

242 words = 121 `{marker, fn}` pairs. Unused slots (both words 0): 13,14,15,16,
56, 73,74, 77,78. Notable fan-in: types 42,43,44,53,54,55,60 → one fn; 89,97,98,
110,111 → GridHelper; 85,104,105 → TriggerZone; 24,118 → SpecialAmmo. Bosses:
71=MonkeyMage, 72=SuperWillie, 100=GlennYntis, 101=ShrineyGuard, 102=JoeHeadJoe,
103=Klogg. Name divergences to reconcile (both sides clean-room guesses): internal
49/50/51 (port: ClayballOnPath A/B/C vs docs: Boss/BossPart); 79 (port:
EnemySpawner, corroborates `ENTITY_REMAPPING_CORRECTION.md` over
`entity-identification.md`'s "Skullmonkey Standard"); 42-60 family (port: SnoBlo vs
docs: SuperWillie heads). Confidence: **high** (structure); **medium** (names).
Source: `port/decomp/data/entity_callback_table.c:6`.

### Sprite-entity allocation & extended-region offsets

Sprite-backed entities allocated at **0x44C bytes** (`InitEntityStruct(e, 0x44C)`)
— base header is still 0x80. Extended region: `+0x34` sprite render context ptr,
`+0x78` frame-table region (`+0x84` pixel width u16, `+0x86` pixel height u16),
`+0x90` ptr to a 0x18-byte sprite-asset descriptor (BLB heap), `+0xB0` decoded-pixel
buffer, `+0xD4` pixel area (w×h), `+0xE0` pending-sprite-state flags (`(v&3)==1` →
`ApplyPendingSpriteState`; `(v>>3)&1` → `UpdateSpriteFrameData`). Six 16.16 scale
fields set to 1.0: `+0x50` scaleRender, `+0x54` scaleRender2, `+0x58/+0x5C`
scalePowerupX/Y, `+0x60/+0x64` scaleParallaxX/Y (docs count only four). Confidence:
**medium-high**. Source: `port/decomp/entity/InitEntityStruct.c:77`,
`entity/AllocSpriteRenderContext.c:37`, `entity/InitEntitySprite.c:41`,
`entity/InitEntityAnimationState.c:54`.

### `g_PlayerCallbackTable` (`0x80011804`) layout

0x80-byte table = two stacked 8-slot `{arg,fn}` pair tables. Standard entity
vtable: `[3]=EntityDestructor_WithSPUVoiceStop`, `[5]=UpdateEntityRender`,
`[7]=UploadEntityTextureIfDirty`. Extended (from +0x40): `[11]=FinnSubentity
DestroyCallback_Simple`, `[13]/[15]=Finn noop callbacks`. **Words 0x60-0x7F are
zero** — which contradicts `fsm-callback-patterns.md`'s claim that `+0x40` is a
"dense array of bounce handlers." Confidence: **medium** (verify whether the
bounce array is a separate adjacent symbol at ~0x80011844). Source:
`port/decomp/data/player_vtable.c:18`.

### Menu widget vtable (7 slots) + HUD reveal event `0x1013`

`MenuInputHandler` dispatches the highlighted item's vtable (`item+0x18`) through
7 slots, each paired with an arg offset: `+0x24` highlight / `+0x2c` unhighlight,
`+0x34` left-adjust / `+0x3c` right-adjust, `+0x44` fast-up / `+0x4c` fast-down,
`+0x54` confirm(code). Nav (pressed 0x4000=down/0x1000=up) plays SFX `0x646c2cc0`;
held nav auto-repeats via counter `+0x131` when `>10`. Confirm action-code 0-7 from
pressed bits: `0x80→1, 0x20→3, 0x40→0, 0x04→4, 0x01→5, 0x08→6, 0x02→7`.

`EntityCollision_HUDItemActivate` (event `0x1013`, "show HUD group N", N=1..9)
group→(flag byte, first-ptr offset, count): `{0xA3,0x20,3},{0xA4,0x2C,3},
{0xA5,0x38,3},{0xA6,0x44,3},{0xA7,0x50,3},{0xA8,0x5C,3},{0xA9,0x68,4},{0xAA,0x78,3},
{0xAB,0x84,3}` — group 7 has 4 sub-entities. On show: add to sorted render list,
flag=1, reset each sub-entity fade byte `+0x110`=0, bump visible-timer `+0x10C` to
≥`0x78` (120) frames. The flag bytes are exactly the reveal side of the 10-flag
visibility map in [hud-system-complete.md](../systems/hud-system-complete.md).
Confidence: **high**. Source: `port/decomp/passwd/MenuInputHandler.c:5`,
`port/decomp/hud/EntityCollision_HUDItemActivate.c:19`.

---

## Player (net-new)

- **Damage-flash / powerup-glow tint** (`playst/PlayerTickCallback.c:198`,
  confidence **high**): writes tint into sprite ctx `+0x34/0x35/0x36`, STP toggle
  `+0x38`, palette-cycle `+0x37`. rgbCooldown `+0x17D` gate; powerup-active fixed
  glow tint `(0x20,0x60,0x30)` with STP on; damage flash `base=(frame&3)*0x20`,
  `adj=base-0x30`, tint = baseRGB+adj (clamped) or `(base-0x10, base+0x30, base)`.
- **Powerup-timer expiry** (`:106`): on countdown to 1, play `0x40E28045`, restore
  CLUT (`+0xF8=1` clutDirty), tear down HUD entity `+0x14C`.
- **Three-way render-scale easing** (`:179`): shrinkFlag `+0x1B0` → floor **0x4000**;
  else shrink_mode → **0x8000**; else grow toward **0x10000**; ±0x1000/frame, gated
  by disableScale `+0x178`.
- **Hamster-shield override** (`:63`): if `(g_GameFlags & 1)` and currentSprite
  `+0xCC != 0x1E28E0D4`, force state → `HamsterSpriteCallback` (D_800A5D90). Docs
  called this a "debug mode check" — likely wrong (see
  [player-state-fsm.md](../systems/player/player-state-fsm.md), which independently
  names the callback).
- **`+0x16C` = Yellow-Bird glide companion** (powerup bit 2), created by
  `CreateYellowBirdEntity` (0x110-byte alloc) — not "trail_entity/CreateTrailEntity."
  `+0x168` = halo companion (powerup bit 1). Confidence **high**. (`:156`)
- **`PlayerCallback_JumpInputAndCounters` (0x800602E0)** — airborne input-FSM slot
  (`+0x104/+0x108`): weapon-click SFX `0x64221E61`; swirly-Q air toss (special-mask
  `D_800A596E`); glide deploy; air steering gated by jumpHoldCounter `+0x11F`;
  velY floor clamp −0x48000 when jump not held. Confidence **high**.
  (`playst/PlayerCallback_JumpInputAndCounters.c:38`)
- **`PlayerProcessTileCollision` (0x8005A914) does trigger-zone dispatch, not tile
  collision** — AABB rect list at `GameState+0x74`/count `+0x78`, 16-byte records
  `{s16 l,t,r,b; s32 attr; s32 data}` (BLB asset 602). Suggested name
  `PlayerProcessTriggerZones`. Trigger case `0x2A` = crouch-slide chute (→
  `PlayerStateInit_CrouchSlide`), not "water state." Confidence **medium-high**.
  (`playst/trigger_zones.c:1`)
- **`PlayerStateCallback_2` (0x8006864C) is the spawn-drop / falling-state init**,
  not "hit/damage" or "normal/idle": caps velY, installs falling FSM +
  `FallingPhysicsMain`, falling sprite `0x00388110`, dispatches event `0x1005`
  (detach) to `+0x12C`. Confidence **high**. (`playst/PlayerStateCallback_2.c:34`)
- **Default player sprite id = `0x08208902`** (first word of `D_8009C174`, a
  0xE0-byte spriteDef blob). Availability test in `CreatePlayerEntity` = build sprite
  context, accept if `ctx+0xC (maxW)!=0 && ctx+0xE (maxH)!=0` (not `LookupSpriteById
  != NULL`). Confidence **medium**. (`player/CreatePlayerEntity.c:46`,
  `data/player_spawn_data.c:12`)

> The full 0x100+ PlayerEntity extended field map the port relies on is **already
> in `include/Game/entity.h`** and matches the port; the lower-effort doc win is to
> sync the `docs/systems/player/` prose offset tables to that struct.

---

## Core / boot / level (net-new)

- **`GameModeCallback`** (`lvlload/GameModeCallback.c:46`, confidence **high**):
  complete per-frame FSM — pause countdown `+0x160`; input masks `0x800` (pause),
  `0x5000` (menu nav), `0x40` (Cross); level-transition flags `+0x144..+0x148`,
  `+0x19C`, `+0x152`; checkpoint `+0x149/+0x14A`; debug pause `+0x18F/+0x190`.
  `SetupAndStartLevel(gs,99) == 0` → `initPlayerState` (fresh restart).
- **Boot sequence** (`boot/game_boot.c:127`): includes `ConstructStaticGameState`
  (installs the `D_80012100` vtable — missing from `game-loop.md`'s main() list). The
  three sdata writes `D_800A596C=0x40 / D_800A596E=0x20 / D_800A5970=0x80` are
  **button-mask config** (jump/special/run), not "default BG color" as
  `game-loop.md` says (confidence **medium** on the reinterpretation; corroborated by
  `docs/plans/pc-port.md` and `src/gfx.c`).
- **`LevelDataParser`** (`level/LevelDataParser.c:28`): primary block = `count`
  (u16 @ base[0]) + records at stride 0xC `{rtype@+4, rsize@+8, roffset@+0xC}`,
  `dataPtr = primary_data + roffset`. Container tags 0x258/0x259/0x25A →
  container_600/601/602. Confidence **high**.
- **`InitializeAndLoadLevel` tail** (`lvlload/InitializeAndLoadLevel.c:197`): load
  FSM on `D_800A60A4` (0/2/4 cycling container mode 1/5/6 for the loading screen);
  sub-heap carve `InitHeapConfig(heap, start, 0x801FC000 - start)` (PSX RAM top);
  free blocks filled `0xEEEEEEEE`; heap min-free latch `*(u16*)(heap+0xA64E) =
  *(u16*)(heap+0xA64C)`; `gs[0x12A] = 0x28`. Confidence **medium-high**.
- **`ProcessDebugMenuInput`** (`main/ProcessDebugMenuInput.c:30`): gated by
  `g_GameFlags & 0x80`; 20-row window; cursor `<10` → `direct_level_load = cursor+1`;
  `10..count+10` → `SetSequenceIndexByMode(ctx, 4, cursor-10)` (mode 'c'); else movie
  (mode 1). `D_800A60BC` = level-count mirror, `D_800A60BE` = movie-count mirror.
  Confidence **high**.
- **`memset_entrypoint`** (first .text fn, `mem/memset_entrypoint.c:13`): signature
  `(void *dst, int count, int fill)`; fills 16 bytes/iter, then an **unconditional**
  single-byte write before the tail loop (writes ≥1 byte even for count%16==0).
  Confidence **medium**.

---

## Applied corrections (conflicts already fixed)

These were corroborated against a second ground-truth source and edited directly
into the authoritative docs:

| Doc | Was | Now | Corroborated by |
|-----|-----|-----|-----------------|
| `entities.md` 0xB4/0xB8 | frame X/Y **scale** | frame **motion** vectors | `include/Game/entity.h:317` |
| `entities.md` 0x78/0x7C | move callbacks | pFrameTable / pPaletteData | `include/Game/entity.h:241` |
| `functions-complete.md` 0x80019650 | GetLayerProperty | GetSpriteFrameDataByIndex | `symbol_addrs.txt:233` |
| `game-loop.md` 0x50 | input_state_ptr | camera_velocity_y (input_state_ptr is 0x140) | `include/Game/game_state.h:85,176` |
| `game-loop.md` flag 0x2000 | CreateBossPlayerEntity | CreateResultsScreenEntity | `symbol_addrs.txt:1595` |
| `hud-system-complete.md` | 5 flags; 0xa4="overall HUD" | 10 flags 0xa2..0xab; 0xa4=lives | `hud/UpdateHUDEntityVisibility.c` + write-side in `CreateMenuEntities.c` |

---

## Port-header address errors

Port file *header comments* cite addresses that disagree with `symbol_addrs.txt`
(ground truth). Use the ground-truth values; the port file *bodies* are still
useful. Flagged so these are not propagated:

| Function | Port header says | Ground truth |
|----------|------------------|--------------|
| `TickEntityAnimation` | 0x8001D2E8 | 0x8001D290 |
| `ApplyPendingSpriteState` | 0x8001D3D0 | 0x8001D554 |
| `UpdateParallaxScrollWithWrap_Standard` | 0x8001F19C | 0x8001EEEC |
| `UpdateParallaxScrollWithWrap_Medium` | 0x8001F3B4 | 0x8001F368 |
| `UpdateParallaxScrollWithWrap_Small` | 0x8001F5A4 | 0x8001F778 |
| `SetupSpriteFromFrame` | 0x800195A8 | 0x80019678 |
| `FindOrderingTableSlotById` | 0x800195A0 | 0x800195B0 |
| `PlaySoundEffect` (comment) | 0x8007A5A4 | 0x8007C388 |
| `DecodeRLESpriteCore` | 0x80010264 | 0x80010068 (0x80010264 may be a mid-fn label) |

See also [unconfirmed-findings.md](unconfirmed-findings.md) for the items still
genuinely unsettled (e.g. the Asset-302 bit `0x04` disagreement).
