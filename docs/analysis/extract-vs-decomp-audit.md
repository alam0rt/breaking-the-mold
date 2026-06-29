---
title: "Extract vs Decomp Audit (2026-06-14)"
category: analysis
tags: [analysis, extract, vs, decomp, audit]
---

# Extract vs Decomp Audit (2026-06-14)

Comparison of `make extract` output (splat segments, asm, data) against decompiled source to identify patterns, undocumented systems, and decomp priorities.

## Decomp Progress Summary

| Module | ROM Size | Decompiled | ASM Stubs | Status |
|--------|----------|-----------|-----------|--------|
| ENGINE (entity/render core) | ~95KB | ~160 funcs | ~73 stubs | Setters/accessors done; tick/render/collision remains |
| OBJECT (entity behaviors) | ~188KB | 0 | ~550 funcs | Untouched — largest block |
| PLAYER | ~93KB | 1 func | ~220 funcs | Nearly untouched |
| PLAYER_STATES | ~50KB | 0 | ~150 funcs | Untouched |
| BOSS | ~9KB | ~24 funcs | 21 stubs | Partially done |
| VEHICLE/MAIN (game flow) | ~27KB | ~196 funcs | 19 stubs | Mostly done |
| LIBS (LIBCD/LIBGPU/LIBSPU) | ~56KB | 3 funcs | N/A | PSY-Q, low priority |

**Total: ~384 decompiled C functions / ~1,160 total (~33%)**

---

## Undocumented Systems

### 1. Visual Effect Renderers (OBJECT segment)

Large GTE-heavy procedural rendering functions with no docs:

| Function | Size | Purpose |
|----------|------|---------|
| `RenderGridDistortionEffect` | 0xA0C (2.5KB) | Heat-haze/warp (boss arenas?) |
| `RenderRopeSegments` | 0x55C | Multi-segment rope/chain |
| `RenderPathEntitySegments` | 0x5B8 | Spline-based path rendering |
| `RenderSpotlightBeamEffect` | 0x310 | Cone/beam light |
| `RenderRippleExpandEffect` | 0x4D8 | Expanding circle/ripple |
| `Render_RotatingStarEffect` | 0x4B8 | Spinning particle star |
| `InitAuraEffectAtPlayer` | 0x2D4 | Player powerup aura |

### 2. Depth Bucket Rendering

The actual entity draw-order mechanism:
- `AddToDepthBucket` — inserts entities into depth-ordered bins
- `FlushDepthBucketsGlobal` — renders all bins back-to-front

### 3. Multi-Part Entity System

Compound object framework for segmented entities:
- `MultiPartEntityTick` (0x2D4)
- `MultiPartEntityRenderTick`
- `MultiPartEntityMessageHandler`

Used for: password screen digits, segmented enemies, possibly boss limbs.

### 4. Background Decor/Particle State Machine

Ambiance system with its own state group in the master state table:
- `DecorSetSpriteActive/Idle`, `DecorStartAnimation`, `DecorSetRandomTimer`, `DecorPlaySoundAndAnimate`
- `InitBackgroundParticleEmitter`, `ParticleEmitterTickCallback`

Likely drives: falling snow (SNOW), bubbles, ambient creatures.

### 5. Extended Menu Vtable Pattern

Standard vtables are 8 entries. Menu entities use **14-entry extended vtables** with:
- Slots 0–2: destroy/render/texture (standard)
- Slots 3–4: activate/deactivate callbacks
- Slots 5–9: event handlers (confirm, cancel, cycle, backspace, input)

Found vtables for: password entry (`D_80011ED4`), skull icon buttons (`D_80011F2C`), level select (`D_80011F84`), generic buttons (`D_80011FDC`, `D_80012034`).

### 6. Homing Projectile AI

Two ~0x4AC-byte functions implementing guided missile tracking:
- `HomingProjectileTick` — general homing behavior
- `HomingMissileTrackTarget` — target-seeking with steering

Among the largest entity tick functions. Likely boss attack projectiles.

---

## Structural Discoveries

### Entity Type Architecture: 3-Tier Dispatch

More complex than previously documented:
1. **BLB level data** → raw entity type ID
2. **`RemapEntityTypesForLevel`** → internal type (121 slots, 9 unused: types 13-16, 56, 73-74, 77-78)
3. **Init function** → variant sub-dispatch on `entity_def+0x0C`

Effective behavior count: **~200+** (exceeds the 121-slot table).

### Master Entity State Table (`0x800A598C` in sdata)

Format: `{0xFFFF0000, StateCallback}` pairs, groups terminated by `"END2"` (0x32444E45) sentinel.

**19 identified state groups:**
1. Platform states (ride start/complete)
2. Decor/background animation
3. Animation sequence triggers
4. Enemy AI (patrol, death, laser monkey, sound emitter)
5. Collectibles (idle, walk)
6. Entity utilities (show/hide/activate/disable)
7. Platform mechanics
8. BounceClay (idle, destroying)
9. Hazards (active, hide with timer)
10. Teleporters (activate, exit)
11. Glider (wake up, reset at spawn)
12. Boss generics (defeated particles, random attack)
13. ShrineyGuard boss (6 states: attack counter/death/loop/ready/anim/idle)
14. JoeHeadJoe boss (7 states: select pattern/death/enter/return/clear voice)
15. Projectiles (active, homing ×3, missile track)
16. Clayballs (indicator wait, destroy with debris)
17. **Player (40+ states)** — idle/death/run/walk/jump/fall/bounce/crouch/pickup/teleport/throw/shrink/grow/universe enema/hamster
18. Finn level entities
19. Soar/Runn level entities

### Table-Driven Physics

Frame-indexed acceleration curves in `.data`:

| Address | Table | Values |
|---------|-------|--------|
| `0x8009B038` | Wobble/oscillation | 30 signed values, growing amplitude |
| `0x8009B074` | Speed ramp | 1.0→9.0 over 18 frames (run accel) |
| `0x8009B0BC` | Gravity ramp | Caps at 10.0 after 5 frames (terminal velocity) |
| `0x8009B104` | Float ramp | Gentler 16-entry curve (gliding/floating) |

### Player Collision Dispatch (124-entry jump table)

`jtbl_800118F4` in PLAYER.rodata maps tile collision type IDs (0–123) to handler functions. Sparse — most entries hit default. Active handlers at indices 0, 7-8, 38-41, 57-58, 68-69, 78-79. Represents different floor/trigger/hazard behaviors on contact.

### Player Input Dispatch (44-entry jump table)

`jtbl_80011844` — maps button/input combinations to state transitions. Active at indices 0-2, 19, 21, 40-42.

### Entity Pool

`0x800907EC` — 42KB pre-allocated buffer (~200 entity slots), managed as freelist via heap pointer at `D_800A5954`.

---

## Debug/Hidden Features

| Feature | Address | Size | Activation |
|---------|---------|------|------------|
| Debug menu | `0x80082C10` | 0x280 | `D_800A5958` bit 0x80 (L2 during pause?) |
| Debug camera | 3 player callbacks | — | Unknown cheat sequence |
| Entity browser | Part of debug menu | — | Iterates `D_8009DDE0` |
| Cheat system | `0x800820B4` | 0x60C | 22 codes, 8-button buffer at GameState+0x17C, table at `D_8009DAE0` |
| Level debug flag | `0x80082748` | — | `GetLevelDebugFlag` |

---

## Segment Name ↔ Logical Module Mapping

| Splat Segment | Logical Module | Content |
|---------------|---------------|---------|
| `ENGINE_boot` | core/graphics | VRAM, tilemap, render init |
| `ENGINE` + `_5C3C` + `_9554` | entity/core | Entity lifecycle, collision, animation |
| `OBJECT` | entity/behaviors | All 550+ entity behavior functions |
| `BOSS` | entity/behaviors (subset) | Boss init/tick/events (overlaps OBJECT range) |
| `PLAYER` | player/main | Player FSM, physics, input |
| `PLAYER_STATES` | player/states + menus + level-specific | Mixed: player states, menu system, Finn/Soar/Runn entities |
| `VEHICLE` + `MAIN` | level/loader + game flow | Level loading, entity spawning, main(), checkpoints |

---

## Ghidra Verification (2026-06-14)

All structures below have been verified via MCP and annotated in the Ghidra database.

### Entity State Table — VERIFIED

- **Address:** `0x800A598C` (labeled `g_apfnEntityStateTable`)
- **Format confirmed:** 8-byte entries `{0xFFFF0000, callback_ptr}`, groups terminated by ASCII `"END2"`
- **15 active groups** (not 19 as initially estimated — groups 3, 16-18 are empty):
  - Group 1: Platform ride (3 entries: PlatformRideComplete, PlatformRideStartUp, PlatformRideStartDown)
  - Group 2: Decor (7: DecorSetSpriteActive/Idle, DecorStartAnimation/Alt, DecorSetRandomTimer, DecorStartWithRandomTimer, DecorPlaySoundAndAnimate)
  - Group 4: Enemy AI (17: StartAnimSequence4A/B/C, EnemyStartMovingWithSound, LaserMonkeyIdleState, ...)
  - Group 5: Entity visibility (10: EntityHideAndDisable, EntityShowAndActivate, PlatformShow/Hide, ...)
  - Group 6: Animation frames (4: StartAnimSequence13Frames, StartAnimSequence7Frames, EnemySetWalkSprite, EnemySetIdleSprite)
  - Group 7: Boss/complex enemies (32 entries, 0x800479D0-0x8004EAC8)
  - Group 8: Collectible/pickup (13 entries)
  - Group 9: Vehicle (4 entries)
  - Group 10: **Player states** (65+ entries, 0x80066CE0-0x8006EA0C — confirmed PlayerStateInit_Idle, PlayerState_HideAndClearBounce, PlayerState_SetupBounceRight/Up, etc.)
  - Group 11: 3 entries (0x8006FDF4-0x80070094)
  - Group 12: Boss patterns (10 entries)
  - Group 13-14: Boss sub-states (3+4 entries)
  - Group 15: Single entry (0x800750CC)

### Entity Type Dispatch Table — VERIFIED

- **Address:** `0x8009D5F8` (labeled `g_EntityTypeInitTable`)
- **121 entries**, 8 bytes each: `{0xFFFF0000, init_func_ptr}`
- **82 unique init functions** across 121 slots (many types share handlers)
- **9 unused slots:** types 13-16, 56, 73-74, 77-78 (all NULL)
- **Shared handlers:** e.g. EntityType000_003_004_PickupVariant_Init used by types 0,3,4; EntityType042_thru_060_SnoBlo_Init by types 42-44,53-55,60
- **Boss types:** 100=GlennYntis, 101=ShrineyGuard, 102=JoeHeadJoe, 103=Klogg

### Physics Tables — VERIFIED (16.16 fixed-point)

| Label | Address | Entries | Range | Purpose |
|-------|---------|---------|-------|---------|
| `g_WobbleTable` | `0x8009B038` | 15 | ±8.0 | Oscillation dx/dy pairs |
| `g_SpeedRampTable` | `0x8009B074` | 18 | 1.0→9.0 | Running acceleration curve |
| `g_GravityRampTable` | `0x8009B0BC` | 18 | 1.0→10.0 (caps) | Freefall terminal velocity |
| `g_FloatRampTable` | `0x8009B104` | 16 | 1.0→8.0 | Glide/float descent (gentler) |

### Vtable Patterns — VERIFIED

**Entity Object Class VTable** (`0x80010650`):
- 12 classes (A-L), 32 bytes each
- Per-class: `[NULL, NULL, NULL, destructor_fn, NULL, UpdateEntityRender, NULL, UploadEntityTextureIfDirty]`
- All classes share UpdateEntityRender (0x8001D988) and UploadEntityTextureIfDirty (0x8001E5B8)
- Destructors differ per class (EntityDestructor_Vtable0x80010870_A through _L)

**Ending Entity VTables** (`0x80011ED4`-`0x80012034`):
- **Correction:** Originally documented as "menu vtables" — actually ending-sequence entity vtables
- 5 variants with same 32-byte structure
- Destructors: EndingEntityDestroyCallback_2034_V1 through V5

**PrimObject Render VTable** (`0x80010344`):
- Non-uniform stride, mixed render callback types
- Core entries: SubmitPrimitiveBufferToGPU, RenderTilemapWithWrapAround, RenderTilemapPrimitivesWithBounds, RenderTilemapLayerWithScroll, RenderSpriteOrScaledQuad
- Entity entries share UpdateEntityRender + UpdateParallaxLayerPosition

### Depth Bucket System — VERIFIED (pre-documented in Ghidra)

- `AddToDepthBucket` (0x80081EC0): z>>1 clamped to [0,255], prepend to singly-linked list at `gameState[+0x16C][bucket]`
- `FlushDepthBuckets` (0x80081F1C): iterate 256 buckets, SetPolyGT4 each prim, AddPrim to OT at `g_pBlbHeapBase[+0xA084]+0x70`
- `FlushDepthBucketsGlobal` (0x80033D14): wrapper calling FlushDepthBuckets with global GameState

### Debug/Cheat System — VERIFIED

- `CheckCheatCodeInput` (0x800820B4): Confirmed — calls ApplyRandomRGBEffect, ResetPlayerUnlocksByLevel, PlaySoundEffect, GetLevelFlags
- `ProcessDebugMenuInput` (0x80082C10): Confirmed — hidden debug menu input handler

---

## Decomp Priority Recommendations

1. **Entity state table parser** — understanding the `{flags, callback} + END2` format unlocks naming for ~200 state functions
2. **Depth bucket system** — small, self-contained, used everywhere
3. **Player collision dispatch** — 124-entry table drives all tile interactions
4. **Decor/particle system** — small state machine, good decomp target
5. **Entity init dispatch table** (vehicle.c INCLUDE_ASM stubs) — completes the factory layer
6. **Visual effect renderers** — large but self-contained GTE code, can be done independently
