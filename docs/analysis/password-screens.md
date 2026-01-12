# Password Screens

The game contains 16 hidden password screen containers not referenced in the main level table.

## Purpose

Skullmonkeys has **no memory card support**. After completing each world, players see a password screen they can use to return to that checkpoint.

## Location Discovery

These containers exist in "gaps" between level data - sectors not referenced by any level's primary/secondary/tertiary offsets. Discovered by scanning unreferenced sectors for valid TOC signatures.

## Container List

| Container | File Offset | Size | Content |
|-----------|-------------|------|---------|
| Password 1 | 0x00EB7000 | 252 KB | World 1 - Gray theme |
| Password 2 | 0x01355000 | 248 KB | World 2 - Gray theme |
| Password 3 | 0x0173E800 | 245 KB | World 3 - Magenta theme |
| Password 4 | 0x01AFB800 | 252 KB | World 4 - Magenta theme |
| Password 5 | 0x01ED8800 | 255 KB | World 5 - Magenta theme |
| Password 6 | 0x02297800 | 254 KB | World 6 - Magenta theme |
| Password 7 | 0x025AB000 | 254 KB | World 7 - Magenta theme |
| Password 8 | 0x02880800 | 253 KB | World 8 - Magenta theme |
| Password 9 | 0x02D1B000 | 252 KB | World 9 - Magenta theme |
| Password 10 | 0x032D7000 | 248 KB | World 10 - Magenta theme |
| Password 11 | 0x0357A800 | 245 KB | World 11 - Magenta theme |
| Password 12 | 0x0397F800 | 244 KB | World 12 - Magenta theme |
| Password 13 | 0x03E93000 | 250 KB | World 13 - Magenta theme |
| Password 14 | 0x03FFC800 | 249 KB | World 14 - Magenta theme |
| Password 15 | 0x044AA800 | 236 KB | World 15 - Magenta theme |
| YOU WIN | 0x047DC800 | 654 KB | Victory screen - Magenta |

## Background Colors

- **Passwords 1-2**: Gray `RGB(122-130, 122-130, 142)` (early worlds)
- **Passwords 3-16**: Magenta `RGB(150, 0, 106)` = `#96006A` (main theme)

## Container Structure

Each password screen has 11 assets:

| Asset ID | Hex | Size Range | Description |
|----------|-----|------------|-------------|
| 100 | 0x064 | 36 bytes | Tile header |
| 200 | 0x0C8 | 1.5-1.7 KB | Tilemap data |
| 201 | 0x0C9 | 460 bytes | Layer entries |
| 300 | 0x12C | 139-144 KB | Tile pixels (8bpp) |
| 301 | 0x12D | 530-675 bytes | Palette indices |
| 302 | 0x12E | 530-675 bytes | Tile flags |
| 400 | 0x190 | 2.1 KB | Palette container (4 palettes) |
| 401 | 0x191 | 16-32 bytes | Palette config |
| 600 | 0x258 | 52 KB | Sprites (RLE) |
| 601 | 0x259 | 38 KB | SPU audio samples |
| 602 | 0x25A | 32 bytes | Audio metadata |

## Identical Asset Sizes

All 16 screens share nearly identical sprite/palette sizes:
- Asset 600 (sprites): 52,368 bytes
- Asset 400 (palettes): 2,100 bytes (4 × 512 + header)

This suggests common templates with screen-specific tile graphics.

## Rendering

Password screens render to 320×256 pixels (20×16 tiles):

1. Parse Asset 100 for dimensions and tile count
2. Extract tiles from Asset 300 with palettes from Asset 301/400
3. For each layer in Asset 201:
   - Get tilemap from Asset 200 sub-TOC
   - Render tiles at layer positions

## Loading Mechanism

These screens are NOT in the level metadata table. They load via a separate code path:

1. World completion triggers password screen
2. Game loads container by sector offset
3. Renders tilemap showing password
4. Player writes down password for later

The sector offsets may be in a hardcoded table or calculated from world index. Further investigation needed.

## Related Documentation

- [BLB Overview](../blb/README.md) - File structure
- [Tiles and Tilemaps](../systems/tiles-and-tilemaps.md) - Tile rendering
- [Asset Types](../blb/asset-types.md) - Asset formats
