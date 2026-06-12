# Ghidra Struct Inventory & Analysis

This document catalogs all structs defined in the Ghidra project (`/Skullmonkeys/`, `/btm/`, `/game/` categories) with their field layouts, documentation status, and analysis notes.

**Last updated:** 2026-06-12  
**Source:** Ghidra MCP export from `skullmonkeys` project, program `SLES_010.90`

---

## Struct Categories

| Category | Structs |
|----------|---------|
| `/Skullmonkeys/` | Entity, BasicEntity, GameState, PlayerEntity, FinnPlayerEntity, RunnPlayerEntity, SoarPlayerEntity, PathEnemyEntity, SpriteEntity, PlayerCallbackTable, EntityCallbackTableBase, EntityDefinition, EntityTypeCallback, EntityListNode, MenuEntityBase, BasicEntityVtable, BasicEntityVtableShort, CheatCode, InputState, PlayerState, LevelDataContext, ScrollingLayerData, VRAMSlotConfig, TriggerZoneRGB |
| `/btm/` | SoundEntry, SpriteContext, SpriteFrameEntry, SpriteHeader, SpriteTOCEntry |
| `/game/` | SpriteTypeCallbackEntry |

---

## Entity Hierarchy (Inheritance Pattern)

```
Entity (128 bytes, 0x80)            -- Base game entity with FSM, physics, collision
└── SpriteEntity (256 bytes, 0x100) -- Entity + full animation state machine
    ├── PlayerEntity (436 bytes, 0x1B4)     -- Main player; alloc 0x1B4
    ├── FinnPlayerEntity (276 bytes, 0x114) -- Vehicle player; alloc 0x114
    ├── RunnPlayerEntity (272 bytes, 0x110) -- Auto-scroller runner; alloc 0x110
    ├── SoarPlayerEntity (296 bytes, 0x128) -- Flying player; alloc 0x128
    ├── GlidePlayerEntity (284 bytes, 0x11C)-- Glide platformer; alloc 0x11C
    │                                          NOTE: Ghidra currently types this as
    │                                          SoarPlayerEntity (CreateGlidePlayerEntity
    │                                          @ 0x8006EDB8 reuses FinnMainTickHandler
    │                                          + most Soar fields). Distinct struct TBD.
    └── PathEnemyEntity (288 bytes, 0x120)  -- Path-following enemy; alloc 0x130

BasicEntity (16 bytes, 0x10)        -- Minimal renderable object (UI/sprites)
MenuEntityBase (28 bytes, 0x1C)     -- UI menu element with FSM
```

Allocation sizes confirmed from `SpawnPlayerAndEntities @ 0x8007DF38`:
- Normal player: `AllocateFromHeap(heap, 0x1B4, 1, 0)` → `CreatePlayerEntity`
- FINN vehicle: `AllocateFromHeap(heap, 0x114, 1, 0)` → `CreateFinnPlayerEntity`
- RUNN runner: `AllocateFromHeap(heap, 0x110, 1, 0)` → `CreateRunnPlayerEntity`
- SOAR flyer: `AllocateFromHeap(heap, 0x128, 1, 0)` → `CreateSoarPlayerEntity`
- Glide: `AllocateFromHeap(heap, 0x11C, 1, 0)` → `CreateGlidePlayerEntity`
- Results/ending: `AllocateFromHeap(heap, 0x158, 1, 0)` → `CreateResultsScreenEntity`
- Menu: `AllocateFromHeap(heap, 0x140, 1, 0)` → `InitMenuEntity`
- Camera: `AllocateFromHeap(heap, 0x10C, 1, 0)` → `CreateCameraEntity`

All extended types share the Entity base (0x00-0x7F) and the SpriteEntity animation fields (0x80-0xFF). Type-specific fields begin at offset 0x100.

---

## Structs Synced to Source Headers (2026-06-12)

All structs in this section are now defined in `include/Game/entity.h` (plus
`trigger_zone.h` and `vram_slot.h`), with sizes and offsets verified against
the Ghidra layouts by compile-time checks (MIPS cross-gcc, sizeof/offsetof
asserts). The headers are the source of truth for field layout; the notes
below record behavioral findings.

### SpriteEntity (256 bytes) — Animation State Machine — ✅ in entity.h

The bridge between `Entity` (0x80 bytes) and all extended entity types. Contains the full sprite animation system. Initialized by `InitEntitySprite @ 0x8001C720` which clears 0x44C bytes total (the SpriteEntity + workspace).

**OFFSET CORRECTION (2026-06-12):** an earlier revision of this document
placed `pFrameTable`/`unknownPtr84` at SpriteEntity+0x80/+0x84. The actual
Ghidra layout (confirmed via `get_struct_layout` and `UpdateEntityRender
@ 0x8001D988`) has the frame-table pointers at **Entity+0x78/+0x7C** (the
tail of the Entity base, already in `entity.h`), bytes 0x80-0x87 unobserved
(zeroed at init, no reader identified), and the animation fields starting at
**0x88** (`frameCount`). See `include/Game/entity.h` for the full layout.

**Notes:**
- `InitEntitySprite` clears 0x44C bytes — the extra 0x34C is workspace for extended types
- 152 xrefs to `InitEntitySprite` — every visible game entity uses this

---

### PlayerEntity (436 bytes, 0x1B4) — Main Platformer Player — ✅ in entity.h

The standard platformer/glide player. Created by `CreatePlayerEntity`. Extends SpriteEntity with physics, input handling, powerups, and HUD linkage.

```c
typedef struct PlayerEntity {
    /* 0x00-0xFF: SpriteEntity base */
    SpriteEntity sprite;
    
    /* === Player-Specific Fields (0x100+) === */
    
    /* Input system (0x100-0x10B) */
    /* 0x100 */ InputState *pInput;          /* Controller input pointer */
    /* 0x104 */ s32      inputStateMarker;   /* Input FSM marker */
    /* 0x108 */ void    *inputStateCallback; /* Input handler callback */
    
    /* Physics (0x110-0x11F) */
    /* 0x110 */ s32      velocityY_fixed;    /* Y velocity (fixed-point, separate from base) */
    /* 0x114 */ s32      velocityX_fixed;    /* X velocity (fixed-point, separate from base) */
    /* 0x118 */ s32      cushionVelY;        /* Landing cushion velocity */
    /* 0x11C */ u8       landingTimer;       /* Landing recovery timer */
    /* 0x11E */ u8       counter11E;         /* General-purpose counter */
    /* 0x11F */ u8       jumpHoldCounter;    /* Jump button hold duration */
    
    /* Speed parameters (0x120-0x12B) */
    /* 0x120 */ s32      altSpeed;           /* Alternate movement speed */
    /* 0x124 */ s32      maxVelocity;        /* Maximum velocity cap */
    /* 0x128 */ u8       invincibilityTimer; /* Invincibility countdown (damage flash) */
    
    /* State flags (0x134-0x13F) */
    /* 0x134 */ u8       flag134;            /* State flag (purpose TBD) */
    /* 0x135 */ u8       flag135;            /* State flag (purpose TBD) */
    /* 0x136 */ s16      apexVelocity;       /* Jump apex velocity threshold */
    /* 0x13C */ u8       timer13C;           /* Multi-purpose timer */
    
    /* Powerup system (0x144-0x14F) */
    /* 0x144 */ s16      powerupTimer;       /* Powerup duration countdown */
    /* 0x14C */ void    *hudEntity;          /* Pointer to HUD entity (0x7530 alloc) */
    
    /* Jump/movement (0x156-0x163) */
    /* 0x156 */ s16      jumpParam;          /* Jump force parameter */
    /* 0x159 */ u8       pendingStateChange; /* Queued state transition flag */
    /* 0x15A */ u8       currentRGB[3];      /* Current entity RGB tint */
    /* 0x15D */ u8       baseRGB[3];         /* Base/default RGB tint */
    /* 0x160 */ s16      pushX;              /* External push force X */
    /* 0x162 */ s16      pushY;              /* External push force Y */
    
    /* Linked entities (0x168-0x173) */
    /* 0x168 */ void    *haloEntity;         /* Halo visual effect entity */
    /* 0x16C */ void    *glideEntity;        /* Glide cape entity */
    /* 0x170 */ u8       groundedFlag;       /* On ground (0=airborne, 1=grounded) */
    
    /* Audio (0x174-0x177) */
    /* 0x174 */ s32      soundHandle;        /* Active SPU sound handle */
    
    /* Rendering flags (0x178-0x17F) */
    /* 0x178 */ u8       disableScale;       /* Disable scale effects */
    /* 0x17D */ u8       rgbCooldown;        /* RGB effect cooldown timer */
    
    /* Scroll/camera (0x1A6-0x1A9) */
    /* 0x1A6 */ s16      scrollFlagX;        /* Camera scroll influence X */
    /* 0x1A8 */ s16      scrollFlagY;        /* Camera scroll influence Y */
    
    /* Damage/effects (0x1AE-0x1B3) */
    /* 0x1AE */ u8       damageFlag;         /* Taking damage flag */
    /* 0x1AF */ u8       particleFlag;       /* Particle effect active */
    /* 0x1B0 */ u8       shrinkFlag;         /* Shrink powerup active */
    /* 0x1B3 */ u8       gameMode;           /* Current game mode identifier */
} PlayerEntity;  /* Size: 0x1B4 (436 bytes) */
```

**Notes:**
- Many fields have gaps (Ghidra shows non-contiguous offsets) — likely unnamed padding or arrays
- `allocSize` is set to 0x3E8 (1000 bytes) even though defined fields only reach 0x1B4 — extra space used as scratchpad
- Created by `CreatePlayerEntity` (normal) and `CreateGlidePlayerEntity` (glide variant)

---

### FinnPlayerEntity (276 bytes, 0x114) — Vehicle/Boat Player — ✅ in entity.h

Tank-control vehicle for water/fish levels (level flag 0x0400). Uses rotation-based movement.

```c
typedef struct FinnPlayerEntity {
    /* 0x00-0xFF: SpriteEntity base */
    SpriteEntity sprite;
    
    /* === Finn-Specific Fields (0x100+) === */
    /* 0x100 */ InputState *pInput;         /* Controller input pointer */
    /* 0x104 */ SpriteEntity *pWakeEntity;  /* Secondary sprite (wake/shadow visual) */
    /* 0x108 */ u32      state108;          /* Internal state counter/timer */
    /* 0x10C */ u8       flag10C;           /* Rotation/movement mode flag */
    /* 0x110 */ s32      soundHandle;       /* Active SPU voice handle */
} FinnPlayerEntity;  /* Size: 0x114 (276 bytes) */
```

**Notes:**
- Created by `CreateFinnPlayerEntity @ 0x80074100`
- Secondary sprite (wake) created with same sprite ID, z-order 0x3E9 (just behind player at 1000)
- Tank controls: rotation angle stored at +0x10C, forward/backward movement
- `allocSize` set to 1000 (0x3E8) — extra space for rotation tables and state

---

### RunnPlayerEntity (272 bytes, 0x110) — Auto-Scroller Runner — ✅ in entity.h

Automatic forward-movement player for RUNN levels (level flag 0x0100). Level 22 confirmed.

```c
typedef struct RunnPlayerEntity {
    /* 0x00-0xFF: SpriteEntity base */
    SpriteEntity sprite;
    
    /* === Runn-Specific Fields (0x100+) === */
    /* 0x100 */ InputState *pInput;     /* Controller input pointer */
    /* 0x104 */ s32      velocityX;     /* Runner X velocity (fixed-point) */
    /* 0x108 */ s32      velocityY;     /* Runner Y velocity (fixed-point) */
    /* 0x10C */ s32      spuVoice;      /* SPU voice handle (init -1) */
} RunnPlayerEntity;  /* Size: 0x110 (272 bytes) */
```

**Notes:**
- Created by `CreateRunnPlayerEntity @ 0x80073934`
- NO secondary sprite (unlike FINN)
- Limited controls: Triangle=adjust left, X=adjust right, D-Pad=jump
- SPU voice initialized to -1 (no sound)

---

### SoarPlayerEntity (296 bytes, 0x128) — Flying Player — ✅ in entity.h

Flying mode player for SOAR levels (level flag 0x0010). Creates child particle/shadow entities.

```c
typedef struct SoarPlayerEntity {
    /* 0x00-0xFF: SpriteEntity base */
    SpriteEntity sprite;
    
    /* === Soar-Specific Fields (0x100+) === */
    /* 0x100 */ InputState *pInput;     /* Controller input pointer */
    /* 0x104 */ s32      nFlightSpeed;  /* Current flight speed (0 at init) */
    /* 0x10C */ s32      handle10C;     /* SPU handle (init -1) */
    /* 0x112 */ u8       stateTimer;    /* State transition timer */
    /* 0x118 */ u8       flag118;       /* Movement flags (all zeroed at init) */
    /* 0x119 */ u8       flag119;       /* */
    /* 0x11A */ u8       flag11A;       /* */
    /* 0x11B */ u8       flag11B;       /* */
    /* 0x11C */ u8       flag11C;       /* */
    /* 0x11D */ u8       flag11D;       /* */
    /* 0x11E */ u16      counter11E;    /* Frame counter / timer */
    /* 0x120 */ u16      counter120;    /* Secondary counter */
    /* 0x122 */ u8       rgb[3];        /* Entity RGB tint (init 0x40,0x40,0x40) */
} SoarPlayerEntity;  /* Size: 0x128 (296 bytes) */
```

**Notes:**
- Created by `CreateSoarPlayerEntity @ 0x80070D68`
- Creates 2 child entities: secondary sprite (z=2000) + particle entity (z=0x3D4)
- RGB tint initialized to gray (0x40, 0x40, 0x40)
- Camera offset: -0x80 Y (camera positioned higher for aerial view)
- 6 flags at 0x118-0x11D suggest 6-state flight control

---

### PathEnemyEntity (288 bytes, 0x120) — Path-Following Enemy — ✅ in entity.h

Enemy that follows predefined waypoint paths. Used by entity types 076 and 084.

```c
typedef struct PathEnemyEntity {
    /* 0x00-0xFF: SpriteEntity base */
    SpriteEntity sprite;
    
    /* === Path-Specific Fields (0x100+) === */
    /* 0x104 */ s32      eventStateMarker;   /* Event state FSM marker */
    /* 0x108 */ void    *eventStateCallback; /* Event state handler */
    /* 0x110 */ s16      pathOriginX;        /* Path origin X (base offset) */
    /* 0x112 */ s16      pathOriginY;        /* Path origin Y (base offset) */
    /* 0x114 */ void    *pWaypoints;         /* Waypoint array (short pairs: X, Y) */
    /* 0x11C */ s16      waypointCount;      /* Number of waypoints in path */
    /* 0x11E */ u16      pathTimer;          /* Current path interpolation time */
} PathEnemyEntity;  /* Size: 0x120 (288 bytes) */
```

**Notes:**
- Created via `EntityType076_PathEnemy_Init @ 0x8007F3DC` → `InitPathFollowingEnemy @ 0x8003C5B8`
- Allocation size: 0x130 (304 bytes, slightly larger than struct for workspace)
- Path system: waypoints loaded from spawn data (+0x0C variant field)
- Looping detection: if first==last waypoint, sets loop flag and decrements count
- Frame counter modulo used to stagger multiple enemies on same path
- 6 xrefs — primary factory for path-following enemies
- Sound variants selected by entity subtype (5 sound profiles identified)

---

### BasicEntityVtable (24 bytes) — Render Object Vtable — ✅ in entity.h

Callback table for `BasicEntity` objects (sprite contexts, UI elements).

```c
typedef struct {
    /* 0x00 */ u32  unused_00;              /* Always 0 (padding for dispatch alignment) */
    /* 0x04 */ u32  unused_04;              /* Always 0 */
    /* 0x08 */ EntityCallbackSlot render;   /* Render/draw callback */
    /* 0x10 */ EntityCallbackSlot freeResource; /* Resource cleanup callback */
} BasicEntityVtable;  /* Size: 0x18 (24 bytes) */
```

### BasicEntityVtableShort (16 bytes) — Minimal Vtable — ✅ in entity.h

Minimal vtable with only a render callback (no cleanup needed).

```c
typedef struct {
    /* 0x00 */ u32  unused_00;
    /* 0x04 */ u32  unused_04;
    /* 0x08 */ EntityCallbackSlot render;   /* Render callback only */
} BasicEntityVtableShort;  /* Size: 0x10 (16 bytes) */
```

---

### TriggerZoneRGB (3 bytes) — Color Tint Triple — ✅ in trigger_zone.h

Simple RGB color struct used in arrays for trigger zone and layer tinting.

```c
typedef struct {
    /* 0x00 */ u8  r;
    /* 0x01 */ u8  g;
    /* 0x02 */ u8  b;
} TriggerZoneRGB;  /* Size: 0x03 */
```

**Usage:** Arrays of 20-22 entries (`TriggerZoneRGB[20]`, `TriggerZoneRGB[22]`) for per-zone color data.

---

### SpriteTypeCallbackEntry (16 bytes) — Sprite Type Dispatch Table — ✅ in sprite.h

Per-sprite-type callback table with 4 function pointers. Array of 481 entries at the sprite type dispatch table.

```c
typedef struct {
    /* 0x00 */ void *callback_0;      /* Primary callback (sprite load?) */
    /* 0x04 */ void *callback_1;      /* Secondary callback (update?) */
    /* 0x08 */ void *tick_callback;   /* Per-frame tick handler */
    /* 0x0C */ void *callback_3;      /* Cleanup/finalize? */
} SpriteTypeCallbackEntry;  /* Size: 0x10 (16 bytes) */
```

**Notes:**
- 481 entries × 16 bytes = 7696 bytes total
- Individual callback purposes need further analysis (observe which get called during sprite lifecycle)

---

## Existing Structs with Undocumented Fields

### SpriteHeader — Unknown Fields

| Offset | Field | Status | Notes |
|--------|-------|--------|-------|
| 0x14 | `unknown_flag` | ❓ Unknown | 2 bytes, purpose unclear. Possibly RLE format flag |
| 0x16 | `clut_garbage` | ❓ Unknown | 2 bytes, may be stale CLUT reference or padding |

### SpriteFrameEntry — Unknown Field

| Offset | Field | Status | Notes |
|--------|-------|--------|-------|
| 0x10 | `unknown_10` | ❓ Unknown | 4 bytes between flip_flags and hitbox. Could be texture UV data or animation timing metadata |

### SpriteContext — Unknown Field

| Offset | Field | Status | Notes |
|--------|-------|--------|-------|
| 0x12 | `unknown_12` | ❓ Unknown | 1 byte before `is_valid`. Could be decode state or bank index |

### GameState — Unknown Fields (0x10-0x18)

| Offset | Field | Status | Notes |
|--------|-------|--------|-------|
| 0x10 | `unknown10` | ❓ Disproved | NOT layer list heads. Old claim disproved by `AddLayerToRenderList_Medium` analysis |
| 0x14 | `unknown14` | ❓ Disproved | Need to find actual readers/writers |
| 0x18 | `unknown18` | ❓ Disproved | Need to find actual readers/writers |

**Investigation approach:** Search for `lw $reg, 0x10($gamestate)` / `sw` patterns in disassembly, or use Ghidra xrefs on GameState+0x10.

---

## Trigger Zone System (Undocumented)

Discovered via `CheckTriggerZoneCollision @ 0x800245BC`. This is a complete spatial trigger system:

**Zone Data Format (16 bytes per zone = 8 shorts):**
```
Offset 0x00: s16 left        (world X min)
Offset 0x02: s16 top         (world Y min)  
Offset 0x04: s16 right       (world X max)
Offset 0x06: s16 bottom      (world Y max)
Offset 0x08: s32 attr_data   (trigger type/attributes)
Offset 0x0C: s32 extra_data  (payload: target level, color, etc.)
```

**Storage:** Pointer at `GameState+0x74` (`trigger_zone_data_ptr`), count at `GameState+0x78` (`trigger_zone_count`).

**CORRECTION APPLIED (2026-06-12):** `game_state.h` previously mislabeled these
fields as `animated_tile_data_ptr` / `scroll_limit_height`; both the header and
the Ghidra GameState struct now use the trigger-zone names, and the zone record
layout lives in `include/Game/trigger_zone.h` (`TriggerZone`, 16 bytes).

**Evidence chain:**
1. `InitLayersAndTileState @ 0x80024778` stores `GetLevelDataContextField3C()` → GS+0x74, and `GetAsset100Field1C()` → GS+0x78
2. `CheckTriggerZoneCollision @ 0x800245BC` reads GS+0x74 as zone array pointer and GS+0x78 as count
3. 10 callers (including `EntityCheckTriggerZone`, `PlayerProcessTileCollision`) all pass `(int)g_pGameState` as first arg
4. The data format at GS+0x74 is 16-byte zone structs (4 shorts for AABB + 2 ints for data)

So: `LevelDataContext+0x3C` contains the **trigger zone definitions** (not animated tile offsets), and `TileHeader+0x1C` is the **trigger zone count** (not scroll limit height).

**Related functions (10 total):**
- `CheckTriggerZoneCollision @ 0x800245BC` — Core AABB test
- `EntityCheckTriggerZone @ 0x80056478`
- `EntityPathFollowerWithTriggerZone @ 0x8003F400`
- `FinnCheckTriggerZones @ 0x80070128`
- `HandleGenericTriggerZone @ 0x8007EE6C`
- `InitTriggerZoneEntity @ 0x800441AC`
- `PlatformEntityCheckTriggerZones @ 0x800722EC`
- `SpawnEntityOrTriggerZone @ 0x8002FF88`
- `UpdateCollectibleTriggerZone @ 0x8003A9B4`
- `EntityType085_104_105_TriggerZone_Init @ 0x800812EC`

---

## Summary: Action Items

### Done (2026-06-12 header sync)
1. ~~**Add SpriteEntity to `entity.h`**~~ ✅ Added, with offset correction (anim fields start 0x88)
2. ~~**Verify GameState 0x74/0x78**~~ ✅ Trigger zones confirmed via `CheckTriggerZoneCollision`; header + Ghidra renamed; `TriggerZone` added in `trigger_zone.h`
3. ~~**Add player variant structs**~~ ✅ Finn/Runn/Soar in `entity.h`, sizes compile-checked
4. ~~**Add PathEnemyEntity**~~ ✅ In `entity.h`
5. ~~**Add BasicEntityVtable/VtableShort**~~ ✅ In `entity.h` (FSMStateSlot added too)
6. ~~**Add TriggerZoneRGB**~~ ✅ In `trigger_zone.h`; `VRAMSlotXY` added to `vram_slot.h`

### Open
1. **Verify GameState 0x10-0x18** — Still unknown. The Ghidra struct carries
   names `layer_list_parallax`/`layer_list_standard`/`layer_render_context_ptr`,
   but these reflect the DISPROVED layer-list model (layers actually insert
   into the shared entity lists at +0x1C/+0x20 — see
   `AddLayerToRenderList_Medium` plate). Treat the Ghidra names as stale;
   the header keeps `unknown10/14/18` until actual readers are found.
2. **Document SpriteTypeCallbackEntry callbacks** — Find what calls each slot
3. **Define GlidePlayerEntity** — 0x11C alloc; currently typed as SoarPlayerEntity in Ghidra (see follow-ups doc)
4. **Investigate SpriteHeader unknown_flag/clut_garbage** — May require runtime tracing
5. **Investigate SpriteFrameEntry unknown_10** — Check RLE decode functions

---

## Undefined-Typed Data Survey (2026-06-12)

Ran an inline GhidraScript scan of all defined data items with type `undefined1`/`undefined2`/`undefined4`/`undefined8`. These are placeholders Ghidra creates when a memory address is "defined" but no concrete type is applied. **624 of 1777 defined items (35%) are still untyped** — a major source of "magic" addresses across the codebase.

### Breakdown by type
| Type | Count |
|------|-------|
| `undefined1` | 84 |
| `undefined2` | 93 |
| `undefined4` | 447 |
| `undefined8` | 0 |
| **total** | **624** |

### High-impact untyped globals (≥10 xrefs)

| Address | Type | Xrefs | Current name | Likely identity |
|---------|------|------:|--------------|-----------------|
| `0x800907EC` | undefined2 | **784** | `null_0000h_800907ec` | **GameState-shaped singleton.** Passed as first arg to `AddToXPositionList(GameState*, …)`, `InitTilemapLayerRendering`, `AllocateSpriteContext`, `CreateMenuEntities`, `PrepareSpriteVRAMSlotForContext`, `InitMenuStage2`. `+0x20` is read as `render_list_head`, `+0x34` as a sprite-context slot, `+0x38..+0x40` as tilemap params. **Hypothesis:** secondary GameState/scene-context struct used during menu/title/tilemap setup, distinct from `g_GameStateBase @ 0x8009DC40`. Probably the **embedded TilemapLayerRenderState** or a global scratch `GameState` for menu UI. |
| `0x800907EE` | undefined2 | 46 | `null_0000h_800907ee` | `0x800907EC + 2` — *same struct*, just a different field. All reads from `InitTilemapLayerRendering`, `RenderTilemapSprites16x16`, `SetCameraPositionDirect`, `CreateMenuEntities` (×11). |
| `0x8009A870` | undefined4 | 47 | `null_00000000h_8009a870` | **GPU primitive list pointer / OT-head.** Read-only by every major renderer: `RenderTilemapWithWrapAround`, `RenderSpriteOrScaledQuad`, `SetupTilemapPrimitives`, `RenderTilemapPrimitivesWithBounds`, `SubmitPrimitiveBufferToGPU`, `RenderTilemapSprites16x16`. Likely `g_pCurrentPrimListHead` or `g_pRenderHeapEnd`. |
| `0x8009B1D8`–`0x8009B1F5` | undefined1[~30] | 8–29 ea. | `null_00h_*` | **`PlayerState` struct (CONFIRMED).** `symbol_addrs.txt` declares `g_pPlayerState = 0x800A597C` with comment "Pointer to PlayerState structure (points to 0x8009B1D8)". `ResetPlayerUnlocksByLevel @ 0x80026B18` writes `g_pPlayerState[0x14/0x15/0x16/0x1C] = 0`. Initialised by `initPlayerState @ 0x800260D0` (size 0x8C). **Action:** define `PlayerState` struct in Ghidra and apply at `0x8009B1D8`. |
| `0x800A5988` | undefined4 | 27 | `null_FFFF0000h_800a5988` | **FSM marker** (sentinel `0xFFFF0000` matches Entity/GameState `mode_base_offset` pattern). Pair start of a tagged-union `[marker, fn]` slot. |
| `0x800A0190` | undefined4 | 23 | `null_00000000h_800a0190` | Untyped subsystem state pointer. |
| `0x8009DF24` | undefined4 | 22 | `null_00000000h_8009df24` | Untyped subsystem state. |
| `0x8009E248` | undefined4 | 22 | `null_00000000h_8009e248` | Untyped subsystem state. |
| `0x800ACBB0` | undefined4 | 22 | `null_??_800acbb0` | BSS/work area. |
| `0x800A018C` | undefined4 | 21 | `null_00000000h_800a018c` | Untyped subsystem state. |
| `0x8009DF18` | undefined4 | 20 | `null_00000000h_8009df18` | Untyped subsystem state. |
| `0x800A5A38` | undefined4 | 18 | `null_00000000h_800a5a38` | sdata word. |
| `0x800A85F0` | undefined4 | 15 | `null_??_800a85f0` | BSS/work area. |
| `0x8009F628` | undefined4 | 14 | `null_00000000h_8009f628` | Untyped subsystem state. |
| `0x800A5958` | undefined2 | 13 | `g_GameFlags` | **Already named**, but typed as `undefined2` — should be `u16` with a named enum/bitfield of game flags (bits `2` and `4` checked in `main` for 30fps cap). |
| `0x800ACBB0`/`0x800AE3B8`/`0x800AE3C8` | undefined4 | 13–22 | `null_??_*` | Large BSS structs needing identification. |
| `0x800A5988`/`0x800A5DB0`/`0x800A5DB8`/`0x800A5D30`/`0x800A5F04`/`0x800A5F14`/`0x800A5FBC`/`0x800A5E00` | undefined4 | 7–27 | `null_FFFF0000h_*` | **All match the `mode_base_offset = 0xFFFF0000` FSM-marker pattern.** Each is the first word of a 2-word tagged callback slot. The matching `null_00000000h_` neighbours at +4 are the function pointer halves. **Hypothesis:** subsystem-mode FSM tables (e.g. `g_GameModeTable`, `g_AudioModeTable`, etc.). |
| `g_DefaultBGColorR/G/B` (`0x800A596C/E/70`) | undefined2 | 24/11/11 | named | **Already named**, just typed as raw `undefined2`. Should be `u8` (only low byte used). |
| `g_SPUUploadBlockCount`/`g_SoundTableCount`/`g_DebugMenuCursor` | undefined1/4/2 | 8–9 | named | Same — named but untyped. |

### Adjacent-cluster patterns

The `0x8009B1D8..0x8009B1F5` byte-by-byte cluster is the giveaway pattern: when many adjacent addresses are independently typed as `undefined1`, they are almost certainly fields of a single struct that was never laid down. Other clusters worth investigating:

- `0x800A6168..0x800A6198` — mix of `undefined1`/`undefined4`/`undefined2` with 8–13 xrefs each. Likely another singleton struct (`g_DebugMenuState` candidate given `g_DebugMenuCursor @ 0x800A60BA` is nearby).
- `0x800A008A..0x800A008E` — `undefined1`/`undefined1`/`undefined2`/`undefined2` clustered with 8–16 xrefs.
- `0x800A5A1A..0x800A5A44` — `undefined1`/`undefined4`/`undefined2`/`undefined4` cluster with `FFFF0000h` initializers → another FSM-slot table.

### Action Items (added to backlog)

1. **Define `PlayerState` struct** (HIGH) — Apply at `0x8009B1D8`. Header `include/Game/player_state.h` does not exist yet. Trace `initPlayerState @ 0x800260D0` to derive field layout.
2. **Identify `0x800907EC` singleton** (HIGH) — Decompile callers; determine whether it's `g_MenuGameState`, `g_TilemapLayerScratch`, or a true `GameState` alias. Apply correct type & rename.
3. **Identify `0x8009A870` render pointer** (MEDIUM) — Cross-check with `g_pBlbHeapBase + 0xa084` which `RenderSpriteOrScaledQuad` uses as the "current primitive list" lookup. May be redundant.
4. **Type known-named globals** (LOW, mechanical) — `g_GameFlags`, `g_DefaultBGColorR/G/B`, `g_SoundTableCount`, `g_SPUUploadBlockCount`, `g_DebugMenuCursor` — change from `undefined*` to proper int types (and add bitfield/enum where appropriate).
5. **Sweep `null_FFFF0000h_*` clusters** (MEDIUM) — These are all FSM slot heads. Build a small inventory of subsystem mode tables and type each as a `[s32 marker; void* fn;]` pair (matches `mode_base_offset`/`mode_callback_ptr` in GameState).

### Survey method

Inline GhidraScript via `POST /run_script_inline` (gated by `GHIDRA_MCP_ALLOW_SCRIPTS=1`):
`McpUndefinedSurvey.java` iterates `Listing.getDefinedData(true)`, filters by `DataType.getName() in {undefined,undefined1,undefined2,undefined4,undefined8}`, collects xref counts via `ReferenceManager.getReferencesTo(addr)`, sorts by xref count desc. Restricted to RAM range `0x80010000..0x80100000`.

---

## Undefined Type Resolution Pass (2026-06-12)

After the survey, drilled into each high-xref item and applied types in Ghidra. **Net result: 624 → 391 undefined items (233 cleared, 37%).**

### Findings & resolutions

#### 1. PlayerState @ 0x8009B1D8 (HIGH-VALUE)
**Status: RESOLVED.** The 30-byte `PlayerState` struct was already fully defined in `/Skullmonkeys/PlayerState` (with all 28 fields named and described — `lives`, `orb_count`, `phoenix_hands`, `phart_heads`, `universe_enemas`, `super_willies`, `swirly_q_count`, `total_swirly_qs`, `clayball_flag_0..9`, `powerup_flags`, `shrink_mode`, `icon_1970_count`, `hamster_count`, etc.). It just was **never applied** at the data address. Applied via `mcp_ghidra-new_apply_data_type(0x8009B1D8, PlayerState)`. This cleared ~25 undefined1 entries instantly. The pointer `g_pPlayerState @ 0x800A597C` was also re-typed from raw `pointer` to `PlayerState *`.

**Source-header gap:** `include/Game/player_state.h` does not exist. Should be created mirroring the Ghidra struct (existing `PlayerCallbackTable` in Ghidra is unrelated — that's the FSM dispatch table at `0x800A5D20`).

#### 2. 0x800907EC mystery (784 xrefs) → g_BlbHeapBase
**Status: RESOLVED.** This is **NOT** a struct — it's the base of the 86 KB **BLB heap / working RAM region** that begins at the start of `.data` (block `0x800907EC..0x800A5953`). The 784 xrefs were dereference-tracking artifacts: when Ghidra sees the pattern `lui $a0,0x800a; lw $a0,0x5954($a0)` (= load value at `0x800A5954` = the pointer `g_pBlbHeapBase`), it follows the pointer to `0x800907EC` and creates a PARAM ref there. Every `AllocateFromHeap(g_pBlbHeapBase, …)` callsite generates one — and the codebase has hundreds.

**Memory map clarified:**
| Range | Symbol | Type |
|-------|--------|------|
| `0x800907EC..0x800A5953` | `g_BlbHeapBase` (heap region itself, in `.data`) | 86 KB scratch RAM (initially all zeros, populated at boot) |
| `0x800A5954..0x800A611F` | `.sdata` (GP-relative) — `g_pBlbHeapBase` is the FIRST sdata symbol (matches `gp_value: 0x800A5954` in splat YAML) | primitive globals |
| `0x800A6120..0x800AD953` | `.sbss` | GP-relative BSS |
| `0x800AD954..0x800AE397` | `.bss` | regular BSS |

Action: labeled `0x800907EC` as `g_BlbHeapBase` with plate comment. The "+0xA084 → render-context-pointer" finding (i.e. `0x8009A870`) is now understood as a field inside the heap, not a separate global. Many of the other 0x80090000+ untyped items in the original list are similarly heap-internal offsets — not actionable as top-level globals.

#### 3. FSMStateSlot pattern (`null_FFFF0000h_*`)
**Status: RESOLVED — biggest single cleanup.** Confirmed the hypothesis: every `null_FFFF0000h_*` is the first word of an 8-byte `[s32 marker=0xFFFF0000, void *fn]` pair passed to `EntitySetState(entity, &slot.marker, slot.fn)` (and similar dispatchers). The fn-ptr halves were already named like `PTR_PlayerStateCallback_2_800a5d34`. Defined struct `/Skullmonkeys/FSMStateSlot { s32 marker; void *fn; }` and applied at every matching address in the binary (scanned all defined `undefined4` for value `0xFFFF0000` followed by a code-range pointer). **184 slots applied; 368 undefined4 entries cleared in one transaction.** Examples (representative):

| Slot addr | Fn ptr | Name |
|-----------|--------|------|
| `0x800a5988` | `PlatformRideComplete` | menu HUD ride exit |
| `0x800a5d30` | `PlayerStateCallback_2` | player physics step |
| `0x800a5db0` | `PlayerState_PickupItem` | item pickup state |
| `0x800a5db8` | `PlayerState_LevelExitTeleporter` | exit teleport |
| `0x800a5e00` | `PlayerStateInit_BounceFromEnemy` | bounce init |
| `0x800a5f04` | `PlayerState_ClearInAirFlag` | in-air clear |
| `0x800a5f14` | `PlayerState_ClearBounceFlag` | bounce flag clear |
| `0x800a5fbc` | `RunnEntityDamageEffect` | RUNN damage |

The first 8 slot symbols were also renamed `fsmSlot_<fnname>` for readability.

#### 4. Already-named globals (mechanical type fix)
**Status: RESOLVED.** Eight globals that already had descriptive names but `undefined*` types — applied proper integer types and added plate comments:

| Addr | Name | Type |
|------|------|------|
| `0x800A5958` | `g_GameFlags` | `u16` (bit 0x04 = 30fps cap) |
| `0x800A596C` | `g_DefaultBGColorR` | `u8` |
| `0x800A596E` | `g_DefaultBGColorG` | `u8` |
| `0x800A5970` | `g_DefaultBGColorB` | `u8` |
| `0x800A597C` | `g_pPlayerState` | `PlayerState *` |
| `0x800A6078` | `g_SoundTableCount` | `u32` |
| `0x800A6081` | `g_SPUUploadBlockCount` | `u8` |
| `0x800A60BA` | `g_DebugMenuCursor` | `u16` |

Plus five more named via filtered scan:

| Addr | Name | Type |
|------|------|------|
| `0x800A595B` | `g_SkipVSync` | `u8` |
| `0x800A60C0` | `g_TotalMenuItems` | `u8` |
| `0x800A6087` | `g_AudioMuted` | `u8` |
| `0x800A60B8` | `g_DebugMenuScrollTop` | `u16` |
| `0x800A6120` | `g_pCurrentInputState` | `InputState *` |

#### 5. Movie streaming state @ 0x800A5A1A..0x800A5A44
**Status: RESOLVED.** Cluster of 7 undefined items (xrefs 7–18 each) — all read/written by `InitMovieStreamingBuffers`, `PlayMovieFromCD`, `PlayMovieFromBLBSectors`, `DecodeStaticFrame`, `DecodeNextMovieFrame`, `GetNextStreamSector`, `DisplayLoadingScreen`. Derived field layout from `InitMovieStreamingBuffers` decomp and applied types + names:

| Addr | Name | Type | Meaning |
|------|------|------|---------|
| `0x800A5A1A` | `g_MovieState_byte_1a` | `u8` | unknown byte |
| `0x800A5A1B` | `g_MovieState_byte_1b` | `u8` | cleared at init |
| `0x800A5A20` | `g_MovieStreamCounter1` | `u32` | stream counter |
| `0x800A5A24` | `g_MovieFrontBufferPtr` | `void *` | front display buffer (base + 0x61000) |
| `0x800A5A28` | `g_MovieBackBufferPtr` | `void *` | MDEC decode buffer (base + 0x64000) |
| `0x800A5A2C` | `g_MovieStreamCounter2` | `u32` | stream counter |
| `0x800A5A30` | `g_MovieRingBuf1Ptr` | `void *` | CD ring buffer 1 (base + 0x11000, ~160 KB) |
| `0x800A5A34` | `g_MovieRingBuf2Ptr` | `void *` | CD ring buffer 2 (base + 0x39000, ~160 KB) |
| `0x800A5A38` | `g_MovieAudioFlag` | `u32` | audio enable |
| `0x800A5A3C/3E/42` | `g_MovieDisplayY_*` | `u16` | display Y position |
| `0x800A5A40` | `g_MovieStripHeight` | `u16` | `0x18` PAL / `0x10` NTSC |
| `0x800A5A44` | `g_MovieBufferBase` | `void *` | base alloc (~409 KB total) |

#### 6. VRAMSlotXY table @ 0x8009AE40
**Status: RESOLVED.** Identified as the 3-entry × 8-byte constant table indexed by `InitVRAMSlotTable @ 0x80013B1C`. Each entry is `{ u32 x_coord; u32 clut_or_tpage; }`. Defined struct `/Skullmonkeys/VRAMSlotXY` and applied as `VRAMSlotXY[3]` at `0x8009AE40`, renamed to `g_VRAMSlotXYTable`. Values:

| Entry | x_coord | clut_or_tpage |
|-------|---------|---------------|
| 0 | `0x00000007` | `0x00000300` |
| 1 | `0x00000007` | `0x00000200` |
| 2 | `0x00000006` | `0x000001F0` |

### Library-state items (deliberately skipped)
Many remaining `undefined*` items are PSY-Q library internals (LIBCD, LIBSPU, LIBSTREAM, BIOS interrupts) referenced ONLY by library functions like `BIOS_OBJ_*`, `CD_*`, `ISO9660_*`, `_spu_*`, `St*`, `C_011_*`, `trapIntr`, `startIntr`, `data_ready_callback`, `_reset`, etc. Identified by filtering xrefs through a 211-function library-name prefix list. Examples:

- `0x800A6168..0x800A6198` cluster — LIBCD/ISO9660 state
- `0x800A85F0`, `0x800ACBB0`, `0x800ACB64` — LIBSTREAM ring buffer state
- `0x800A560C`, `0x800A561C`, `0x800A5644` — LIBSPU transfer state
- `0x800AE3B8`, `0x800AE3BC`, `0x800AE3C8` — LIBSTREAM/LIBCD interrupt state
- `0x800A0088..0x800A0190` cluster — LIBGPU `SetGraphDebug` / `_reset` BSS

These would only become relevant if/when we port PSY-Q sources into the repo. Left as-is.

### Heap-internal dereference artifacts
The remaining ~150 undefined4's at addresses in `0x80090000..0x800A5953` (the BLB heap) are dereferenced values from `g_pBlbHeapBase + offset` patterns. They are NOT separate globals — they are positions WITHIN the runtime heap struct. To type them properly we would need to define a `BlbHeap` mega-struct mapping all the known sub-regions (level data buffer, render context, primitive lists, etc.). That's a substantial project on its own; deferred until a unified `BlbHeap` definition is built. Examples that fall in this category and were intentionally NOT named individually:

- `0x8009A870` (47 xrefs) — render-context pointer at heap+0xA084
- `0x800907EE` (46 xrefs) — `g_BlbHeapBase[1]` (second short of heap header)
- `0x8009DF18..0x8009DF28` cluster — heap+0xD72C region (likely a CD streaming descriptor)
- `0x8009E1F8..0x8009E264` cluster — heap+0xDA0C region (likely level-data buffer pointers)
- `0x8009E3B8..0x8009E3E8` cluster — heap+0xDBCC region (likely sound/SPU state)

### Final remaining tallies

| Type | Initial | After pass | Cleared |
|------|--------:|-----------:|--------:|
| `undefined1` | 84 | 60 | 24 |
| `undefined2` | 93 | 82 | 11 |
| `undefined4` | 447 | 249 | 198 |
| **total** | **624** | **391** | **233 (37%)** |

The remaining 391 break down roughly as:
- ~150 BLB heap-internal offsets (need `BlbHeap` mega-struct, deferred)
- ~120 PSY-Q library state (intentionally skipped)
- ~120 unidentified — mostly low-xref items (≤3) that are subsystem state with sparse usage; not worth individual typing right now.

### Action items added to backlog

1. **Create `include/Game/player_state.h`** mirroring the Ghidra `PlayerState` struct (HIGH — currently nothing in source).
2. **Build a `BlbHeap` mega-struct** mapping the known sub-regions inside `g_BlbHeapBase` so the ~150 heap-internal dereferences become typed field accesses (MEDIUM).
3. **Investigate the `0x8009DF18..0x8009DF28` and `0x8009E1F8..0x8009E264` clusters** — they have high xref counts (20+) and are clearly state structs, but currently invisible because they're inside the heap.
4. **Audit the 67 library-only undefined items** — confirm they're all PSY-Q internals and tag them as such (so future surveys can filter them automatically).
