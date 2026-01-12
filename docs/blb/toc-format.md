# TOC Format

All BLB data segments use consistent Table of Contents (TOC) formats.

## Segment TOC

Found at the start of Primary, Secondary, and Tertiary data segments.

```
Offset   Size   Description
------   ----   -----------
0x00     u32    Entry count
0x04+    12×N   TOC entries
```

### TOC Entry (12 bytes)

```
Offset   Size   Type   Description
------   ----   ----   -----------
0x00     4      u32    Asset type ID (e.g., 0x258 = Asset 600)
0x04     4      u32    Asset size in bytes
0x08     4      u32    Offset from segment start
```

### Example (SCIE Primary Segment)

```
Entry count: 3

Entry 0: type=0x258, size=524,212 bytes, offset=0x28
Entry 1: type=0x259, size=126,256 bytes, offset=0x7FFDC
Entry 2: type=0x25A, size=148 bytes, offset=0x9ED0C
```

### Verification

Entry[0].offset always equals `4 + count * 12` (header size), as the first asset immediately follows the TOC.

## Container Sub-TOC

Container assets (0x258, 0x259, 0x190) have an internal sub-TOC structure.

```
Offset   Size   Description
------   ----   -----------
0x00     u32    Sub-entry count
0x04+    12×N   Sub-entries
```

### Sub-TOC Entry (12 bytes)

```
Offset   Size   Type   Description
------   ----   ----   -----------
0x00     4      u32    Entry ID/flags (varies by asset type)
0x04     4      u32    Data size in bytes
0x08     4      u32    Offset from asset start
```

### Entry ID Field Usage

| Asset Type | Entry ID Meaning |
|------------|-----------------|
| 600 (Sprites) | Sprite ID (32-bit hash) |
| 601 (Audio) | Sample ID (32-bit identifier) |
| 400 (Palettes) | Palette index (0, 1, 2, ...) |

### Example (Sprite Container Sub-TOC)

```
Entry count: 20

Entry 0: id=0x09406d8a, size=2480, offset=0xF4
Entry 1: id=0x400c989d, size=1920, offset=0xAC4
...
```

## Container vs Raw Assets

| Type | Has Sub-TOC | Asset IDs |
|------|-------------|-----------|
| Container | Yes | 0x258 (600), 0x259 (601), 0x190 (400) |
| Raw | No | All others |

Raw assets have data starting immediately after the segment TOC entry offset - no additional header.

## Parsing Code

### Read Segment TOC

```c
typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t offset;
} TOCEntry;

void parse_segment_toc(uint8_t* segment_data) {
    uint32_t count = *(uint32_t*)segment_data;
    TOCEntry* entries = (TOCEntry*)(segment_data + 4);
    
    for (int i = 0; i < count; i++) {
        uint32_t type = entries[i].type;
        uint32_t size = entries[i].size;
        uint8_t* data = segment_data + entries[i].offset;
        // Process asset...
    }
}
```

### Read Container Sub-TOC

```c
void parse_container(uint8_t* asset_data) {
    uint32_t count = *(uint32_t*)asset_data;
    TOCEntry* entries = (TOCEntry*)(asset_data + 4);
    
    for (int i = 0; i < count; i++) {
        uint32_t id = entries[i].type;    // Sprite/sample ID
        uint32_t size = entries[i].size;
        uint8_t* data = asset_data + entries[i].offset;
        // Process sub-asset...
    }
}
```

## Key Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `LoadAssetContainer` | 0x8007b074 | Parses segment TOC, populates ctx |
| `LevelDataParser` | 0x8007a62c | Parses primary segment TOC |
| `FindSpriteInTOC` | 0x8007b968 | Searches sprite container by ID |

## Tilemap Sub-TOC (Asset 200)

Asset 200 also uses a sub-TOC for tilemaps:

```
0x00    u32    Layer count
0x04+   12×N   Layer tilemap entries

Entry:
  0x00  u32    Layer index (0, 1, 2, ...)
  0x04  u32    Tilemap size in bytes
  0x08  u32    Tilemap offset from Asset 200 start
```

Each tilemap is an array of u16 tile indices.

## Related Documentation

- [BLB Overview](README.md) - File structure
- [Asset Types](asset-types.md) - All asset types
- [LevelDataContext](../reference/level-data-context.md) - Where pointers are stored
