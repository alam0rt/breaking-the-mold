# Level Flags Reference

**Location**: TileHeader + 0x18 (16-bit word)  
**Accessor**: `GetLevelFlags` @ 0x8007b47c  
**Verified**: 2026-01-20 via Ghidra decompilation

## Quick Reference

| Bit | Hex | Name | Description |
|-----|-----|------|-------------|
| 0 | 0x0001 | MENU_CURSOR | Creates menu cursor entity |
| 1 | 0x0002 | BOSS_DEFEATED_INIT | Initial boss defeated state |
| 2 | 0x0004 | LEVEL_GLIDE | Glide player mode (flying, no gravity) |
| 3 | 0x0008 | FAST_CAMERA | Camera scroll speed = 0xC000 |
| 4 | 0x0010 | LEVEL_SOAR | SOAR player mode (Bird-Man flying) |
| 5 | 0x0020 | ENTITY_POOL | Enables 256-entry entity pool array |
| 6 | 0x0040 | (reserved) | Unknown/unused |
| 7 | 0x0080 | SLOW_CAMERA | Camera scroll speed = 0x8000 |
| 8 | 0x0100 | LEVEL_RUNN | RUNN player mode (auto-runner) |
| 9 | 0x0200 | LEVEL_MENU | Menu screen (no physical player) |
| 10 | 0x0400 | LEVEL_FINN | FINN player mode (swamp boat) |
| 11 | 0x0800 | AUTO_SCROLL | Level auto-scrolls |
| 12 | 0x1000 | NO_CAMERA | Disables camera entity creation |
| 13 | 0x2000 | LEVEL_BOSS | Boss fight mode |
| 14 | 0x4000 | SHOW_HUD | Display HUD elements |
| 15 | 0x8000 | DEBUG | Debug mode enabled |

## Player Types

The game uses 7 different player entity types based on level flags:

| Priority | Flag | Player Type | Function | Size | Hitbox |
|----------|------|-------------|----------|------|--------|
| 1 | 0x400 | FINN | `CreateFinnPlayerEntity` | 0x114 | 0×-80 |
| 2 | 0x200 | MENU | `InitMenuEntity` | 0x140 | 0×0 |
| 3 | 0x2000 | BOSS | `CreateBossPlayerEntity` | 0x158 | 0×0 |
| 4 | 0x100 | RUNN | `CreateRunnPlayerEntity` | 0x110 | 40×-48 |
| 5 | 0x010 | SOAR | `CreateSoarPlayerEntity` | 0x128 | 40×-96 |
| 6 | 0x004 | GLIDE | `CreateGlidePlayerEntity` | 0x11C | 0×0 |
| 7 | (none) | NORMAL | `CreatePlayerEntity` | 0x1B4 | 40×-48 |

Checked in cascade order by `SpawnPlayerAndEntities` @ 0x8007df38.

## Camera Scroll Speed

Based on flags 0x0080 and 0x0008:

| Flag | Speed | Notes |
|------|-------|-------|
| 0x80 set | 0x8000 | Slowest scroll |
| 0x08 set | 0xC000 | Fast scroll |
| Neither | 0x10000 | Normal scroll |

## Helper Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `GetLevelFlags` | 0x8007b47c | Returns TileHeader+0x18 |
| `GetLevelDebugFlag` | 0x80082748 | Returns (flags >> 15) - bit 15 |
| `GetLevelShowHUDFlag` | 0x8008276c | Returns (flags >> 14) & 1 - bit 14 |
| `GetLevelAutoScrollFlag` | 0x80082818 | Returns (flags >> 11) & 1 - bit 11 |
| `IsNormalPlatformLevel` | 0x8008200c | True if no special player flags |

## IsNormalPlatformLevel

Returns TRUE only when ALL these flags are **clear**:
- 0x0400 (FINN)
- 0x0200 (MENU)
- 0x2000 (BOSS)
- 0x0100 (RUNN)
- 0x0010 (SOAR)
- 0x0004 (GLIDE)

Used by cheat system to restrict RGB effects to normal platformer levels.

## Cheat Restrictions

Cheats 0x10, 0x11, 0x14, 0x15 call `IsNormalPlatformLevel` before applying effects:
- **Cheat 0x10**: `ApplyRandomRGBEffect` - random tint on player
- **Cheat 0x11**: Sets `player_entity_alt[0x1AF] = 1`
- **Cheat 0x14**: Sets `player_entity_alt[0x1B0] = 1`
- **Cheat 0x15**: Sets `player_entity_alt[0x1B1] = 1`

These cheats are disabled on vehicle/boss/menu levels to prevent breaking special modes.

## Known Level Flag Combinations

| Level Type | Typical Flags | Description |
|------------|---------------|-------------|
| Normal platformer | 0x4000 | Standard Klaymen controls |
| FINN stage | 0x4400 | Swamp boat vehicle |
| RUNN stage | 0x4100 | Auto-runner |
| SOAR stage | 0x4010 | Bird-Man flying |
| GLIDE stage | 0x4024 | Flying + entity pool |
| Boss fight | 0x6000 | BOSS + HUD |
| Menu screen | 0x0201 | Menu + cursor |

## Related Documentation

- `docs/systems/gamestate-field-analysis.md` - Section 7 (detailed analysis)
- `docs/blb/tile-header-format.md` - TileHeader structure
- `docs/systems/player/` - Player state machine per type
