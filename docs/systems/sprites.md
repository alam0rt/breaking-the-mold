# Sprite System

Sprites in Skullmonkeys are RLE-compressed graphics with embedded palettes.

## Overview

The game uses a two-tier sprite lookup system:
1. **Tertiary container** - Level-specific sprites (enemies, pickups)
2. **Secondary bank** - Shared sprites across levels (often NULL)

Sprite IDs are **32-bit hashes** hardcoded in game code, not stored in BLB data.

## Sprite Container Structure (Asset 600)

```
Offset   Size    Description
------   ----    -----------
0x00     u32     Sprite count (N)
0x04     N×12    Sprite TOC entries

TOC Entry (12 bytes):
  0x00   u32     Sprite ID (32-bit hash)
  0x04   u32     Data size in bytes
  0x08   u32     Offset from container start
```

## Sprite Data Structure

### Sprite Header (12 bytes)

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    2     u16     Animation count
0x02    2     u16     Frame metadata offset
0x04    4     u32     RLE data offset
0x08    4     u32     Palette offset
```

### Embedded Palette (512 bytes)

256 colors × 2 bytes (PSX 15-bit RGB):
```
Color 0: Transparent (typically 0x0000)
Bits 0-4:   Red (×8 for 8-bit)
Bits 5-9:   Green (×8 for 8-bit)
Bits 10-14: Blue (×8 for 8-bit)
Bit 15:     STP (semi-transparency)
```

### Animation Entry (12 bytes each)

Starting at offset 0x0C from sprite header:

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    4     u32     Animation ID
0x04    2     u16     Frame count
0x06    2     u16     Frame data offset (index into frames)
0x08    2     u16     Flags (bit 0 = has frame callback)
0x0A    2     u16     Extra (usually 0)
```

### Frame Metadata (36 bytes each)

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    2     u16     Callback ID (triggers FUN_8001c4a4)
0x02    2     u16     Reserved
0x04    2     u16     Flip flags (non-zero = horizontal mirror)
0x06    2     s16     Render X offset
0x08    2     s16     Render Y offset
0x0A    2     u16     Render width
0x0C    2     u16     Render height
0x0E    2     u16     Frame delay (animation timing)
0x10    2     u16     Reserved
0x12    2     s16     Hitbox X offset
0x14    2     s16     Hitbox Y offset
0x16    2     u16     Hitbox width
0x18    2     u16     Hitbox height
0x1A    6     bytes   Padding
0x20    4     u32     RLE data offset
```

## RLE Pixel Data Format

Located at `sprite_start + rle_offset + frame.rle_offset`:

```
0x00    u16     Command count
0x02+   u16×N   RLE commands
        ...     Pixel data (8bpp indexed)

Command format (u16):
  Bit 15 (0x8000):    New line flag (advance to next row)
  Bits 8-14 (0x7F00): Skip count >> 8 (transparent pixels)
  Bits 0-7 (0xFF):    Copy count (literal pixels to copy)
```

### RLE Decoder (`DecodeRLESprite` @ 0x80010068)

The decoder processes commands sequentially:

```c
for (i = 0; i < command_count; i++) {
    u16 cmd = commands[i];
    
    // Check for new line
    if (cmd & 0x8000) {
        line_count--;
        if (line_count < 0) return;  // Early exit
        dst = row_start + stride;
        row_start = dst;
    }
    
    // Skip transparent pixels
    dst += (cmd & 0x7F00) >> 8;
    
    // Copy literal pixels
    u8 copy_count = cmd & 0xFF;
    memcpy(dst, src, copy_count);
    src += copy_count;
    dst += copy_count;
}
```

### Horizontal Flip

When flip flag is non-zero (`param_2[5] != 0`), the RLE decoder reverses direction:
- Normal: `dst += skip_count`  and `dst += 1` per pixel
- Mirrored: `dst -= skip_count` and `dst -= 1` per pixel

The pixels are copied in reverse order for mirroring:
```c
// Mirrored 8-byte copy (reversed)
dst[-7] = src[7];
dst[-6] = src[6];
dst[-5] = src[5];
// ... etc
```

### Optimized Copy Loops

The decoder uses unrolled loops for performance:
- **8-byte loop**: Copies 8 pixels at a time
- **4-byte loop**: Copies 4 pixels at a time  
- **1-byte loop**: Copies remaining pixels

This optimization is typical for MIPS CPUs where memory bandwidth matters.

## Sprite Lookup Chain

```
InitSpriteContext(ctx+0x78, sprite_id) @ 0x8007bc3c
    └─► LookupSpriteById(sprite_id) @ 0x8007bb10
          ├─► Search tertiary: ctx+0x70
          └─► Search secondary: ctx+0x40 (g_pSecondarySpriteBank)
                └─► FindSpriteInTOC(container, sprite_id) @ 0x8007b968
```

## Known Sprite IDs

Extracted from game code init functions:

### Player & Core

| Sprite ID | Function | Purpose |
|-----------|----------|---------|
| 0x21842018 | FUN_8001fcf0 | Player (Klaymen) |
| 0xe4ac9451 | FUN_80078200 | HUD digits (×18) |
| 0xec95689b | FUN_80078200 | HUD status |
| 0xaa0da270 | FUN_80078200 | HUD secondary |

### Menu/UI

| Sprite ID | Function | Purpose |
|-----------|----------|---------|
| 0xb8700ca1 | FUN_80076928 | Menu frame |
| 0xe2f188 | FUN_800281a4 | Menu items (×12) |
| 0xa9240484 | FUN_800281a4 | Buttons |
| 0x88a28194 | FUN_800281a4 | Icons |

### Entities

| Sprite ID | Function | Purpose |
|-----------|----------|---------|
| 0x168254b5 | FUN_80034bb8 | Particles (z_order=959) |
| 0x6a351094 | FUN_80037ae0 | Sparkle effect |
| 0x1e1000b3 | FUN_8006dd98 | Enemy type 1 |
| 0xc34aa22 | FUN_800549f0 | Flying collectible |

### Bosses

| Sprite ID | Function | Purpose |
|-----------|----------|---------|
| 0x181c3854 | FUN_80047fb8 | Boss element 1 |
| 0x8818a018 | FUN_80058310 | Boss element 2 (×6) |
| 0x244655d | FUN_80047fb8 | Boss detail |

## Entity-to-Sprite Mapping

**Critical insight**: Sprite IDs are **NOT stored in BLB entity data**. They are **hardcoded in the game executable**.

### How Entity Types Get Sprites

1. **BLB Asset 501** contains 24-byte entity definitions (position/bounds/type)
2. **entity_type** field at offset +0x12 is a small integer (e.g., 1-91)
3. **Game code** has ~91 entity init functions, each with a hardcoded sprite ID
4. When an entity spawns, code dispatches by type → calls Init → passes sprite ID

### Spawn Chain

```
LoadEntitiesFromAsset501() - copies 24-byte defs to ctx+0x28
    ↓
Entity type dispatch (switch on entity_type)
    ↓
InitEntity_XXXXXXXX(entity) - entity-specific initializer
    ↓
InitEntitySprite(entity, HARDCODED_SPRITE_ID, z_order, x, y, flags)
    ↓
InitSpriteContext() → LookupSpriteById() → FindSpriteInTOC()
```

### Player Sprite Tables

Player uses `InitEntityWithSprite` with sprite ID tables:

| Table Address | Contents |
|---------------|----------|
| 0x8009c174 | 16+ sprite IDs for player states |
| 0x8009c3a8 | 7 player sprite variants |
| 0x8009b174 | Menu cursor sprites |

`InitPlayerSpriteAvailability` (0x80059a70) checks which player sprites exist in current level.

## Per-Level Sprite Availability

Each level's tertiary container has different sprites:

| Level | Sprites | Notes |
|-------|---------|-------|
| MENU | 9 | UI only |
| SCIE | 22 | Has 0x09406d8a (clayball) |
| TMPL | 20 | Has 0x09406d8a |
| BOIL | 34 | More enemies |

**Implication**: Entity type → sprite ID is hardcoded. If a level doesn't include a sprite, entities using it won't render.

## Key Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `InitSpriteContext` | 0x8007bc3c | Parse sprite header |
| `LookupSpriteById` | 0x8007bb10 | Find sprite by 32-bit ID |
| `FindSpriteInTOC` | 0x8007b968 | Search container TOC |
| `DecodeRLESprite` | 0x80010068 | RLE decoder with flip |
| `GetFrameMetadata` | 0x8007bebc | Get 36-byte frame entry |
| `InitEntitySprite` | 0x8001c720 | Core entity sprite init |

## Global Variables

| Address | Name | Description |
|---------|------|-------------|
| 0x800a6060 | g_pSecondarySpriteBank | Secondary sprites (often NULL) |
| 0x800a6064 | g_pLevelDataContext | Points to LevelDataContext |

## Related Documentation

- [Entities](entities.md) - Entity system using sprites
- [Player Animation](player-animation.md) - Player sprite direction, flipping, powerups
- [Asset Types](../blb/asset-types.md) - Asset 600 details
- [Rendering Order](rendering-order.md) - Sprite z-ordering
