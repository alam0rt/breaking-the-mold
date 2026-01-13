# Skullmonkeys Rendering Order Documentation

**Status: VERIFIED via Ghidra analysis (2026-01-12)**

This document describes how layers and entities are rendered and ordered in Skullmonkeys (PAL SLES-01090).

## Executive Summary

The game uses a **priority-based rendering system** where:
- **Layers and entities share a common priority space** (16-bit signed integer)
- **Lower priority values render behind higher values**
- **Layer priority comes from LayerEntry offset 0x0C** (render_param low 16 bits)
- **Entity priority is hardcoded per entity type** in InitEntitySprite calls

Typical priority ranges observed:
| Priority Range | Content |
|----------------|---------|
| 150-800 | Background layers, parallax |
| 900-1100 | Main gameplay layer, entities |
| 1200-1500 | Foreground layers |
| 10000 | Player entity, UI/HUD |

## Data Structures

### LayerEntry (92 bytes, Asset 201)

```
Offset  Size  Type    Field           Description
------  ----  ------  --------------- --------------------------------------
0x00    2     u16     x_offset        Layer X position in tiles
0x02    2     u16     y_offset        Layer Y position in tiles
0x04    2     u16     width           Layer width in tiles
0x06    2     u16     height          Layer height in tiles
0x08    2     u16     level_width     Level width (from Asset 100)
0x0A    2     u16     level_height    Level height (from Asset 100)
0x0C    4     u32     render_param    Priority in low 16 bits (signed short)
0x10    4     u32     scroll_x        Parallax factor X (0x10000 = 1.0)
0x14    4     u32     scroll_y        Parallax factor Y
...
0x26    1     u8      layer_type      0=normal, 3=skip render # NOT VERIFIED
0x28    2     u16     skip_render     If !=0, skip this layer #  NOT VERIFIED
0x2C    48    u8[48]  color_tints     16 RGB entries for tile tinting
```

### Entity Definition (24 bytes, Asset 501)

```
Offset  Size  Type    Field           Description
------  ----  ------  --------------- --------------------------------------
0x00    2     u16     x1              Bounding box left
0x02    2     u16     y1              Bounding box top
0x04    2     u16     x2              Bounding box right
0x06    2     u16     y2              Bounding box bottom
0x08    2     u16     x_center        Spawn X position (pixels)
0x0A    2     u16     y_center        Spawn Y position (pixels)
0x0C    2     u16     variant         Entity variant/parameter
0x0E    4     u8[4]   padding         Always zero
0x12    2     u16     entity_type     Entity type ID
0x14    2     u16     layer           Layer flags (see below)
0x16    2     u16     padding         Always zero
```

**Layer Field (offset 0x14) Format:**
- Bits 0-7: Render layer (1, 2, or 3) - basic depth grouping
- Bits 8-15: Render flags (purpose unverified, may affect z-ordering)

Most entities use simple values (1, 2, 3). Some entities (types 9, 81 in CSTL) use extended values like 0xF301.

## Initialization Flow

### Layer Initialization (InitLayersAndTileState @ 0x80024778)

1. Iterates all layers from 0 to `GetLayerCount()`
2. For each layer:
   - Checks skip conditions: `layer_type == 3` OR `skip_render != 0` → skip
   - Creates layer render context based on dimensions:
     - ≤64x64: Calls `FUN_8001f534` → adds to render list A
     - ≤128x128: Calls `FUN_8001f150` → adds to render list B
     - Otherwise: Calls `FUN_8001ecc0` → adds to render list C
   - **Priority is `(short)(render_param & 0xFFFF)`** from LayerEntry+0x0C

### Entity Initialization (InitEntitySprite @ 0x8001c720)

Each entity type has a dedicated init function that calls `InitEntitySprite` with **hardcoded parameters**:

```c
// InitEntitySprite(entity, sprite_id, z_order, x, y, flags)
InitEntitySprite(param_1, 0x8c510186, 10000, 0x18, sVar1 + 0x20, 0);  // UI element
InitEntitySprite(param_1, 0x168254b5, 959, x, y, 1);                   // Particle
InitEntitySprite(param_1, 0xa89d0ad0, 1001, 0xa0, 0x78, 0);            // Game entity
```

Known z_order values:
| Entity Type | z_order | Description |
|-------------|---------|-------------|
| Player | 10000 | Always rendered in front of most layers |
| UI/HUD | 10000 | Score, lives, etc. |
| Particles | 959 | Effects, debris |
| General entities | ~1000 | Enemies, pickups |

## Render Order System

### GameState Render Lists

The game maintains two linked lists in GameState:
- **GameState+0x1C**: Update/tick list
- **GameState+0x20**: Render list

Both lists are sorted by priority during insertion (see `FUN_80021590/FUN_80021778/FUN_80021960`):

```c
// Sorted insertion - lower priority values come first in list
if (*(short *)(new_item + 0x10) <= *(short *)(existing_item + 0x10)) {
    // Insert before existing
}
```

### Main Loop Render Order (main @ 0x800828b0)

```
1. EntityTickLoop(g_GameStatePtr)         // Update entities via +0x1C list
2. WaitForVBlankIfNeeded()                // Conditional VSync (skipped if g_SkipVSync)
3. RenderEntities(g_GameStatePtr)         // Draw entities via +0x20 list
4. DrawSync(0)                            // Wait for GPU
5. [Layer Render Callback]                // Draw tile layers (via GameState+0x0C)
6. DrawSync(0)                            // Wait again
7. VSync(2) frame timing                  // If g_GameFlags & 6 set
```

### Layer Render Callback Detail

The layer render callback is invoked via an indirect call chain:
```c
// GameState+0x0C contains a render context pointer
// At renderCtx+0x1C is the actual render function pointer
// At renderCtx+0x18 is a short offset added to GameState
(**(code **)(*(int *)(g_GameStatePtr + 0xc) + 0x1c))
          ((int)g_GameStatePtr + (int)*(short *)(*(int *)(g_GameStatePtr + 0xc) + 0x18));
```

This callback is set up during `InitLayersAndTileState` and iterates the layer render lists to draw all tile layers.

### PSX Ordering Table (OT)

The PSX GPU uses an Ordering Table (OT) for z-sorting. Key characteristics:
- OT is an array of linked list heads (typically 2048-4096 entries)
- `AddPrim(ot[z], primitive)` inserts at position z
- `DrawOTag(ot)` renders from ot[max] down to ot[0]
- **Primitives added FIRST to a z-slot appear BEHIND later ones**

The priority value (from LayerEntry or entity z_order) maps to OT slot index.

## Priority Calculation Examples

### SCIE Stage 0 Layers

| Layer | Priority | scroll_x | Description |
|-------|----------|----------|-------------|
| 0 | 150 | 0x0000 | Far background |
| 1 | 250 | 0xAAAA | Parallax layer |
| 2 | 350 | 0xBA2E | Parallax layer |
| 3 | 350 | 0xCCCC | Parallax layer |
| 4 | 450 | 0xD999 | Mid-distance |
| 5 | 550 | 0xE666 | Mid-distance |
| 6 | 650 | 0xF333 | Near parallax |
| 7 | 950 | 0x10000 | Main gameplay (before player) |
| 8 | 1050 | 0x10000 | Main gameplay (after player) |
| 9 | 1250 | 0x10CCC | Foreground |
| 10 | 1350 | 0x12666 | Far foreground |

With player z_order=10000, the player renders in front of layers 0-10 (max priority 1350).

## Implementation Notes for EVIL Engine

### Current Implementation Issue

The current `stage_scene_builder.gd` uses:
```gdscript
tile_layer.z_index = i  # Just layer index, not actual priority!
entities_container.z_index = 100
player_container.z_index = 200
```

This is incorrect. Should use the actual priority values from render_param.

### Correct Implementation

1. **Read render_param from LayerEntry+0x0C**
2. **Extract priority as `(short)(render_param & 0xFFFF)`**
3. **Use priority value directly as z_index** (or map to appropriate range)
4. **For entities, use the z_order from InitEntitySprite calls**

```gdscript
# Correct approach
for i in range(layer_count):
    var layer_entry = get_layer_entry(i)
    var priority = (layer_entry.render_param & 0xFFFF) as int
    if priority > 32767:  # Handle signed conversion
        priority -= 65536
    tile_layer.z_index = priority

# Entities should use their hardcoded z_order, or map layer field
entities_container.z_index = 1000  # Approximate middle gameplay layer
player_container.z_index = 10000   # Player is always high priority
```

## Verification

These findings were verified through:
1. **Ghidra decompilation** of InitLayersAndTileState (0x80024778)
2. **Ghidra decompilation** of layer render list functions (0x80021590, etc.)
3. **Ghidra decompilation** of InitEntitySprite (0x8001c720) and callers
4. **Raw binary analysis** of SCIE stage0 LayerEntry data at 0x7631C8
5. **Main loop analysis** (0x800828b0) for render order

## References

- `InitLayersAndTileState` @ 0x80024778
- `FUN_8001ecc0` (layer render init C) @ 0x8001ecc0
- `FUN_80021590` (add to render list C) @ 0x80021590
- `InitEntitySprite` @ 0x8001c720
- `RenderEntities` @ 0x80020e80
- `main` @ 0x800828b0
