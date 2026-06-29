---
title: "Source Directory / Architecture Hypothesis"
category: architecture
tags: [architecture, source, directory, hypothesis]
---

# Source Directory / Architecture Hypothesis

**Date:** 2026-06-12  
**Basis:** Ghidra function map for `SLES_010.90` plus general PSX-era C project organization patterns.  
**Important caveat:** This is not a claim about original source names or exact directories. Ghidra names are clean-room inferred labels, and source paths were not preserved in the binary. Treat this as an architecture map for decompilation planning, not as recovered source truth.

## What Ghidra actually shows

The loaded program has 2269 functions in the game/lib address range. Function ordering strongly suggests a mostly contiguous link of game object files followed by PSY-Q/library code.

| Address range | Ghidra-observed concentration | High-level subsystem hypothesis |
|---------------|-------------------------------|---------------------------------|
| `0x80010000-0x8001031F` | `memset_entrypoint`, `DecodeRLESpriteCore` | Early hand-written/standalone routines: libc stub and sprite RLE core. |
| `0x800127EC` | `GpuDmaTransferCallback` | Isolated GPU callback / interrupt plumbing. |
| `0x800131F0-0x8001A0BF` | Graphics init, buffer swaps, ordering tables, VRAM slots, sprite/tile rendering, CLUT upload | Low-level render/VRAM/video module group. |
| `0x8001A0C0-0x80024777` | Entity init, animation, render callbacks, entity lists, collision dispatch, trigger checks, camera prelude | Entity kernel: core actor struct, animation/render bridge, linked lists, collision messaging. |
| `0x80024778-0x8002C7FF` | Layer/tile setup, tile attributes, entity spawn data, player state/password helpers, HUD/menu early code, collectibles | Level-world setup and global state helpers. |
| `0x8002C800-0x8003DFFF` | Decor objects, password entities, particles, path followers, collectible families, CD audio helpers | General object library and mid-level gameplay actors. |
| `0x8003E000-0x80057FFF` | Enemies, projectiles, bosses, path platforms, clayballs, sound emitters | Actor implementations: enemy/projectile/boss/platform object code. |
| `0x80058000-0x8006DFFF` | Dense `Player*` state-machine functions, normal platforming physics/collision/death/powerups | Main platformer player module group. |
| `0x8006E000-0x800747FF` | FINN/RUNN/SOAR/GLIDE vehicle and variant-player code | Player variants / special movement modes. |
| `0x80074800-0x8007CFFF` | Menu cursor/buttons/password display, ending/credits, sprite lookup, audio upload/settings, `InitGameState` tail | Frontend/UI plus resource lookup/audio setup. |
| `0x8007D000-0x800830FF` | Level load/setup, spawn, game-mode callback, checkpoint, entity type init table region, cheat input, `main` | Game mode orchestration and top-level level lifecycle. |
| `0x80083100-0x800907D3` | `Cd*`, `Spu*`, `Pad*`, `Fnt*`, libc-like helpers | PSY-Q/libc/libgpu/libspu/libcd support. |

This ordering looks like a 90s console game with large subsystem translation units or small groups of `.C` files linked in a stable hand-managed order, not like a modern package tree.

## Probable layered architecture

```text
main / mode loop
  ├─ frontend modes: menus, password entry, pause, ending/credits, cheats
  ├─ level lifecycle: BLB header, asset containers, LevelDataContext, spawn setup
  │   ├─ world setup: tilemaps, layer entries, trigger zones, tile collision
  │   └─ actor spawning: entity definitions → entity callback table → init functions
  ├─ per-frame runtime
  │   ├─ input / pad state
  │   ├─ camera and scrolling
  │   ├─ entity tick list
  │   │   ├─ player state machines
  │   │   ├─ player variants: FINN, RUNN, SOAR, GLIDE
  │   │   ├─ enemies / bosses / projectiles / collectibles / platforms
  │   │   └─ collision and event dispatch
  │   ├─ render list
  │   │   ├─ sprite animation and RLE decode/upload
  │   │   ├─ tilemap/layer rendering
  │   │   └─ GPU primitive submission / ordering tables
  │   └─ audio update: SPU samples, positioned sounds, CD audio
  └─ PSY-Q services: libcd, libgpu, libspu, libetc/libpad/libgte-like support
```

## Plausible source tree shape

A modern reconstruction could be organized like this, but the original may have been flatter and used uppercase PSY-Q-style filenames. Names below are decompilation-facing guesses.

```text
src/
  main.c                 # main loop, frame pacing, game-mode dispatch
  system/
    boot.c               # early init, callbacks, video mode, reset paths
    heap.c               # heap/free-list helpers
    pad.c                # controller/input state glue
    psx_gpu.c            # ordering tables, primitive submission, buffer swaps
    psx_cd.c             # CD/BLB sector read wrappers, movie stream glue
    psx_spu.c            # SPU upload, voice setup, sound handles

  assets/
    blb.c                # BLB header and sector/table navigation
    level_context.c      # LevelDataContext init/clear, sector_read_callback use
    asset_container.c    # primary/secondary/tertiary asset TOC parsing
    sprite_assets.c      # sprite TOC lookup, SpriteHeader/SpriteFrameEntry setup
    audio_assets.c       # Asset 601/700 upload and metadata

  render/
    vram.c               # VRAM slot allocation, CLUT/texture upload
    primitives.c         # POLY/SPRT/TILE primitive allocation/setup
    sprite_render.c      # RenderSpriteOrScaledQuad, RLE decode bridge
    tilemap_render.c     # wrapped tilemap/layer render paths
    palette_effects.c    # CLUT cycling / color lerp effects

  world/
    gamestate.c          # GameState init/reset, mode flags, checkpoint globals
    level_load.c         # InitializeAndLoadLevel / SetupAndStartLevel
    layer.c              # LayerEntry parsing, parallax/autoscroll layer contexts
    tile_collision.c     # Tile attributes and collision map queries
    trigger_zone.c       # TriggerZone iteration and dispatch
    camera.c             # smooth camera follow and scroll limits
    spawn.c              # Asset 501 entity defs, remap/spawn tables

  entity/
    entity.c             # Entity base init/free, callback marker dispatch
    entity_lists.c       # tick/render/collision linked-list management
    entity_render.c      # UpdateEntityRender, texture dirty/upload paths
    entity_collision.c   # entity/entity and point/box collision messaging
    animation.c          # SetEntitySpriteId, frame timing, sequence stepping
    callbacks.c          # vtable/callback slot glue

  objects/
    collectibles.c       # orbs, swirly-q, powerups, clayballs, result tallies
    decor.c              # decorative/path decor entities
    platforms.c          # moving/circular/path platforms
    particles.c          # debris, dust, sparkles, sprite particles
    projectiles.c        # homing/projectile spawns and movement
    sound_emitters.c     # positional sound entities
    enemies_common.c     # shared enemy movement/event helpers
    enemies_*.c          # individual enemy families
    bosses/
      shriney_guard.c
      joe_head_joe.c
      glenn_yntis.c
      monkey_mage.c
      klogg.c

  player/
    player.c             # normal player creation/init/destruction
    player_input.c       # normal input handlers
    player_physics.c     # walk/jump/fall/bounce/collision physics
    player_states.c      # normal platformer FSM states
    player_powerups.c    # shrink/halo/glide/pickups/HUD interaction
    player_particles.c   # player dust/trails/effects
    finn.c               # FINN vehicle mode and subentity/wake handling
    runn.c               # RUNN auto-scroller mode
    soar.c               # SOAR flight mode
    glide.c              # GLIDE variant glue

  ui/
    hud.c                # HUD digits/icons/animated HUD entities
    menu.c               # menu entity base, buttons, cursor, pause menu
    password.c           # password encode/decode and password UI entities
    ending.c             # ending/credits/results-screen entities
    fade.c               # fade overlay/transition sprites

  audio/
    sound.c              # sound table lookup, positioned sounds, panning
    cd_audio.c           # CD music play/pause/resume/stream state
    reverb.c             # SPU reverb/volume settings

  data/
    entity_tables.c      # entity type callback tables, vtables
    sprite_tables.c      # sprite IDs, animation tables
    sound_tables.c       # sound entries and playback metadata
    password_tables.c    # password/cheat tables
```

## Why this shape fits the binary

- The low-address game code begins with frame/video/VRAM primitives before the entity kernel, which is typical for PSX games that bring up graphics and allocator systems first.
- The entity kernel is placed before actor implementations, consistent with actors depending on shared `Entity`, list, animation, and collision helpers.
- Large runs of similarly named functions appear in contiguous blocks: normal player functions cluster around `0x80058000-0x8006DFFF`, vehicle variants around `0x8006E000-0x800747FF`, and top-level level/mode functions around `0x8007D000-0x800830FF`.
- PSY-Q/library symbols are concentrated at the end (`0x80083100+`), consistent with a link order of game objects followed by SDK libraries.
- The architecture is callback-table heavy: entities, menu items, player states, and sprite/render paths all look like small C functions connected through tables, a common 90s console pattern for memory-constrained games.

## Decompilation-facing module boundaries

For source migration, these are safer than pretending to know exact original paths:

| Proposed module group | Anchor functions / evidence |
|-----------------------|------------------------------|
| `render/` | `InitGraphicsSystem`, `RenderSpriteOrScaledQuad`, `RenderTilemapLayerWithScroll`, VRAM/CLUT helpers. |
| `entity/` | `InitEntityStruct`, `UpdateEntityRender`, entity list/collision/message dispatchers. |
| `world/` | `InitLayersAndTileState`, `InitTileAttributeState`, `LoadEntitiesFromAsset501`, trigger/camera helpers. |
| `objects/` | `Init*Entity`, collectible/decor/path/projectile/boss clusters between roughly `0x8002C800-0x80057FFF`. |
| `player/normal/` | Dense `PlayerCallback_*` and `PlayerState_*` functions around `0x80058000-0x8006DFFF`. |
| `player/vehicles/` | FINN/RUNN/SOAR/GLIDE blocks around `0x8006E000-0x800747FF`. |
| `ui/` | `InitMenu*`, password UI, pause, ending/credits/result-screen entities around `0x80074800-0x8007CFFF`. |
| `assets/level/` | `InitLevelDataContext`, `LoadAssetContainer`, `InitializeAndLoadLevel`, `SetupAndStartLevel`. |
| `audio/` | SPU upload/settings and CD audio helpers, split between game wrappers and PSY-Q library calls. |
| `libs/` | PSY-Q/libc support at `0x80083100+`; should remain separate from game-authored source. |

## Open uncertainty

- Exact original filenames/directories are unrecoverable from the current binary alone.
- Link order may not equal source tree order; it can reflect makefile/object ordering.
- Several Ghidra function names are still inferred or provisional, so module assignment should be revisited as names improve.
- PSX studios often used flat directories and broad files such as `GAME.C`, `OBJ.C`, `PLAYER.C`, `MENU.C`, rather than the nested tree above. The nested tree is a clean decompilation organization hypothesis, not a historical assertion.