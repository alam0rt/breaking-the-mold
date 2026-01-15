# Using Struct Definitions in Ghidra

## Overview

We've created comprehensive struct definitions in `include/blb.h` and `include/game.h`. Loading these into Ghidra will dramatically improve decompilation quality by replacing pointer arithmetic with named field access.

## Step-by-Step: Loading Headers into Ghidra

### Method 1: Parse C Header Files (Recommended)

1. **Open Data Type Manager**
   - Window → Data Type Manager (or press `Ctrl+T`)

2. **Create Archive**
   - Right-click in Data Type Manager
   - New → Category → "Skullmonkeys"

3. **Parse C Source**
   - File → Parse C Source...
   - Add source files:
     ```
     include/common.h
     include/blb.h
     include/game.h
     ```
   - Add parse configuration:
     - Compiler: gcc (or leave default)
     - Parse options: Add include paths if needed
   - Click "Parse to Program"

4. **Verify Types Loaded**
   - Expand "Skullmonkeys" category
   - Should see: `PlayerState`, `LevelDataContext`, `GameState`, `BLBHeader`, etc.

### Method 2: Manual Struct Creation

If parsing fails, create structs manually:

1. **Create New Structure**
   - Data Type Manager → Right-click → New → Structure

2. **Add Fields**
   - Use the field offsets from headers
   - Example for PlayerState:
     ```
     Offset  Type    Name
     0x00    u8      lives
     0x01    u8      continues
     0x02    s16     unknown02
     0x11    u8      health
     ...
     ```

## Using Structs in Decompilation

### Re-type Function Parameters

**Before:**
```c
void initPlayerState(u8 *state) {
    state[0x00] = 1;
    state[0x01] = 1;
    state[0x11] = 5;
}
```

**Steps:**
1. Right-click function name → Edit Function Signature
2. Change `u8 *state` to `PlayerState *state`
3. Or: Right-click parameter → Retype Variable → Select `PlayerState *`

**After (Ghidra will show):**
```c
void initPlayerState(PlayerState *state) {
    state->lives = 1;
    state->continues = 1;
    state->health = 5;
}
```

### Re-type Pointers in Code

**For cast expressions:**
1. Click on the cast `(u8 *)gameState + 0x84`
2. Right-click → Retype Variable
3. Select `LevelDataContext *`
4. Ghidra will show: `&gameState->levelContext`

**For dereferenced pointers:**
1. Select the expression
2. Press `Ctrl+L` (or Right-click → Retype Variable)
3. Choose appropriate struct type

### Create Struct at Memory Address

**To apply struct layout to RAM data:**
1. Navigate to address (e.g., 0x8009DC40 for GameState)
2. Right-click → Data → Choose Data Type
3. Select `GameState` from dropdown
4. Ghidra applies struct layout showing all fields

## Key Structs and Their Addresses

### PAL Version (SLES-01090) Runtime Addresses

| Struct | Address | Size | Usage |
|--------|---------|------|-------|
| `GameState` | 0x8009DC40 | ~0x14A+ bytes | Main game state |
| `LevelDataContext` | 0x8009DCC4 | 128 bytes | GameState+0x84 |
| `BLBHeader` | 0x800AE3E0 | 4096 bytes | Loaded BLB header |
| Entity callback table | 0x8009D5F8 | ~968 bytes | 121 entries × 8 |

**To analyze in Ghidra:**
```
1. Go to address (G → 0x8009DC40)
2. Apply GameState struct (Right-click → Data → GameState)
3. All fields now visible with names
```

### File Format Structures (for BLB parsing code)

| Struct | Usage |
|--------|-------|
| `BLBHeader` | Parse GAME.BLB header (first 0x1000 bytes) |
| `LevelMetadata` | Access level sector offsets |
| `AssetContainer` | Parse TOC entries |
| `TileHeader` | Asset 100 (tile metadata) |
| `EntityDefinition` | Asset 501 (24-byte entity data) |
| `LayerEntry` | Asset 201 (92-byte layer info) |

## Examples

### Example 1: Fixing LevelDataContext Access

**Original Ghidra output:**
```c
void* GetTileHeader(void *ctx) {
    return *(void **)(ctx + 4);
}
```

**After typing `ctx` as `LevelDataContext *`:**
```c
TileHeader* GetTileHeader(LevelDataContext *ctx) {
    return ctx->tileHeader;
}
```

### Example 2: GameState Entity List Access

**Original:**
```c
void EntityTickLoop(void *gs) {
    s32 entity = *(s32 *)((u8 *)gs + 0x1C);
    while (entity != 0) {
        // Process entity...
        entity = *(s32 *)(entity + 4);  // Next pointer
    }
}
```

**After typing `gs` as `GameState *`:**
```c
void EntityTickLoop(GameState *gs) {
    Entity *entity = gs->entityTickList;
    while (entity != NULL) {
        // Process entity...
        entity = entity->next;  // Much clearer!
    }
}
```

### Example 3: BLB Header Movie Access

**Original:**
```c
u16 GetMovieSectorCount(void *header, s32 index) {
    return *(u16 *)((u8 *)header + 0xB60 + (index * 0x1C) + 2);
}
```

**After typing `header` as `BLBHeader *`:**
```c
u16 GetMovieSectorCount(BLBHeader *header, s32 index) {
    return header->movies[index].sectorCount;
}
```

## Benefits

1. **Readable decompilation** - Named fields instead of offsets
2. **Type safety** - Ghidra checks struct member access
3. **Automatic propagation** - Once typed, Ghidra updates all uses
4. **Cross-references** - See all uses of specific struct fields
5. **Documentation** - Struct definitions document the format

## Workflow

When decompiling new functions:

1. **Identify pointer parameters** - What do they point to?
2. **Check if struct exists** - GameState? LevelDataContext? PlayerState?
3. **Re-type the parameter** - Use the struct type
4. **Review decompilation** - Much clearer now!
5. **Copy to C file** - Use the improved output

## Updating Structs

As you discover new fields:

1. **Update header files** (`include/blb.h` or `include/game.h`)
2. **Re-parse in Ghidra** (File → Parse C Source)
3. **Existing typed pointers** automatically show new fields
4. **Document verification** - Add comment with Ghidra address

## Notes

- **Binary matching**: Using structs in function signatures can change code generation
- **Solution**: Keep pointer arithmetic in C files for matching, but use structs in Ghidra for analysis
- **Example**: `initPlayerState` uses `u8 *ptr` internally but signature says `PlayerState *`

This gives us the best of both worlds:
- Clear type information for documentation
- Exact pointer arithmetic for binary matching
