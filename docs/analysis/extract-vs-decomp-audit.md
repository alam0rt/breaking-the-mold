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

## Decomp Priority Recommendations

1. **Entity state table parser** — understanding the `{flags, callback} + END2` format unlocks naming for ~200 state functions
2. **Depth bucket system** — small, self-contained, used everywhere
3. **Player collision dispatch** — 124-entry table drives all tile interactions
4. **Decor/particle system** — small state machine, good decomp target
5. **Entity init dispatch table** (vehicle.c INCLUDE_ASM stubs) — completes the factory layer
6. **Visual effect renderers** — large but self-contained GTE code, can be done independently
