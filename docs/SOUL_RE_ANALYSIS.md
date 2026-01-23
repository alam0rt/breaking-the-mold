# Soul Reaver Decompilation Process Analysis

Based on examination of [fmil95/soul-re](https://github.com/fmil95/soul-re) repository.

## Project Structure Overview

soul-re uses splat to organize code into logical modules/subsystems:

```
src/Game/
├── BSP.c              # Binary Space Partition
├── CAMERA.c           # Camera system (125KB - huge file!)
├── COLLIDE.c          # Collision detection
├── DEBUG.c            # Debug functions
├── DRAW.c             # Drawing/rendering
├── EVENT.c            # Event system
├── FX.c               # Visual effects
├── GAMELOOP.c         # Main game loop
├── GAMEPAD.c          # Controller input
├── GENERIC.c          # Generic utilities
├── GLYPH.c            # Glyph system
├── HASM_1.s           # Handwritten assembly (data + functions)
├── HASM_2.s           # More handwritten assembly
├── INSTANCE.c         # Instance management
├── LIGHT3D.c          # Lighting
├── LIST.c             # List utilities
├── LOAD3D.c           # 3D loading
├── MATH3D.c           # 3D math functions
├── MEMPACK.c          # Memory management
├── OBTABLE.c          # Object tables
├── PHYSICS.c          # Physics system
├── PHYSOBS.c          # Physics objects
├── PIPE3D.c           # 3D pipeline
├── PLAYER.c           # Player logic
├── REAVER.c           # Soul Reaver weapon
├── SCRIPT.c           # Scripting
├── SIGNAL.c           # Signal system
├── SOUND.c            # Sound/audio
├── SPLINE.c           # Spline math
├── STATE.c            # State machines
├── STREAM.c           # Streaming
├── TIMER.c            # Timing
├── VRAM.c             # VRAM management
├── G2/                # Animation subsystem
├── CINEMA/            # Cinematics
├── LOCAL/             # Localization
├── MCARD/             # Memory card
├── MENU/              # Menu system
├── MONSTER/           # Monster AI
├── PLAN/              # Planning/pathfinding
├── PSX/               # PSX-specific code
└── RAZIEL/            # Player character
```

## Key Splat Configuration Patterns

### 1. Segment Naming Convention

They use `_c` type for C files (not just `c`), which means the C file already exists and splat shouldn't generate it:

```yaml
- [0x003554, _c, Game/PLAYER]      # Uses existing src/Game/PLAYER.c
- [0x0036B8, _c, Game/DEBUG]       # Uses existing src/Game/DEBUG.c
```

### 2. Rodata-Text Pairing by Name

Rodata segments are paired with text segments using the same path name:

```yaml
# Rodata segments (at beginning, before text)
- [0x00085C, .rodata, Game/CAMERA]
- [0x000974, .rodata, Game/DRAW]

# Text segments (after rodata)
- [0x0052A4, _c, Game/CAMERA]      # Paired with Game/CAMERA rodata
- [0x01B110, _c, Game/DRAW]        # Paired with Game/DRAW rodata
```

### 3. Section Order

They use a custom section order with rodata FIRST:

```yaml
section_order: [".rodata", ".text", ".data", ".sdata", ".sbss", ".bss"]
```

This is common for PSX games where rodata appears before text in the binary.

### 4. Handwritten Assembly Files (hasm)

For code that can't be decompiled (inline assembly, GTE macros, etc.):

```yaml
- [0x0159AC, hasm, Game/HASM_1]    # Hand-written assembly
- [0x0689E4, hasm, Game/HASM_2]    # More hand-written assembly
```

These are stored as `.s` files in `src/Game/` alongside the C files.

### 5. BSS Segments

BSS uses the `vram` field since it has no ROM offset:

```yaml
- {start: 0x0C28AC, type: .sbss, vram: 0x800D20FC, name: Game/CAMERA}
- {start: 0x0C28AC, type: .bss, vram: 0x800D75BC, name: Game/SOUND}
```

### 6. Key Options Used

```yaml
options:
  migrate_rodata_to_functions: True    # Include rodata in function asm files
  hasm_in_src_path: True               # Put hasm files in src/ not asm/
  make_full_disasm_for_code: True      # Generate full disassembly for _c segments
  use_legacy_include_asm: False        # Use new INCLUDE_ASM style
```

## Workflow Summary

1. **Initial Split**: One big `asm` segment for all code
2. **Add Symbol Names**: Populated `symbol_addrs.txt` with function names from debug info or reverse engineering
3. **Split by Module**: Divided code into logical modules (CAMERA, DRAW, MATH3D, etc.)
4. **Create C Files**: Changed segments from `asm` → `_c` and created stub C files with `INCLUDE_ASM`
5. **Decompile Functions**: Replace `INCLUDE_ASM` with actual C code one function at a time
6. **Migrate Data**: Move data/rodata to C files, change segments from `data` → `.data`

## The Critical Commit (20a54f7)

The commit you referenced "Split further - Now has symbols but data symbol sizes are approximate" did:

1. Deleted the monolithic `asm/355C.s` (207,302 lines!)
2. Created ~2,566 individual function files organized by module:
   - `asm/Game/BSP/SBSP_IntroduceInstances.s`
   - `asm/Game/CAMERA/CAMERA_Adjust.s`
   - `asm/Game/MATH3D/MATH3D_Sort3VectorCoords.s`
   - etc.

This was an intermediate step before converting to C - organizing functions into logical groups.

## Recommended Approach for BTM (Skullmonkeys)

Based on soul-re's patterns, here's how to organize BTM:

### Proposed Module Structure

```
src/
├── core/
│   ├── main.c          # Entry point, initialization
│   ├── memory.c        # Memory management
│   └── timer.c         # Timing utilities
├── entity/
│   ├── entity.c        # Entity system core
│   ├── collision.c     # Entity collision
│   ├── spawn.c         # Entity spawning
│   └── callbacks/      # Entity type callbacks
├── player/
│   ├── player.c        # Player state machine
│   ├── physics.c       # Player physics
│   └── animation.c     # Player animation
├── level/
│   ├── level.c         # Level loading
│   ├── tilemap.c       # Tilemap rendering
│   └── assets.c        # Asset management (BLB)
├── render/
│   ├── render.c        # Main rendering
│   ├── sprites.c       # Sprite rendering
│   └── effects.c       # Visual effects
├── audio/
│   ├── sound.c         # Sound effects
│   └── music.c         # Music/CD audio
├── menu/
│   └── menu.c          # Menu system
└── libs/
    └── psyq_stubs.s    # PSY-Q library stubs
```

### Migration Steps

1. **Keep Current asm Segments**: Don't change to C until ready
2. **Add Named Segments**: Split the single `asm` into logical modules
3. **Export Function Names**: Use Ghidra analysis to populate symbol_addrs.txt
4. **Create C Stubs**: When ready, change `asm` → `c` and let splat generate stubs
5. **Decompile Incrementally**: One function at a time, starting with small utilities
