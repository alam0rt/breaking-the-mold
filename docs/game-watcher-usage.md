# Skullmonkeys Game Watcher v2.5 - Usage Guide

The enhanced game watcher provides comprehensive runtime debugging for the Skullmonkeys (SLES-01090) decompilation project. It captures frame-by-frame game state, BLB asset loading, entity behavior, and allows boot-time level selection.

## Features

### ✅ Runtime Debugging
- **Frame-by-frame state capture**: Full game state every N frames
- **Entity tracking**: Position, velocity, animation, hitboxes for all entities
- **Player state machine**: Detailed callback tracking (Idle, Walk, Jump, etc.)
- **Animation/sprite changes**: Track sprite ID changes and frame progression
- **Collision detection**: Tile collision and entity-entity collision events
- **Memory-mapped addresses**: Verified against Ghidra decompilation

### ✅ BLB Structure Logging
- **Level metadata**: Name, ID, stage count, sector offsets from BLB header
- **Asset container tracking**: LoadAssetContainer calls with segment type
- **Audio uploads**: SPU sample uploads with size tracking
- **Current level/stage**: Real-time BLB index monitoring

### ✅ Boot-Time Level Selection
- **Memory patching**: Override level/stage at game boot
- **Runtime level switching**: Change level mid-game (requires mode transition)
- **Level browser**: List all 26 levels with metadata

### ✅ Structured JSON Export
- **JSONL format**: One JSON object per line for streaming/parsing
- **Rich metadata**: BLB info, statistics, config embedded
- **Cross-comparison ready**: Format designed for evil-engine validation

## Quick Start

### 1. Load the Watcher

In PCSX-Redux:
```lua
-- Load the script
dofile("scripts/game_watcher.lua")
```

Or via command line:
```bash
pcsx-redux -dofile scripts/game_watcher.lua -iso disks/pal/SLES_010.90 -run -web
```

### 2. Basic Commands

```lua
-- Show current game state
status()

-- List all levels
levels()

-- Show level details
level_info(1)  -- SCIE level

-- Get current frame snapshot
snap = snapshot()
print(snap.player.x, snap.player.y)

-- Show capture statistics
stats()
```

### 3. Level Loading

```lua
-- Load level by index and stage
load_level(1, 0)  -- SCIE Stage 0

-- Load level by ID
load_level_by_id("PHRO", 2)  -- Pharaoh Room Stage 2

-- The game will load the level on next mode transition
-- (death, exit, or reset)
```

### 4. Boot-Time Override

To force a specific level at game boot:

```lua
-- Set boot override
set_boot_override(5, 1)  -- MOSS Stage 1

-- Reload watchers to apply
reload_watchers()

-- Reset the game
PCSX.reset()
```

Or configure in the script before loading:
```lua
CONFIG.boot_level_override = {level = 5, stage = 1}
```

### 5. Logging and Export

```lua
-- Save trace to JSONL
dump_log()
-- Writes to /tmp/skullmonkeys_trace.jsonl

-- Export with full metadata
export_trace("/tmp/skullmonkeys_full_trace.json")

-- Clear log to start fresh
clear_log()
```

## Configuration Options

Edit `CONFIG` table in `game_watcher.lua`:

### Watcher Categories
```lua
watch_frame_state = true      -- Full state dumps each frame
watch_entity_tick = true      -- EntityTickLoop tracking
watch_entity_callbacks = true -- Entity state changes
watch_player = true           -- Player-specific tracking
watch_animation = true        -- Sprite/animation changes
watch_collision = true        -- Collision detection
watch_level_load = true       -- Level loading events
watch_blb_access = true       -- BLB asset loading
watch_memory_ops = false      -- Memory alloc (very verbose!)
```

### Dump Options
```lua
dump_all_entities = true      -- Include all entities in frame dumps
dump_blb_metadata = true      -- Include BLB level info in frames
dump_asset_info = true        -- Track asset container loads
sample_every_n_frames = 1     -- Full dump frequency (1 = every frame)
max_log_entries = 50000       -- Auto-stop limit
```

### Logging
```lua
log_file_path = "/tmp/skullmonkeys_trace.jsonl"
log_to_console = false        -- Print all entries to console
console_summary_only = true   -- Only print summaries
log_only_changes = false      -- Skip unchanged frames
```

## JSON Output Format

### JSONL Trace Format

Each line is a JSON object representing one event:

```json
{"frame":1234,"type":"FrameState","data":{"camera":{"x":100,"y":200},"entity_count":15,"blb":{"valid":true,"current":{"level_index":1,"stage_index":0,"game_mode":3},"level":{"index":1,"id":"SCIE","name":"Science Castle","stage_count":5}},"player":{"x":152,"y":240,"vx":2.5,"vy":0,"facing":"right","state":"Walk","sprite_id":570982936,"sprite_id_hex":"0x21842018","anim_frame":3,"anim_end":8},"entities":[...]}}

{"frame":1235,"type":"PlayerStateChange","data":{"callback":"Jump","callback_addr":"0x8005BD60","state_data":"0x00000000"}}

{"frame":1240,"type":"LoadAssetContainer","data":{"level_id":"SCIE","stage_index":0,"asset_index":1,"segment_type":"secondary","load_mode":1}}
```

### Event Types

| Type | Description | Key Fields |
|------|-------------|------------|
| `FrameState` | Full game state snapshot | `camera`, `player`, `entities`, `blb` |
| `PlayerStateChange` | Player callback change | `callback`, `callback_addr` |
| `PlayerSpriteChange` | Sprite ID change | `sprite_id`, `sprite_hex` |
| `PlayerAnimTick` | Animation frame advance | `frame`, `end_frame`, `timer` |
| `PlayerMove` | Position change | `x`, `y`, `vx`, `vy`, `input` |
| `PlayerCollisionCheck` | Entity collision test | `other_entity`, `other_type` |
| `EntityStateChange` | Non-player entity state | `entity`, `entity_type`, `callback` |
| `LevelLoad` | Level initialization | `blb_metadata`, `timestamp` |
| `SpawnEntities` | Entity spawn from Asset 501 | `level_id`, `stage_index` |
| `LoadAssetContainer` | BLB asset load | `asset_index`, `segment_type` |
| `UploadAudioToSPU` | Audio sample upload | `size_bytes`, `size_kb` |
| `TileAttrCheck` | Tile collision lookup | `x`, `y`, `tile_x`, `tile_y` |

### Structured Export Format

`export_trace()` produces a single JSON file with metadata:

```json
{
  "metadata": {
    "version": "2.0",
    "game": "Skullmonkeys",
    "region": "PAL",
    "game_version": "SLES-01090",
    "export_time": 1736889600,
    "frame_count": 5000,
    "log_entries": 12543
  },
  "statistics": {
    "total_entities_seen": 0,
    "player_state_changes": 45,
    "sprite_changes": 23,
    "level_loads": 1
  },
  "blb_info": {
    "valid": true,
    "current": {"level_index": 1, "stage_index": 0},
    "level": {"id": "SCIE", "name": "Science Castle"}
  },
  "config": {
    "sample_rate": 1,
    "dump_all_entities": true
  },
  "entries": [ /* all log entries */ ]
}
```

## Advanced Usage

### Trace Replay in evil-engine

The trace format is designed for evil-engine verification:

```gdscript
# In evil-engine demo/trace_player.gd
var trace := GameTracePlayer.new()
trace.load_trace("res://traces/scie_stage0.jsonl")
trace.game_runner = $GameRunner
trace.play()

# Verify positions frame-by-frame
assert(trace.player_pos_matches(152, 240, tolerance=1))
```

### Custom Breakpoint Handlers

Add your own breakpoints:

```lua
-- Track a specific function
state.breakpoints.my_func = PCSX.addBreakpoint(
    0x80012345, 'Exec', 4, 'MyFunction',
    function(addr, width, cause)
        local regs = PCSX.getRegisters()
        log_entry("MyEvent", {
            param1 = regs.GPR.n.a0,
            param2 = regs.GPR.n.a1,
        })
    end
)
```

### Memory Inspection During Gameplay

```lua
-- Pause game
PCSX.pause()

-- Read arbitrary memory
local tile_header = read_u32(0x8009DCC8)  -- LevelDataContext+4
print(string.format("Tile header at 0x%08X", tile_header))

-- Read player position
local player_ptr = get_player_ptr()
local x = read_s16(player_ptr + 0x68)
local y = read_s16(player_ptr + 0x6A)
print(string.format("Player at (%d, %d)", x, y))

-- Resume
PCSX.resume()
```

## Troubleshooting

### Watcher Not Loading
- Ensure PCSX-Redux Lua console is enabled
- Check for syntax errors: `lua -c scripts/game_watcher.lua`
- Verify memory access: `status()` should show player data

### No Log Entries
- Check `CONFIG.watch_*` flags are enabled
- Ensure game is running (not paused)
- Verify `max_log_entries` not reached: `stats()`

### Boot Override Not Working
- Call `reload_watchers()` after `set_boot_override()`
- Reset the game: `PCSX.reset()`
- Verify indices: `level_info(idx)` before override

### Level Won't Load
- Level loads require mode transition (death/exit/reset)
- Check game is past intro: `get_current_level().game_mode == 3`
- Verify level index: `levels()` to see valid indices

## Memory Address Reference

All addresses verified against Ghidra decompilation (SLES-01090):

| Symbol | Address | Description |
|--------|---------|-------------|
| `g_GameStateBase` | 0x8009DC40 | Main GameState structure |
| `LevelDataContext` | 0x8009DCC4 | GameState+0x84, level context |
| `g_pPlayerState` | 0x800A5754 | Player save/progression data |
| `BLBHeader` | 0x800AE3E0 | Loaded BLB header (4KB) |
| `EntityCallbackTable` | 0x8009D5F8 | 121 entity type callbacks |
| `InitializeAndLoadLevel` | 0x8007D1D0 | Main level loader |
| `EntityTickLoop` | 0x80020E1C | Main entity update |
| `LoadAssetContainer` | 0x8007B074 | BLB asset loader |
| `UploadAudioToSPU` | 0x8007C088 | SPU sample upload |

See `scripts/game_watcher.lua` `ADDR` table for complete list.

## Integration with Decompilation

### Verifying Function Addresses

Use traces to verify Ghidra addresses:

```bash
# Extract all LoadAssetContainer calls
grep LoadAssetContainer /tmp/skullmonkeys_trace.jsonl | jq .data

# Count entity state changes per type
grep EntityStateChange /tmp/skullmonkeys_trace.jsonl | jq .data.entity_type | sort | uniq -c
```

### Asset Loading Analysis

Track which assets are loaded when:

```bash
# Find all secondary segment loads
grep '"segment_type":"secondary"' /tmp/skullmonkeys_trace.jsonl

# Track audio uploads per level
grep UploadAudioToSPU /tmp/skullmonkeys_trace.jsonl | jq '{level:.data.level_id, size_kb:.data.size_kb}'
```

## Performance Notes

- **Frame dumps**: ~5KB per frame with all entities
- **Sampling**: Use `sample_every_n_frames = 5` for lighter traces
- **Storage**: 50K entries ≈ 250MB JSONL file
- **Overhead**: Minimal when `log_to_console = false`

## See Also

- `docs/blb-data-format.md` - BLB file structure
- `docs/entity-system.md` - Entity architecture
- `docs/runtime-behavior.md` - Game loop and callbacks
- `scripts/blb.hexpat` - ImHex BLB template (source of truth)
