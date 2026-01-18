# Data Section Memory Map

This document maps the .data and .sdata sections of the PAL binary (SLES-01090).

**Last Updated**: January 19, 2026  
**Ghidra Plate Comments**: Key data areas have been annotated with plate comments for easier navigation.

## Overview

| Section | Start | End | Size | Purpose |
|---------|-------|-----|------|---------|
| .data | 0x8009B000 | 0x800A5950 | ~0xA950 (43KB) | Static data, tables, arrays |
| .sdata | 0x800A5950 | 0x800A60C4 | ~0xB74 (2.9KB) | Small data (GP-relative) |
| .rodata | 0x800A60C4 | ~0x800A6500 | ~0x440 | Read-only strings |

## Key Data Areas (Ghidra Annotated)

These addresses have plate comments in Ghidra for reference:

| Address | Name | Size | Ghidra Comment |
|---------|------|------|----------------|
| 0x8009B074 | g_MenuLookupTables | ~300 | Menu background/anim/transition data |
| 0x8009B3D8 | g_SectorTable | 100 | CD sector lookup table (sector offsets) |
| 0x8009B4B4 | g_GameBLBFile | 40 | CdlFILE structure for GAME.BLB |
| 0x8009B4DC | g_CollectibleSpriteTables | ~128 | Sprite arrays for pickups/items |
| 0x8009BA48 | g_BossSpriteTables | ~160 | Sprite arrays for 5 bosses |
| 0x8009C174 | g_PlayerSpriteTable | 56 | Player state → sprite ID mapping |
| 0x8009D5F8 | g_EntityTypeCallbackTable | 968 | 121 entity types × 8 bytes |
| 0x8009D9C0 | g_TriggerZoneColorTable | 60 | RGB colors for 20 trigger zones |
| 0x8009DAE0 | g_CheatPatternTable | 280 | Button sequences for cheats |
| 0x8009DC40 | g_GameState | 416 | Main game state structure |
| 0x800A5754 | g_pPlayerState | 4 | Pointer to 30-byte PlayerState |
| 0x800A5960 | g_GameStatePtr | 4 | Pointer to active GameState |
| 0x800A59F0 | g_GameBLBSector | 4 | BLB starting sector (0x146) |
| 0x800AE3E0 | BLBHeaderBuffer | 0x1000 | Loaded BLB header at runtime |

## .data Section (0x8009B000 - 0x800A5950)

### Motion Tables (0x8009B038 - 0x8009B074)

Used by animation/physics system.

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x8009B038 | 60 | g_SineMotionTable | Motion curve data (s16 deltas) |

### Menu Sprite Tables (0x8009B074 - 0x8009B198)

| Address | Entries | Name | Purpose |
|---------|---------|------|---------|
| 0x8009B074 | ? | g_MenuBackgroundTable | Background sprite lookup |
| 0x8009B0BC | ? | g_MenuAnimTable | Animation frame lookup |
| 0x8009B104 | ? | g_MenuTransitionTable | Menu transition data |
| 0x8009B174 | 3 | g_MenuCursorSprites | Cursor sprite IDs |
| 0x8009B180 | ? | g_MenuSpriteTable2 | Additional menu sprites |
| 0x8009B18C | ? | g_MenuSpriteTable3 | Additional menu sprites |

### Password Encoding Tables (0x8009B198 - 0x8009B1E8)

**Address**: 0x8009B198 / 0x8009B199  
**Size**: 80 bytes (32 entries × 2 bytes + padding)  
**Name**: g_PasswordEncodingTable  
**Referenced by**: `BuildPasswordFromPlayerState` @ 0x80025C7C, `DecodePassword` @ 0x80025E48

Bit-field encoding lookup table for the password system. Used to map PlayerState fields
(level, lives, collectibles) into a 12-button password sequence.

**Format**: 32 pairs of (field_index, bit_position)
```c
// Each pair defines which PlayerState byte and bit to encode:
// [0]: field_index (which byte of PlayerState to read)
// [1]: bit_position (which bit within that byte)
// 
// Example from dump at 0x8009B198:
// 01 00 - field 1, bit 0
// 01 01 - field 1, bit 1  
// 01 02 - field 1, bit 2
// 01 03 - field 1, bit 3
// ...
```

**Password Encoded Fields** (from decompilation):
| PlayerState Offset | Field | Purpose |
|--------------------|-------|---------|
| 0x00 | level | Current level (+1, skips 5 and 0x11) |
| 0x11 | lives | Life count |
| 0x14 | phoenix_hands | Phoenix Hand collectibles |
| 0x15 | phart_heads | Phart Head collectibles |
| 0x16 | universe_enemas | Universe Enema collectibles |
| 0x19 | 1970_icons | 1970 collectibles |
| 0x1B | swirly_qs | Swirly Q collectibles |
| 0x1C | super_willies | Super Willie collectibles |

**Note**: This closes a gap in KNOWLEDGE_GAPS.md - the password encoding algorithm is fully implemented.

### CD-ROM Data (0x8009B3D8 - 0x8009B4DC)

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x8009B3D8 | 100 | g_SectorTable | CD sector lookup table |
| 0x8009B43C | 24 | g_CdlLOCParams | CD location parameters |
| 0x8009B4B4 | 40 | g_GameBLBFile | CdlFILE for GAME.BLB |

### Collectible Sprite Tables (0x8009B4DC - 0x8009B55C)

| Address | Entries | Name | Init Function |
|---------|---------|------|---------------|
| 0x8009B4DC | 4 | g_SpecialPickupSprites | InitSpecialPickupEntity @ 0x8003cfd8 |
| 0x8009B4EC | 7 | g_WalkingCollectibleSprites | InitWalkingCollectibleEnemy @ 0x8003d638 |
| 0x8009B508 | 7 | g_TempestPulsingMonkeySprites | InitTempestPulsingMonkey @ 0x8003e0fc |
| 0x8009B524 | 3 | g_SoundEmittingEnemySprites | InitSoundEmittingEnemy @ 0x8003e950 |
| 0x8009B530 | 3 | g_CutEntitySprites_UNUSED | **CUT CONTENT** - sprite IDs not in BLB |
| 0x8009B53C | 4 | g_LevelStateCollectibleSprites | InitLevelStateCollectible @ 0x8003f704 |
| 0x8009B54C | 4 | g_CollectibleVariantSprites | InitCollectibleVariant @ 0x8003fe90 |

### Boss Sprite Tables (0x8009BA48 - 0x8009BAE4)

| Address | Entries | Name | Init Function |
|---------|---------|------|---------------|
| 0x8009BA48 | 7 | g_MonkeyMageBossSprites | InitMonkeyMageBoss @ 0x80047fb8 |
| 0x8009BA64 | 9 | g_GlennYntisBossSprites | InitGlennYntisBoss @ 0x80049a54 |
| 0x8009BA88 | 9 | g_ShrineyGuardBossSprites | InitShrineyGuardBoss @ 0x8004af64 |
| 0x8009BAAC | 8 | g_JoeHeadJoeBossSprites | InitJoeHeadJoeBoss @ 0x8004c0e0 |
| 0x8009BACC | 6 | g_KloggBossSprites | InitKloggBoss @ 0x8004d88c |
| 0x8009BAF8 | 12 | g_BossSoundTable | Boss sound effect lookup |

### Enemy Sprite Tables (0x8009BBE0 - 0x8009BC08)

| Address | Entries | Name | Init Function |
|---------|---------|------|---------------|
| 0x8009BBE0 | 3 | g_EnemyAISprites | InitEnemyEntityWithAI @ 0x8004f8dc |
| 0x8009BBEC | 4 | g_JoeHeadJoeBallRegularSprites | InitJoeHeadJoeBallRegular @ 0x80053afc |
| 0x8009BBFC | 3 | g_ProjectileSprites | InitProjectileWithTimer @ 0x80050970 |

### Animation Motion Curves (0x8009BC08 - 0x8009C0E8)

**Address**: 0x8009BC08  
**Size**: ~0x4E0 bytes (1248 bytes)  
**Name**: g_AnimationMotionCurves  
**Referenced by**: Animation callback at LAB_80053664 (via InitAnimatedDirectionalEntity)

Large table of signed 16-bit coordinate pairs (x, y deltas) forming sine/cosine motion curves.
Used for smooth entity movement, bobbing, and oscillating animations.

**Structure**: s16 pairs representing (dx, dy) per animation frame
```c
// Example from 0x8009BF00:
// Oscillating pattern around a center point (sine-like):
// 0xFFFF (-1), 0x00B0 (176)  - Y high
// 0x0022 (34), 0x00AF (175)
// 0x0043 (67), 0x00AA (170)
// ... gradually decreases Y while X increases
// Pattern continues through full oscillation
```

**Note**: No direct XRefs at aligned addresses - accessed via base pointer + index calculations.

### Player Data (0x8009C0E8 - 0x8009C3C0)

| Address | Entries | Name | Purpose |
|---------|---------|------|---------|
| 0x8009C0E8 | 3 | g_JoeHeadJoeBallSprites | Special ball sprites |
| 0x8009C174 | 56 | g_PlayerSpriteTable | Player state sprite IDs (16+ entries) |
| 0x8009C3A8 | 7 | g_PlayerSpriteVariants | Player variant sprites |

### Menu/UI Data (0x8009CB00 - 0x8009CC30)

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x8009CB00 | 12 | g_MenuPasswordPositions | Password entry x/y coords |
| 0x8009CB0C | 48 | g_MenuButtonPositions | Button x/y positions (4 buttons × 6 bytes) |
| 0x8009CB4E | ? | g_MenuAnimationFrames | Animation frame indices |
| 0x8009CBAC | 15 | g_MenuBackgroundColorTable | RGB presets (5 colors × 3 bytes) |

**g_MenuBackgroundColorTable Format:**
```c
// 5 preset colors (RGB triplets), used by UpdateBackgroundColor (0x800778EC)
// Index via options menu color picker
u8 g_MenuBackgroundColorTable[15] = {
    0x80, 0x40, 0x20,  // [0] Brown-ish
    0x10, 0x20, 0x80,  // [1] Blue
    0x40, 0x80, 0x20,  // [2] Green  
    0x20, 0x10, 0x80,  // [3] Purple
    0x20, 0x40, 0x80,  // [4] Cyan-blue
};
```

| Address | Entries | Name | Purpose |
|---------|---------|------|---------|
| 0x8009CBDC | 3 | g_MenuCursorSprites | Menu cursor sprite IDs |
| 0x8009CBE8 | 4 | g_MenuButtonSprites | Menu button sprites |
| 0x8009CBF8 | 4 | g_MenuLogoSprites | Logo sprites |
| 0x8009CC08 | 4 | g_MenuBackgroundSprites | Background sprites |
| 0x8009CC30 | 48 | g_SPUVoicePitchBuffer | SPU voice pitch save buffer

### Cheat System (0x8009DAE0 - 0x8009D5F8)

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x8009DAE0 | 280 | g_CheatPatternTable | Cheat input patterns (button sequences) |

### Trigger Zone Colors (0x8009D9C0 - 0x8009D9FC)

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x8009D9C0 | 60 | g_TriggerZoneColorTable | RGB triplets for 20 trigger zone types |

Format: 20 entries × 3 bytes (RGB), used by HandleGenericTriggerZone.

```c
// Example colors from dump:
// [0] 0x00, 0x00, 0x00 - Black
// [1] 0x20, 0x20, 0x20 - Dark gray
// [2] 0x30, 0x30, 0x30 - Gray
// [3] 0x40, 0x40, 0x40 - Medium gray
// ...
// [6] 0xFF, 0xFF, 0xFF - White
// [7] 0x60, 0x20, 0x20 - Dark red
```

### Entity Type Init Sprite Lists (0x8009D9FC - 0x8009DADC)

| Address | Entries | Name | Entity Type |
|---------|---------|------|-------------|
| 0x8009D9FC | 7 | g_SpriteList_Type004_Type114 | EntityType 004/114 |
| 0x8009DA18 | 7 | g_SpriteList_Type088_ForegroundDecor | EntityType 088 |
| 0x8009DA34 | 7 | g_SpriteList_Type117_DecorationB | EntityType 117 |
| 0x8009DA50 | 3 | g_SpriteList_Type010_InteractiveObject | EntityType 010 |
| 0x8009DA5C | 3 | g_SpriteList_Type026_Enemy | EntityType 026 |
| 0x8009DA68 | 4 | g_SpriteList_Type027_PathEnemy | EntityType 027 |
| 0x8009DA78 | 5 | g_SpriteList_Type061_ScaledPlatform | EntityType 061 |
| 0x8009DA8C | 3 | g_SpriteList_Type062_Platform | EntityType 062 |
| 0x8009DA98 | 3 | g_SpriteList_Type063_MovingPlatform | EntityType 063 |
| 0x8009DAA4 | 3 | g_SpriteList_Type064_Trigger | EntityType 064 |
| 0x8009DAB0 | 7 | g_SpriteList_Type111_Background | EntityType 111 |
| 0x8009DACC | 5 | g_SpriteList_Type113_Hazard | EntityType 113 |

### Entity Type Callback Table (0x8009D5F8 - 0x8009D9C0)

**Address**: 0x8009D5F8  
**Size**: 968 bytes (121 entries × 8 bytes)  
**Name**: g_EntityTypeCallbackTable  

Maps entity types (0-120) to initialization callbacks. Each entry:
```c
struct EntityTypeEntry {
    u16 flags;       // Entity flags (usually 0x0000 or 0xFFFF)
    u16 padding;     // Always 0xFFFF
    void* callback;  // Pointer to InitEntity_XXXXXXXX function
};
```

Referenced by:
- `RemapEntityTypesForLevel` - Translates BLB types to internal types
- `GameState+0x7C` stores pointer to this table at runtime

### GameState (0x8009DC40 - 0x8009DDE0)

**Address**: 0x8009DC40 (g_GameState)  
**Size**: 0x1A0 (416 bytes)  
**Documentation**: See `include/Game/game_state.h` for complete struct.

Key offsets:
| Offset | Type | Name | Purpose |
|--------|------|------|---------|
| +0x00 | s16 | mode_index | Current mode callback index |
| +0x1C | ptr | entity_tick_list_head | Z-sorted tick list |
| +0x20 | ptr | entity_render_list_head | Z-sorted render list |
| +0x30 | ptr | player_entity | Main player entity |
| +0x40 | ptr | blb_header_ptr | BLB header @ 0x800AE3E0 |
| +0x7C | ptr | callback_table_ptr | → g_EntityTypeCallbackTable |
| +0x84 | struct | level_data_context | LevelDataContext (88 bytes) |

### Debug Menu Item Names (0x8009DDE0 - 0x8009DE08)

**Address**: 0x8009DDE0 (g_MenuItemNames)  
**Size**: 40 bytes (10 pointers)  

Array of pointers to stage name strings used by `ProcessDebugMenuInput()`.
Points to strings like "sub 01", "sub 02", etc. at 0x800A60C4.

Enabled when `g_GameFlags & 0x80` is set.

## .sdata Section (0x800A5950 - 0x800A60C4)

Small data section, accessed via GP-relative addressing.

### Graphics/Rendering State (0x800A5948 - 0x800A5970)

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x800A5948 | 4 | g_FrameReady | Frame complete flag |
| 0x800A594C | 4 | g_SkipVSync | Skip VSync flag |
| 0x800A5950 | 4 | g_GameFlags | Game flags (bit 0x80=debug menu) |
| 0x800A5960 | 4 | g_GameStatePtr | Pointer to active GameState |

### Input State (0x800A5754 - 0x800A5770)

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x800A5754 | 4 | g_pPlayerState | Player persistent state |
| 0x800A5764 | 4 | g_pPlayer1Input | Player 1 input state |
| 0x800A5768 | 4 | g_pPlayer2Input | Player 2 input state |
| 0x800A576C | 4 | g_pCurrentInputState | Active input state |

### Background Color (0x800A5770 - 0x800A5773)

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x800A5770 | 1 | g_DefaultBGColorR | Default BG red |
| 0x800A5771 | 1 | g_DefaultBGColorG | Default BG green |
| 0x800A5772 | 1 | g_DefaultBGColorB | Default BG blue |

### BLB/CD State (0x800A59F0 - 0x800A6070)

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x800A59F0 | 4 | g_GameBLBSector | BLB starting sector (0x146) |
| 0x800A6060 | 4 | g_pSecondarySpriteBank | Secondary sprite bank (often NULL) |
| 0x800A6064 | 4 | g_pLevelDataContext | Current level context pointer |

### Game Mode (0x800A6082)

| Address | Size | Name | Purpose |
|---------|------|------|---------|
| 0x800A6082 | 1 | g_CurrentGameMode | Current game mode (0-6) |

## .rodata Section (0x800A60C4+)

Read-only strings and constants.

### Debug Menu Strings (0x800A60C4 - 0x800A6124)

```
0x800A60C4: "sub 01\0"
0x800A60CC: "sub 02\0"
0x800A60D4: "sub 03\0"
...
0x800A610C: "sub 10\0"
0x800A6114: "> \0"     (cursor prefix)
0x800A6118: "  \0"     (empty prefix)
```

### BLB Header Buffer (0x800AE3E0+)

After game boots, BLB header is loaded to 0x800AE3E0.
- Level count at +0xF31 (offset 0x800AF311)
- Movie count at +0xF32
- Level index at +0xF92 (current level being loaded)

## Memory Layout Diagram

```
0x8009B000 ┌────────────────────────────────┐
           │ Motion tables                   │
           │ Menu sprite tables              │
0x8009B4B4 │ g_GameBLBFile (CdlFILE)        │
           │ Collectible sprite tables       │
0x8009BA48 │ Boss sprite tables              │
0x8009BBE0 │ Enemy sprite tables             │
0x8009C174 │ g_PlayerSpriteTable             │
           │ Menu/UI sprite tables           │
0x8009CC30 │ g_SPUVoicePitchBuffer          │
0x8009D5F8 │ g_EntityTypeCallbackTable       │
           │ (121 entries × 8 bytes)         │
0x8009D9C0 │ g_TriggerZoneColorTable         │
0x8009D9FC │ Entity type sprite lists        │
0x8009DAE0 │ g_CheatPatternTable             │
0x8009DC40 │ g_GameState (416 bytes)         │
0x8009DDE0 │ g_MenuItemNames                 │
0x800A5950 ├────────────────────────────────┤
           │ .sdata section                  │
           │ (GP-relative globals)           │
0x800A60C4 ├────────────────────────────────┤
           │ .rodata section                 │
           │ (debug menu strings)            │
0x800AE3E0 ├────────────────────────────────┤
           │ BLB header buffer               │
           │ (loaded at runtime)             │
           └────────────────────────────────┘
```

## Related Documentation

- [game_state.h](../../include/Game/game_state.h) - GameState struct definition
- [game-functions.md](game-functions.md) - Function addresses
- [sprites.md](../systems/sprites.md) - Sprite array details
- [entities.md](../systems/entities.md) - Entity system
