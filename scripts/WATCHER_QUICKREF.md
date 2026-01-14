# Game Watcher Quick Reference

## Load Watcher
```lua
dofile("scripts/game_watcher.lua")
```

## View Current State
```lua
status()        -- Full game state overview
entities()      -- List all active entities
stats()         -- Capture statistics
snapshot()      -- Get state as Lua table
```

## Level Management
```lua
levels()                      -- List all 26 levels
level_info(1)                 -- Show level 1 (SCIE) details
load_level(1, 0)              -- Queue SCIE Stage 0
load_level_by_id("PHRO", 2)   -- Queue Pharaoh Stage 2
```

## Boot Override (must reset after)
```lua
set_boot_override(5, 1)       -- Boot to MOSS Stage 1
reload_watchers()             -- Apply changes
PCSX.reset()                  -- Reset game
```

## Logging
```lua
dump_log()                    -- Save to /tmp/skullmonkeys_trace.jsonl
clear_log()                   -- Clear captured data
export_trace()                -- Full JSON export with metadata
```

## Cleanup
```lua
cleanup()                     -- Remove all breakpoints
reload_watchers()             -- Reapply with new config
```

## Level Indices
| # | ID | Name | Stages |
|--:|:---|:-----|:------:|
| 0 | MENU | Main Menu | 6 |
| 1 | SCIE | Science Castle | 5 |
| 2 | TMPL | Neverhood | 5 |
| 3 | PHRO | Pharaoh Room | 5 |
| 4 | FINN | Submarine | 3 |
| 5 | MOSS | Mushroom Forest | 5 |
| 6 | BOIL | Boiler Room | 5 |
| 7 | FOOD | Food Processing | 5 |
| 8 | BRG1 | Bridge 1 | 5 |
| 9 | GLID | Glide Course | 3 |
| 10 | CAVE | Crystal Cavern | 5 |
| 11 | KLOG | Klogg's Throne | 5 |
| 12 | WEED | Weed | 5 |
| 13 | HEAD | Headless | 5 |
| 14 | WIZZ | Wizard | 5 |
| 15 | EGGS | Egg Hatchery | 5 |
| 16 | SOAR | Soaring Course | 3 |
| 17 | GLEN | Mushroom Glen | 5 |
| 18 | CRYS | Crystal City | 5 |
| 19 | CLOU | Cloud Castle | 5 |
| 20 | SNOW | Snow World | 5 |
| 21 | CSTL | Evil's Castle | 5 |
| 22 | RUNN | Runner Course | 3 |
| 23 | EVIL | Evil Engine | 5 |
| 24 | MEGA | Mega Battle | 3 |
| 25 | SEVN | Bonus Smash | 2 |

## Common Workflows

### Capture Full Level Playthrough
```lua
clear_log()
load_level_by_id("SCIE", 0)
-- Play through level
dump_log()
```

### Test Specific Level from Boot
```lua
set_boot_override(1, 0)  -- SCIE Stage 0
reload_watchers()
PCSX.reset()
-- Game will start at SCIE
```

### Find Entity Type in Level
```lua
entities()
-- Look for entity type numbers
-- Check docs/reference/entity-types.md for mappings
```

### Export for evil-engine Testing
```lua
export_trace("/tmp/scie_stage0_trace.json")
-- Copy to evil-engine/traces/
-- Load in trace_player.gd
```

## JSON Event Types
- `FrameState` - Full state snapshot
- `PlayerStateChange` - Callback change
- `PlayerSpriteChange` - Sprite ID update
- `PlayerMove` - Position change
- `LoadAssetContainer` - BLB asset load
- `LevelLoad` - Level initialization
- `SpawnEntities` - Entity spawn

## Configuration
Edit in `scripts/game_watcher.lua`:
```lua
CONFIG.sample_every_n_frames = 5  -- Lighter sampling
CONFIG.dump_all_entities = false  -- Player only
CONFIG.watch_collision = false    -- Reduce noise
CONFIG.max_log_entries = 100000   -- Larger traces
```
