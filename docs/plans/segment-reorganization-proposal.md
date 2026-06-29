---
title: "Segment Reorganization Proposal"
category: plans
tags: [plans, segment, reorganization, proposal]
---

# Segment Reorganization Proposal

**Created:** January 24, 2026  
**Status:** Proposal for Review

## Problem Summary

The current segment organization has several issues:

1. **GAMELOOP contains entity init functions** - 18 EntityInit callbacks + 30 destructors that belong with the entity system
2. **ENTITY vs OBJECT overlap** - Both contain UI, entity management, and collision code
3. **PLAYER contains non-player code** - Platform entities, Clayball, Shriney Guard mixed in
4. **No clear subsystem boundaries** - Functions are grouped by rough address ranges, not by logical subsystem

## Current vs Proposed Organization

### Current Segments (from SLES_010.90.yaml)

| Segment | Address Range | Functions | Description |
|---------|---------------|-----------|-------------|
| rodata/code | 0x80010000-0x800131EF | 3 | Early init, RLE decode |
| RENDER | 0x800131F0-0x8001A0C7 | 88 | Graphics, VRAM, tiles, sprites |
| ENTITY | 0x8001A0C8-0x8002A377 | 208 | Entity core, animation, collision |
| OBJECT | 0x8002A378-0x80058167 | 709 | UI, enemies, collectibles, bosses |
| PLAYER | 0x80058168-0x80071047 | 308 | Player states + misc entities |
| GAMELOOP | 0x80071048-0x80082FFF | 382 | Main, menus, level loading, tile collision |
| LIBS | 0x80083000-0x80090FFF | 559 | PSY-Q SDK |

### Analysis: What's Actually In Each Segment

#### RENDER (88 functions) - ✓ Mostly coherent
- Graphics init (15)
- VRAM/CLUT allocation (18)
- Heap management (6)
- Tilemap rendering (23)
- Sprite/layer rendering (22)
- Menu entity init (4)

**Issues:** Contains some menu entity code that might belong elsewhere.

#### ENTITY (208 functions) - ✓ Core is coherent, some spillover
- Entity struct init (17)
- Animation system (22)
- Collision detection (14)
- Rendering/layer management (49)
- Position/transform (18)
- State machine (29)
- UI display (2) ← Should be in UI segment
- Unknown/misc (57)

**Issues:** UI functions should be with UI system.

#### OBJECT (709 functions) - Needs subdivision
Contains many distinct subsystems:
- **UI (21)** - HUD display, pause menu
- **Decor/Static (43)** - Background decoration entities
- **Path/Platform (35)** - Moving platforms, paths
- **Enemy (57)** - Enemy AI and behaviors
- **Projectile (21)** - Bullets, homing missiles
- **Sound/Audio (28)** - Sound emitters, CD audio
- **Collectibles (57)** - Orbs, lives, powerups
- **VRAM/Texture (9)** - Texture management
- **Bosses** - Joe Head Joe, Glenn Yntis, Klogg, etc.
- **Unknown (438)** - Needs further classification

#### PLAYER (308 functions) - Mixed content
Contains:
- **Player State (168)** - ✓ Correct placement
- **Player Animation (4)** - ✓ Correct
- **Collision (7)** - ✓ Correct
- **Platform Entity (4)** - ✗ Should be in PLATFORM
- **Clayball (7)** - ✗ Should be in OBJECT/ENEMY
- **Shriney (3)** - ✗ Should be in OBJECT/ENEMY
- **Entity Init/Destroy (39)** - ✗ Generic, mixed
- **Unknown (76)** - Needs classification

#### GAMELOOP (382 functions) - Heavily mixed
Contains:
- **EntityInit (18)** - ✗ Should be in ENTITY
- **EntityTick (12)** - ✗ Should be in ENTITY
- **EntityDestruct (30)** - ✗ Should be in ENTITY
- **Callback_ (9)** - ✗ Needs classification
- **Platform/Tile (31)** - Tile collision system
- **Menu (49)** - Menu system
- **Level/BLB (47)** - Level loading, BLB access
- **FINN/Vehicle (13)** - FINN special vehicle mode
- **Unknown (173)** - Includes RUNN, scaled sprites, timer, etc.

---

## Proposed New Organization

Based on address locality and functional analysis, here's a better organization:

### Option A: Logical Subsystems (Recommended)

Reorganize by logical subsystem, accepting that some subsystems span address ranges:

```yaml
# === EARLY: Bootstrap and decode (0x80010000-0x800131EF) ===
- [0x800, c, Game/EARLY/bootstrap]

# === RENDER: Low-level graphics (0x800131F0-0x8001A0C7) ===
# Kept mostly as-is, already coherent
- [0x39F0, asm, Game/RENDER/graphics]      # Graphics init, buffer swap
- [0x40F0, asm, Game/RENDER/vram]          # VRAM/CLUT allocation
- [0x4BA4, asm, Game/RENDER/heap]          # Heap management  
- [0x5C24, asm, Game/RENDER/sprite]        # Sprite rendering
- [0x681C, asm, Game/RENDER/tilemap]       # Tilemap rendering

# === ENTITY_CORE: Entity system foundation (0x8001A0C8-0x80025930) ===
# Core entity management, animation, collision, state machine
- [0xA8C8, asm, Game/ENTITY_CORE/struct]       # Entity struct management
- [0xAB04, asm, Game/ENTITY_CORE/position]     # Position, transform
- [0xBB60, asm, Game/ENTITY_CORE/collision]    # Collision detection
- [0xCEC8, asm, Game/ENTITY_CORE/animation]    # Animation system
- [0xF2AC, asm, Game/ENTITY_CORE/state]        # State machine, callbacks
- [0x10144, asm, Game/ENTITY_CORE/layer]       # Layer rendering
- [0x1161C, asm, Game/ENTITY_CORE/list]        # Entity list management
- [0x14DBC, asm, Game/ENTITY_CORE/spawn]       # Entity spawning, tile data

# === UI: UI system (0x80025994-0x8002BFF0) ===
- [0x16194, asm, Game/UI/display]             # HUD init, digits, items
- [0x168D0, asm, Game/UI/player_state]        # Player state UI
- [0x183B8, asm, Game/UI/menu_entities]       # Menu entity spawning
- [0x1AB78, asm, Game/UI/pause]               # Pause menu

# === DECOR: Static/decoration entities (0x8002BFF0-0x80032800) ===
- [0x1C7F0, asm, Game/DECOR/destructors]       # Entity destructors
- [0x1D008, asm, Game/DECOR/path]              # Path-following decor
- [0x1E864, asm, Game/DECOR/collectible]       # Collectible effects

# === VRAM_ENTITY: VRAM-based entities (0x800318E0-0x80032800) ===
- [0x220E0, asm, Game/VRAM_ENTITY/slot]        # VRAM slot entities
- [0x22924, asm, Game/VRAM_ENTITY/multipart]   # Multi-part entities

# === PATH_SYSTEM: Path following (0x80032E0C-0x80034CA8) ===
- [0x2360C, asm, Game/PATH_SYSTEM/follow]      # Path follower entities
- [0x23CD4, asm, Game/PATH_SYSTEM/rope]        # Rope rendering

# === PARTICLE: Particle effects (0x80034EC8-0x80038C00) ===
- [0x256C8, asm, Game/PARTICLE/debris]         # Debris particles
- [0x25EBC, asm, Game/PARTICLE/grid]           # Grid distortion
- [0x27B30, asm, Game/PARTICLE/effects]        # Various effects

# === AUDIO: Sound system (0x80038C00-0x8003A000) ===
- [0x29400, asm, Game/AUDIO/cd]                # CD audio
- [0x29D4C, asm, Game/AUDIO/movie]             # Movie playback

# === COLLECTIBLE: Pickup entities (0x8003AE00-0x8003C800) ===
- [0x2B600, asm, Game/COLLECTIBLE/orb]         # Orbs and powerups
- [0x2C634, asm, Game/COLLECTIBLE/sparkle]     # Sparkle effects

# === ENEMY: Enemy AI (0x8003C800-0x80048A54) ===
- [0x2D000, asm, Game/ENEMY/sound_emitter]     # Sound emitter enemies
- [0x2D800, asm, Game/ENEMY/patrol]            # Patrol behavior
- [0x2E508, asm, Game/ENEMY/projectile]        # Enemy projectiles

# === BOSS: Boss enemies (0x80048000-0x8004F000) ===
- [0x38F9C, asm, Game/BOSS/monkey_mage]        # Monkey Mage boss
- [0x3A454, asm, Game/BOSS/glenn_yntis]        # Glenn Yntis boss
- [0x3B764, asm, Game/BOSS/shriney_guard]      # Shriney Guard boss
- [0x3C8E0, asm, Game/BOSS/joe_head_joe]       # Joe Head Joe boss
- [0x3E530, asm, Game/BOSS/klogg]              # Klogg final boss

# === PROJECTILE: Projectile system (0x8004F000-0x80053000) ===
- [0x3F800, asm, Game/PROJECTILE/homing]       # Homing projectiles
- [0x40C40, asm, Game/PROJECTILE/hud]          # HUD follow

# === PATH_ENTITY: Advanced path entities (0x80055800-0x80058168) ===
- [0x460B8, asm, Game/PATH_ENTITY/update]      # Path update
- [0x46958, asm, Game/PATH_ENTITY/clayball]    # Clayball entities

# === PLAYER: Player character (0x80058168-0x8006E000) ===
- [0x48968, asm, Game/PLAYER/core]             # Player entity creation
- [0x4A218, asm, Game/PLAYER/collision]        # Player collision
- [0x4B454, asm, Game/PLAYER/physics]          # Player physics
- [0x4C200, asm, Game/PLAYER/state_idle]       # Idle states
- [0x4D374, asm, Game/PLAYER/state_walk]       # Walking states
- [0x52334, asm, Game/PLAYER/state_jump]       # Jump states
- [0x55D84, asm, Game/PLAYER/state_fall]       # Falling states
- [0x58380, asm, Game/PLAYER/state_death]      # Death states

# === FINN_VEHICLE: FINN special mode (0x8006E000-0x80071048) ===
- [0x5E800, asm, Game/FINN_VEHICLE/follow]     # Subentity follow
- [0x5FA50, asm, Game/FINN_VEHICLE/movement]   # FINN movement
- [0x60128, asm, Game/FINN_VEHICLE/input]      # FINN input

# === TILE_PHYSICS: Tile collision physics (0x80071048-0x80073400) ===
- [0x61848, asm, Game/TILE_PHYSICS/collision]  # Tile-entity collision
- [0x61D10, asm, Game/TILE_PHYSICS/platform]   # Platform physics

# === RUNN_VEHICLE: RUNN special mode (0x80073400-0x80074600) ===
- [0x63C00, asm, Game/RUNN_VEHICLE/core]       # RUNN entity

# === SCALED_SPRITE: Scaled sprite entities (0x80073400-0x80075000) ===
- [0x64138, asm, Game/SCALED_SPRITE/render]    # Scaled rendering

# === MENU: Menu system (0x80074F98-0x80078200) ===
- [0x65798, asm, Game/MENU/cursor]             # Menu cursor
- [0x65A34, asm, Game/MENU/button]             # Menu buttons
- [0x663FC, asm, Game/MENU/password]           # Password system
- [0x67BA0, asm, Game/MENU/stages]             # Menu stages

# === BOSS_PLAYER: Boss mode player (0x80078200-0x80079800) ===
- [0x68A00, asm, Game/BOSS_PLAYER/core]        # Boss player

# === LEVEL_LOADER: Level management (0x8007A1BC-0x8007C800) ===
- [0x6A9BC, asm, Game/LEVEL_LOADER/context]    # Level context
- [0x6B074, asm, Game/LEVEL_LOADER/asset]      # Asset loading
- [0x6BBF0, asm, Game/LEVEL_LOADER/accessor]   # BLB accessors

# === SOUND_ENGINE: Sound effect system (0x8007C388-0x8007CD34) ===
- [0x6CB88, asm, Game/SOUND_ENGINE/play]       # Sound playback
- [0x6D288, asm, Game/SOUND_ENGINE/spu]        # SPU upload

# === GAMESTATE: Game state management (0x8007CD34-0x8008150C) ===
- [0x6D534, asm, Game/GAMESTATE/init]          # InitGameState
- [0x6D7A0, asm, Game/GAMESTATE/load]          # Level loading
- [0x6E654, asm, Game/GAMESTATE/mode]          # Game mode callbacks
- [0x6F2AC, asm, Game/GAMESTATE/checkpoint]    # Checkpoint system

# === ENTITY_REMAP: Entity type remapping (0x8008150C-0x80082700) ===
- [0x71D0C, asm, Game/ENTITY_REMAP/remap]      # Type remapping
- [0x723AC, asm, Game/ENTITY_REMAP/sprite]     # Sprite loading

# === MAIN: Entry point (0x800828B0-0x80082FFF) ===
- [0x730B0, c, Game/MAIN/main]                 # main()
- [0x73410, c, Game/MAIN/debug]                # Debug menu

# === LIBS: PSY-Q SDK (0x80083000-0x80090FFF) ===
- [0x73800, asm, LIBS]
```

### Option B: Address-Based with Better Names

Keep roughly similar boundaries but with descriptive names:

```yaml
# Game code segments (organized by address locality)
- [0x39F0, asm, Game/render_system]           # 0x800131F0 - Graphics/VRAM
- [0xA8C8, asm, Game/entity_core]             # 0x8001A0C8 - Entity foundation
- [0x1AB78, asm, Game/game_objects]           # 0x8002A378 - Enemies/items/decor
- [0x48968, asm, Game/player_system]          # 0x80058168 - Player character
- [0x61848, asm, Game/level_system]           # 0x80071048 - Levels/menus/main
- [0x73800, asm, LIBS]                        # 0x80083000 - PSY-Q SDK
```

---

## Recommended Approach

Given the complexity of the codebase and the fact that memory addresses don't always align with logical boundaries, I recommend:

### Phase 1: Keep Current Segments, Add Subsegments

Rather than completely reorganizing, add subsegments within existing segments to better document the logical groupings:

```yaml
# --- GAMELOOP with subsegments ---
- [0x61848, asm, Game/GAMELOOP/tile_collision]    # Tile physics
- [0x63C00, asm, Game/GAMELOOP/runn_vehicle]      # RUNN mode
- [0x64138, asm, Game/GAMELOOP/scaled_sprite]     # Scaled sprites
- [0x65798, asm, Game/GAMELOOP/menu]              # Menu system
- [0x6A9BC, asm, Game/GAMELOOP/level_loader]      # Level loading
- [0x6CB88, asm, Game/GAMELOOP/sound]             # Sound engine
- [0x6D534, asm, Game/GAMELOOP/gamestate]         # Game state
- [0x71D0C, asm, Game/GAMELOOP/entity_remap]      # Entity remapping
- [0x730B0, c, Game/GAMELOOP/main]                # main()
```

### Phase 2: Create Documentation Map

Create `docs/module_map.yaml` to document which functions belong to which logical subsystem, regardless of address:

```yaml
subsystems:
  player_state_machine:
    description: "Player state callbacks and transitions"
    functions:
      - PlayerState_IdleRandom
      - PlayerState_WalkingRight
      # etc.
    
  entity_spawn:
    description: "Entity spawning and initialization"
    functions:
      - InitEntityStruct
      - EntityInitCallback_800713f4  # In GAMELOOP but belongs here
      # etc.
```

### Phase 3: Gradual Migration

As functions are decompiled to C, move them to logical source files:
- `src/Game/entity/spawn.c`
- `src/Game/player/state.c`
- `src/Game/level/loader.c`

---

## Function Counts by Proposed Subsystem

| Subsystem | Est. Functions | Current Location | Notes |
|-----------|----------------|------------------|-------|
| render_graphics | 35 | RENDER | Coherent |
| render_vram | 24 | RENDER | Coherent |
| render_tilemap | 23 | RENDER | Coherent |
| entity_core | 80 | ENTITY | Coherent |
| entity_animation | 40 | ENTITY | Coherent |
| entity_collision | 35 | ENTITY | Coherent |
| entity_spawn | 50 | ENTITY+GAMELOOP | Split |
| hud_display | 25 | ENTITY+OBJECT | Split |
| collectible | 60 | OBJECT | Coherent |
| enemy_basic | 80 | OBJECT | Coherent |
| enemy_boss | 100 | OBJECT | Coherent |
| projectile | 40 | OBJECT | Coherent |
| audio_system | 35 | OBJECT+GAMELOOP | Split |
| player_core | 40 | PLAYER | Coherent |
| player_states | 160 | PLAYER | Coherent |
| platform_physics | 30 | PLAYER+GAMELOOP | Split |
| menu_system | 50 | GAMELOOP | Coherent |
| level_loader | 50 | GAMELOOP | Coherent |
| game_state | 30 | GAMELOOP | Coherent |
| finn_vehicle | 25 | GAMELOOP | Coherent |
| runn_vehicle | 15 | GAMELOOP | Coherent |

---

## Automated Analysis Results

Running `python3 tools/scripts/analyze_segments.py` produces suggested subsegments based on function clustering:

### RENDER Subsegments (88 functions)
Already fairly coherent. Categories:
- unknown (21) - utility functions
- destroy (18) - memory/resource freeing
- init (16) - initialization
- render (9) - actual rendering
- vram (8) - VRAM allocation
- animation (6) - frame/sprite helpers
- level (5) - texture upload
- tilemap (3) - layer management
- tick (2) - palette cycling

### ENTITY Subsegments (208 functions)
Core entity system. Categories:
- unknown (54) - scale, position, transform
- tick (38) - update callbacks
- destroy (26) - cleanup functions
- animation (22) - animation system
- collision (17) - detection/response
- init (17) - entity creation
- render (15) - rendering
- level (7) - layer management
- player (5) - input handling
- hud (4) - digit/timer display
- menu (2) - menu entities

### OBJECT Subsegments (709 functions)
Very large, needs subdivision:
- unknown (305) - needs classification
- init (117) - entity initialization
- destroy (78) - cleanup
- enemy (65) - AI and combat
- tick (55) - update callbacks
- boss (17) - boss-specific
- collision (16) - collision handlers
- animation (16) - sprite/frame
- collectible (14) - pickups
- render (14) - drawing
- audio (13) - sound
- hud (12) - timers
- decor (9) - decorations
- projectile (5) - bullets
- menu (4) - pause menu
- level (3) - asset loading
- vram (1), tilemap (1), player (1)

### PLAYER Subsegments (308 functions)
Player character + some entity code:
- tick (167) - state callbacks
- init (52) - entity creation
- player (40) - player-specific
- destroy (15) - cleanup
- collision (13) - wall/floor detection
- unknown (12) - transforms
- audio (4) - sound
- vehicle (3) - FINN mode
- decor (1), tilemap (1)

### GAMELOOP Subsegments (382 functions)
Main loop, menus, level loading:
- init (139) - EntityInitCallback_ functions
- unknown (47) - game flow
- level (37) - BLB/level loading
- tick (34) - callbacks
- destroy (31) - cleanup
- menu (28) - menu system
- tilemap (14) - tile accessors
- collision (12) - tile physics
- vehicle (10) - FINN/RUNN
- audio (10) - sound engine
- animation (8) - sprite lookup
- render (5) - display
- player (4) - tilemap width
- enemy (2) - damage handlers
- decor (1) - platform input

---

## Recommended Concrete Subsegments

Based on analysis, here are the recommended YAML subsegments:

```yaml
# === GAMELOOP with recommended subsegments ===
# Tile collision physics (12 functions)
- [0x61848, asm, Game/GAMELOOP/tile_collision]    # 0x80071048

# Entity init callbacks cluster (11+ functions)
- [0x62230, asm, Game/GAMELOOP/entity_init]       # 0x80071a30

# Platform and tick callbacks
- [0x62FB4, asm, Game/GAMELOOP/platform_tick]     # 0x800727b4

# RUNN/FINN vehicle mode
- [0x639FC, asm, Game/GAMELOOP/runn_destroy]      # 0x800731fc
- [0x64B38, asm, Game/GAMELOOP/finn_vehicle]      # 0x80074338

# Menu system (28+ functions)
- [0x65798, asm, Game/GAMELOOP/menu_cursor]       # 0x80074f98
- [0x66A1C, asm, Game/GAMELOOP/menu_password]     # 0x8007621c
- [0x673A0, asm, Game/GAMELOOP/menu_stages]       # 0x80076ba0

# Game mode and ending
- [0x68A00, asm, Game/GAMELOOP/boss_player]       # 0x80078200
- [0x6A0B0, asm, Game/GAMELOOP/ending]            # 0x800798b0

# Level context and loading
- [0x6A9BC, asm, Game/GAMELOOP/level_context]     # 0x8007a1bc
- [0x6AE2C, asm, Game/GAMELOOP/level_parser]      # 0x8007a62c
- [0x6B5B8, asm, Game/GAMELOOP/level_accessor]    # 0x8007adb8

# Tile/asset accessors
- [0x6B874, asm, Game/GAMELOOP/asset_container]   # 0x8007b074
- [0x6BEBC, asm, Game/GAMELOOP/tile_accessor]     # 0x8007b6bc

# Sound engine
- [0x6CB88, asm, Game/GAMELOOP/sound_play]        # 0x8007c388
- [0x6D088, asm, Game/GAMELOOP/sound_spu]         # 0x8007c888

# Game state
- [0x6D534, asm, Game/GAMELOOP/gamestate_init]    # 0x8007cd34
- [0x6DF38, asm, Game/GAMELOOP/gamestate_spawn]   # 0x8007df38
- [0x6F2AC, asm, Game/GAMELOOP/checkpoint]        # 0x8007eaac

# Entity type init cluster (86 functions!)
- [0x6F6D8, asm, Game/GAMELOOP/entity_type_init]  # 0x8007eed8

# Entity remapping and sprite loading
- [0x71D0C, asm, Game/GAMELOOP/entity_remap]      # 0x8008150c
- [0x723AC, asm, Game/GAMELOOP/sprite_load]       # 0x80081bac

# Cheat codes and main
- [0x728B4, asm, Game/GAMELOOP/cheat]             # 0x800820b4
- [0x730B0, c, Game/GAMELOOP/main]                # 0x800828b0
- [0x73410, asm, Game/GAMELOOP/debug_menu]        # 0x80082c10
```

---

## Next Steps

1. **Review this proposal** - Does the logical grouping make sense?
2. **Decide on approach** - Option A (full reorganization) or Option B (subsegments)?
3. **Create module_map.yaml** - Document logical groupings
4. **Update YAML incrementally** - Add subsegments as we decompile

## Questions for Review

1. Should FINN and RUNN vehicle code be combined into a single "vehicle_modes" subsystem?
2. Should all entity init callbacks (currently in GAMELOOP) be moved to ENTITY_CORE?
3. How should we handle the 438 "Unknown" functions in OBJECT?
4. The 139 `EntityInitCallback_*` functions in GAMELOOP - are these entity type initializers that should stay together?
