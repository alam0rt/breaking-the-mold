---
title: Recovered Compilation-Unit Structure (SLES-01090)
category: architecture
tags: [compilation-units, link-order, file-boundaries, ghidra, rodata, psyq, glover-reference, splat]
---

# Recovered Compilation-Unit Structure (SLES-01090)

**Date:** 2026-06-14
**Method:** Ghidra link-order analysis (rodata→text cross-reference) + ordered
function-name clustering + PSY-Q library `*_OBJ_*` boundary markers, cross-checked
against the real Glover (PSX) source tree as a granularity reference.

**Caveat (clean-room):** Original filenames are *not* recoverable from the binary.
Names below are inferred labels for decompilation planning. What *is* hard evidence
is the **link order** and the **rodata-anchored object boundaries** — those are
derived from the binary, not guessed.

---

## 1. How the binary is linked

Ghidra memory map (authoritative):

| section | range | notes |
|---|---|---|
| `.rdata` (rodata) | `0x80010000–0x800131EF` | **all** rodata, grouped at front |
| `.text` | `0x800131F0–0x800907CF` | all code (~513 KB) |
| `.data` | `0x800907EC–0x800A5953` | |
| `.sdata` | `0x800A5954–0x800A611F` | GP-relative (`-G8`) |
| `.sbss`/`.bss` | `0x800A6120–0x800AE3DF` | |

This is **section-grouped GNU-ld output**: every object file contributes to each
section, and objects appear in the **same relative order in every section**. So the
order of rodata sub-blocks == order of text sub-blocks == link order. ROM offset =
`VRAM − 0x8000F800`.

### Link-order skeleton is confirmed (not guessed)

`get_bulk_xrefs` on the first item of each jsonnet rodata sub-segment shows which
text address first references it. Because an object's functions reference that
object's own rodata, this pins each rodata object to its text location:

| rodata seg (jsonnet) | first item | referenced by text @ | ⇒ text unit begins |
|---|---|---|---|
| ENGINE | `0x80010324` | `0x80015B40` | render core |
| OBJECT | `0x8001062C` | `0x8002ABF8` | `0x8002A378` ✓ |
| BOSS | `0x80011628` | `0x80058FE4` | `0x80058168` ✓ |
| PLAYER | `0x80011844` | `0x8005A71C` | `0x8005A630` ✓ |
| PLAYER_STATES | `0x80011D54` | `0x80070730` | `0x80071048` ✓ |
| VEHICLE | `0x80012140` | `0x8007D29C` | `0x8007D1D0` ✓ |
| MAIN | `0x80012758` | `0x80082EF8` | `0x80082EF0` ✓ |
| LIBCD | `0x80012778` | `0x800830A4` | `0x80083000` ✓ |

**Conclusion:** the 10 rodata-anchored units in `SLES_010.90.jsonnet` are real
object/object-group boundaries. The structure is sound; it is just **too coarse** —
several big text blobs bundle multiple source files that happen to carry no rodata
(so they are invisible in the rodata section and got lumped together).

---

## 2. Recovered fine structure

Boundaries are at real function starts. Evidence class:
**[R]** rodata-anchored (hard); **[L]** PSY-Q `*_OBJ_*` library marker (hard);
**[N]** clean subsystem name-cluster transition (strong); **[g]** large inter-function
gap (supporting).

### Render / engine core  (jsonnet `ENGINE*`, `0x800131F0–0x8001A0C8`)

| VRAM start | inferred unit | anchor functions |
|---|---|---|
| `0x800131F0` [R] | `gfx.c` — video/double-buffer/OT + prim pool | `InitGraphicsSystem`, `SwapBuffersAndClearOT`, `AllocPrim20..36` |
| `0x800138F0` [N] | `vram.c` — VRAM slots, heap, CLUT slots | `AllocateVRAMSlot`, `AllocateFromHeap`, `AllocateCLUTSlot` |
| `0x8001543C` [N] | `sprite.c` — sprite + tilemap rendering | `RenderSpriteOrScaledQuad`, `RenderTilemapLayerWithScroll` |
| `0x8001889C` [g] | `layer.c` — sprite obj + layer render-slot mgmt | `InitSpriteObject`, `FreeLayerRenderSlot`, `LoadSpriteFramesToVRAM` |
| `0x80019748` [N] | `clut_fx.c` — menu-entity base + CLUT cycle/lerp | `InitMenuEntityWithVtable`, `CLUTPaletteCycleTickCallback` |

### Entity kernel  (jsonnet `ENGINE/entity_system`, `0x8001A0C8–0x8002A378`)

One large engine unit (≈ `entity.c`), possibly split as entity + animation +
collision + entity-list in the original. Anchors: `InitEntityStruct`, parallax /
world↔screen transforms, `CheckBoxCollision`, `TickEntityAnimation`,
`UpdateEntityRender`, `EntitySetState`, `EntityTickLoop`, `RenderEntities`,
`AddTo*List`, `LoadBLBHeader`.

### OBJECT blob  (jsonnet `OBJECT`, `0x8002A378–0x80058168`, ~188 KB → ~8 files)

This single jsonnet unit is really a stack of distinct source files:

| VRAM start | inferred unit | anchor functions |
|---|---|---|
| `0x8002A378` [R] | `hud.c` — HUD + pause menu | `UpdateHUDEntityVisibility`, `ShowPauseMenuHUD` |
| `0x8002BFF0` [N] | `entity_dtor.c` — generic destructor family | `EntityDestructor_Type0..6` |
| `0x8002C7D8` [N] | `decor.c` — path/decor entities | `InitPathDecorEntity`, `DecorEntityTick*` |
| `0x8002D474` [N] | `collectibles.c` — pickups | `InitClayballWithRandomColor`, `Init*Collectible` (PhoenixHand, PhartHead, UniverseEnema, 1970Icon, HamsterShield, SuperWillie) |
| `0x80030D0C` [N] | `effects.c` / `particles.c` — particles, grid/ripple/beam FX, password/VRAM-slot/menu-sprite/loading/fade entities | `InitParticleEntity`, `RenderGridDistortionEffect`, `RenderRippleExpandEffect`, `RenderSpotlightBeamEffect`, `InitDebrisParticleEntity` |
| `0x800389DC` [N] | `cd.c` — game-level CD/BLB I/O + audio track | `LoadGameAssetLocations`, `CdBLB_ReadSectors`, `PlayCDAudioTrack`, `ProcessCDStreamState` |
| `0x80039150` [N] | `movie.c` — STR movie streaming/decode | `PlayMovieFromCD`, `InitMovieDecoder`, `MovieFrameDecodeCallback` |
| `0x8003A394` [N] | `enemies.c` — enemy AI, projectiles, switches, teleporters, platforms, sound-emitters | `EntityEventHandler*`, `InitSnoBloEnemy`, `LaserMonkey*`, `Init*Projectile`, `Teleporter*` |
| `0x80047288` [N] | `bosses.c` — boss state machines + homing/clay projectiles | `InitKloggBossEntity`, `InitMonkeyMageBoss`, `InitGlennYntisBoss`, `InitShrineyGuardBoss`, `InitJoeHeadJoeBoss`, `HomingProjectile_TrackTarget` |

> Note: the actual boss **state machines** (Shriney/Joe/Klogg/Glenn/MonkeyMage) live
> here at `0x80047288+`, *inside* the OBJECT blob. The segment the jsonnet currently
> *labels* `BOSS/boss` at `0x80058168` is the **next** object — see below.

### BOSS object  (jsonnet `BOSS/boss`, `0x80058168–0x8005A630`) [R]

Clayball / circular-platform / ShrineyGuard-sound code: `CircularPlatform*`,
`InitClayballWithSwitchBlock`, `InitBonusClayballEntity`, `ShrineyGuardSoundUpdateTick`.
(The jsonnet name "BOSS" is loose; its rodata at `0x80011628` is genuinely a separate
object.)

### PLAYER  (jsonnet `PLAYER` + `PLAYER/destructor`, `0x8005A630–0x80071048`) [R]

| VRAM start | inferred unit | anchors |
|---|---|---|
| `0x800596A4`* | `player.c` — create/init + collision/coord primitives | `CreatePlayerEntity`, `CheckWallCollision`, `TransformYCoordWithScaleSnapped` |
| `0x8005A630` [R] | `player_states.c` — the big player FSM | `PlayerTickCallback`, `PlayerState_*`, `PlayerCallback_*` |
| `0x8006E008` [N] | `finn.c` — FINN vehicle + subentity | `Finn*`, `FINN*`, `CreateGlidePlayerEntity` |

*`player.c` start (`0x800596A4`) precedes the PLAYER rodata anchor; it sits at the tail
of the prior segment in the jsonnet — `player.c` may itself be the first PLAYER-group object.

### PLAYER_STATES blob  (jsonnet `PLAYER_STATES`, `0x80071048–0x8007D1D0`) [R]

Another multi-file blob:

| VRAM start | inferred unit | anchors |
|---|---|---|
| `0x80071048` [R] | `vehicle.c` — Runn/Soar/Finn vehicle modes + moving platforms | `RunnState_*`, `SoarState_*`, `Platform*`, `CreateRunnPlayerEntity`, `CreateFinnPlayerEntity` |
| `0x80074F98` [N] | `menu.c` — cursor/buttons/options/level-select | `InitMenuCursorEntity`, `InitMenuButton*`, `InitMenuStage1..4`, `MenuInputHandler` |
| `0x80075FF4` [N] | `password.c` — password entry UI | `InitPasswordDisplayEntity`, `PasswordDigitInputHandler` |
| `0x80077FE8` [N] | `hud_results.c` — HUD digits + results screen | `InitHUDDigitDisplay`, `CreateResultsScreenEntity` |
| `0x8007963C` [N] | `ending.c` — ending/credits | `EndingTickCallback`, `EndingCredits*` |
| `0x8007A1BC` [N] | `level.c` — level-data context + playback sequence | `InitLevelDataContext`, `AdvancePlaybackSequence`, `LevelDataParser`, `LoadLevelByWorldIndex` |
| `0x8007A9B0` [N] | `blb_accessors.c` — BLB/level/tile/sprite TOC accessors | `GetLevelCount`, `GetTilemapDataPtr`, `FindSpriteInTOC` (~120 tiny accessors) |
| `0x8007BFB8` [N] | `sound.c` — SPU upload, SFX, voice/volume/reverb, CD audio | `InitSPUDefaults`, `PlaySoundEffect`, `StartCDAudioForLevel` |
| `0x8007CCFC` [N] | `gamestate.c` — `InitGameState`, respawn, level start | `InitGameState`, `RespawnAfterDeath`, `StartLevelWithResetOrAdvance` |

### VEHICLE blob  (jsonnet `VEHICLE/vehicle`, `0x8007D1D0–0x80082E90`) [R]

| VRAM start | inferred unit | anchors |
|---|---|---|
| `0x8007D1D0` [R] | `level_load.c` — load/setup level, game-mode loop, checkpoints, pause | `InitializeAndLoadLevel`, `GameModeCallback`, `SaveCheckpointState`, `PauseGameAndShowMenu` |
| `0x8007EFD0` [N] | `entity_init.c` — the `EntityType###_*_Init` spawn table (~120 fns) + `RemapEntityTypesForLevel` | `EntityType000_..._Init`, `RemapEntityTypesForLevel`, `LoadLevelSpriteAssets` |
| `0x800820B4` [N] | `main.c` — cheats, `main`, debug menu | `CheckCheatCodeInput`, `main`, `ProcessDebugMenuInput` |

### Tail game objects

| VRAM start | unit |
|---|---|
| `0x80082E90` [R] | static GameState ctor (`DestroyStaticGameState`, `CRT_ConstructGameState`) |
| `0x80082EF0` [R] | `blb_memory` (`FreeBLBMemory`) |
| `0x80082F54` [R] | libc-ish (`strcmp`,`memcpy`,`rand`,`srand`) |
| `0x80082F94` [R] | `memmove` |

---

## 3. PSY-Q libraries  (`0x80083000–0x800907CF`)

The `*_OBJ_*` symbol prefixes were named at real PSY-Q object boundaries [L]:

| VRAM start | library object(s) | markers |
|---|---|---|
| `0x80083000` | **LIBCD** — `libcd` API | `CdInit`, `CdControl`, `CdSync`, `CdIntToPos` |
| `0x80083950` | **LIBCD** — BIOS/cdrom internals | `BIOS_OBJ_*`, `CD_sync`, `CD_init`, `callback` |
| `0x800850E4` | **LIBCD** — ISO9660 / file search | `CdSearchFile`, `ISO9660_OBJ_*`, `CD_searchdir` |
| `0x80085A04` | **LIBCD** — cdread / cdread2 / cdreade | `CdRead`, `CDREAD_OBJ_*`, `CdReadFile` |
| `0x8008660C` | **LIBCD** — streaming (`St*`) + DMA | `StSetStream`, `StGetNext`, `C_011_OBJ_*`, `dma_execute` |
| `0x8008762C` | **LIBETC / libpad** — controller | `PadInit`, `PadRead`, `send_pad`, `SENDPAD_OBJ_*` |
| `0x80087C24` | **LIBETC** — vsync / interrupts / callbacks | `VSync`, `*Intr*`, `ResetCallback`, `FlushCache`, `setjmp` |
| `0x80088958` | **LIBGPU** — gpu/image/font | `SetVideoMode`, `LoadTPage`, `LoadClut`, `SetDefDrawEnv`, `FntLoad/Open/Flush` |
| `0x800892E4` | **LIBGPU** (jsonnet `LIBGPU` start) | |
| `0x8008FA60` | **LIBSPU** (jsonnet `LIBSPU` start) | |

The jsonnet's `LIBCD`/`LIBGPU`/`LIBSPU` is close but lumps libcd internals together
and omits the **LIBETC** (pad+vsync+intr) split before LIBGPU.

---

## 4. Recommendations for `SLES_010.90.jsonnet`

1. The 10-unit **rodata-anchored skeleton is correct — keep it.**
2. Subdivide the three oversized text blobs into named `asm()` (or `c()`) segments at
   the boundaries in §2: `OBJECT` → ~8 units (hud/decor/collectibles/effects/cd/movie/
   enemies/bosses); `PLAYER_STATES` → vehicle/menu/password/hud_results/ending/level/
   blb_accessors/sound/gamestate; `VEHICLE` → level_load/entity_init/main.
   Splitting at real function starts with `migrate_rodata_to_functions: false` keeps the
   single rodata segment intact and should be **byte-match-neutral** (verify with `make check`).
3. Optionally split `LIBCD` internally and add a `LIBETC` segment per §3.
4. These remain *labels*; only the boundaries are evidence-backed.
</content>

---

## 5. Split-correctness audit (2026-07-04)

Re-checked against the question "are we splitting at wrong places and reshaping the
bytes with unnatural C?" **Conclusion: the splits are structurally sound; almost all
"weird C" is genuine cc1 codegen, not split compensation.**

- **Coarse (rodata-anchored) boundaries: correct.** The 10 boundaries are
  binary-confirmed (§1). Independent checks that pass today: rodata sub-block order
  equals text order (gfx<hud<clayball<playst<vehicle<lvlload<libc in *both* sections),
  and the full link is clean — **zero** `multiple definition` / overlap / region-overflow
  errors, which a mis-placed coarse boundary would produce.
- **Fine boundaries: mostly correct, occasionally too coarse (byte-safe).** Function-level
  mis-splits do occur but are actively caught and merged in `symbol_addrs.txt` (~23
  documented, e.g. `layer.c` FindLayerSlot loop tail, `finn.c` dispatch body,
  `pickups.c` TriggerFadeOut). These do not force compensating C — the functions still
  link in the same order.

### The one real "reshaping" area: the shared `.sdata`/`.data` blobs

`.data` and `.sdata` are each a **single pooled asm segment** (`data('80FEC','data')`,
`data('96154','sdata')`), not split per owning TU. Ten game TUs reach into the sdata
blob via the **tentative-def + `--use-comm-section` gp_rel trick** (finn, gamecd, menu,
bosses, pickups, enemies, …). This yields *correct bytes* — it is a standard technique,
not a bug — but it is the "unnatural construct" the splits push onto callers. It fails
outright for **initialized** small globals (e.g. `D_800A595C = .word D_8009AE58`): the
tentative def adds storage and shifts the whole sdata segment, so `FindLayerSlot` and
~8 siblings stay shelved. The proper fix is a per-TU **data/sdata migration** (define the
global in its owning C TU, excise it from the pooled blob at its exact gp offset) — the
sdata analog of the rodata migration in `docs/plans/rodata-sdata-migration.md`.

### splat warning: `clayball` rodata jumptable alignment

`python -m splat split` emits:
> The rodata segment 'clayball' has jumptables that are not aligned properly file-wise,
> indicating one or more likely file split.  - [0x2004, rodata]

Cause (verified, byte-safe — **not a correctness bug**): the `clayball` rodata segment
(`0x80011628`–`0x80011844`) bundles **player**-owned rodata at its tail —
`g_PlayerCallbackTable` (`0x80011804`–`0x80011844`, an entity destructor/render callback
vtable) is referenced only by `CreatePlayerEntity` @ `0x800596E8` (player text unit
`0x800596A4`), via **absolute** `lui/%hi + addiu/%lo`, not gp_rel. Because the order is
still clayball-rodata < player-rodata < playst-rodata (matching clayball-text <
player-text < playst-text), the bytes are correct; only the *label* is coarse. splat's
jumptable-alignment heuristic correctly infers a missed file split at `0x2004`. To clear
the warning and set up a future `player.o(.rodata)` migration, split the `clayball`
rodata subsegment so `g_PlayerCallbackTable` (and any preceding player vtables) form a
`player` rodata segment. Byte-match-neutral; deferred (not applied this pass).
