--[[
  Skullmonkeys (SLES-01090 PAL) Comprehensive Game Watcher v2.0

  This script captures comprehensive game state each frame for analysis
  and replication in evil-engine.

  Load in PCSX-Redux via: Debug > Lua Console or Editor
  Or use: pcsx-redux -dofile scripts/game_watcher.lua

  Output: Structured JSONL to /tmp/skullmonkeys_trace.jsonl

  Captures:
    - Frame-by-frame entity tick loop state
    - All entity positions, callbacks, and animation
    - Player state machine transitions
    - Tile collision checks
    - Level loading events
    - Camera position
]]

-- =============================================================================
-- Configuration
-- =============================================================================

local CONFIG = {
    -- Master enable/disable for watcher categories
    watch_frame_state = true,      -- Full state dump each frame
    watch_entity_tick = true,      -- EntityTickLoop breakpoint
    watch_entity_callbacks = false, -- Individual entity callbacks
    watch_player = true,           -- Player-specific state changes
    watch_animation = true,        -- Sprite/animation changes
    watch_collision = false,       -- Tile and entity collision (very verbose)
    watch_level_load = true,       -- Level loading events
    watch_blb_access = true,       -- BLB header and asset access
    watch_memory_ops = false,      -- Memory allocations (very verbose)

    -- Debugging
    debug_breakpoints = false,     -- Print when breakpoints fire (very verbose!)

    -- Frame state dump options
    dump_all_entities = true,     -- Include ALL active entities each frame
    dump_player_only = false,     -- Only dump player state (faster)
    max_entities_per_frame = 64,  -- Cap entity dumps per frame
    dump_blb_metadata = true,     -- Include BLB level metadata in frames
    dump_asset_info = true,       -- Include asset container info

    -- Logging
    log_to_console = false,       -- Print to Lua console (verbose!)
    console_summary_only = true,  -- Only print summaries to console
    log_dir = "game_watcher/logs",  -- Directory for log files
    stream_to_file = true,        -- Stream logs to file immediately (less memory)
    log_format_pretty = false,    -- Pretty-print JSON (larger files)
    auto_flush_interval = 60,     -- Flush file buffer every N frames

    -- Stdout event filtering (what to print to console)
    stdout_events = {
        WatcherStarted = true,     -- Initial startup message
        BootOverride = true,       -- Boot-time level override applied
        LevelLoad = true,          -- Level loading started
        SpawnEntities = true,      -- Entity spawn completed
        GameStateTick = false,     -- Complete frame state for replay (very verbose)
        FrameState = false,        -- Full frame state (very verbose)
        PlayerState = false,       -- Player state changes
        PlayerMove = false,        -- Player movement
        PlayerAnim = false,        -- Player animation changes
        PlayerVelocity = false,    -- Player velocity samples (high frequency)
        EntitySetState = false,    -- Entity state machine transitions
        SetEntitySpriteId = false, -- Sprite ID changes
        TickEntityAnimation = false, -- Animation tick events
        PlayerCollisionCheck = false, -- Collision checks
        TileAttrCheck = false,     -- Tile attribute lookups
        LoadAsset = false,         -- Asset container loading
        UploadAudio = false,       -- SPU audio uploads
        Marker = true,             -- User-added markers
    },

    -- Sampling
    sample_every_n_frames = 300,  -- Full dump every N frames (300 = every 5 seconds at 60fps)
    sample_velocity_every_n_frames = 4,  -- Log velocity every N frames (4 = 15Hz, good for physics)
    max_log_entries = 50000,      -- Max entries before auto-stop

    -- Change detection (reduce noise)
    log_only_changes = false,     -- Only log when state changes
    log_velocity_changes_only = false,  -- Only log velocity when it changes (false = log every N frames)

    -- Boot-time level override (nil = normal boot)
    boot_level_override = {level=1, stage=0},    -- Set to {level=1, stage=0} to force SCIE Stage 0
    boot_stage_override = nil,    -- Deprecated: use boot_level_override table
}

-- =============================================================================
-- Memory Addresses (PAL / SLES-01090)
-- =============================================================================

local ADDR = {
    -- Core structures
    GameState = 0x8009DC40,
    LevelDataContext = 0x8009DCC4,
    BLBHeader = 0x800AE3E0,
    EntityCallbackTable = 0x8009D5F8,  -- 121 entries × 8 bytes

    -- GameState offsets (from 0x8009DC40)
    GS_entity_tick_list = 0x1C,    -- Head of tick-sorted entity list
    GS_entity_render_list = 0x20,  -- Head of render-sorted list
    GS_collision_list = 0x24,      -- Entity collision queue
    GS_entity_pool = 0x28,         -- Raw entity definitions
    GS_player_alt = 0x2C,          -- Alternate player ref
    GS_player = 0x30,              -- Main player entity pointer
    GS_camera_x = 0x38,            -- Camera X position (s16)
    GS_camera_y = 0x3C,            -- Camera Y position (s16)
    GS_callback_table = 0x7C,      -- Entity type callback table ptr
    GS_checkpoint_list = 0x134,    -- Saved entity list for respawn

    -- Entity structure offsets (0x44C bytes total)
    ENT_next = 0x00,               -- Next entity in list (u32 ptr)
    ENT_callback_main = 0x04,      -- Main tick callback (u32 fn ptr)
    ENT_callback_render = 0x08,    -- Render callback (u32 fn ptr)
    ENT_entity_type = 0x0C,        -- Entity type ID (u16)
    ENT_flags = 0x0E,              -- Entity flags (u16)
    ENT_layer = 0x10,              -- Render layer (u16)
    ENT_z_order = 0x12,            -- Z-order for sorting (s16)

    -- Entity position/physics
    ENT_x_whole = 0x48,            -- X position whole part (s16)
    ENT_y_whole = 0x4A,            -- Y position whole part (s16)
    ENT_x_frac = 0x4C,             -- X position fractional (u16)
    ENT_y_frac = 0x4E,             -- Y position fractional (u16)
    ENT_x_pos = 0x68,              -- X pixel position (s16) - used for rendering
    ENT_y_pos = 0x6A,              -- Y pixel position (s16)
    ENT_facing = 0x74,             -- Facing direction (0=right, 1=left)

    -- Entity velocity (fixed-point 16.16)
    ENT_vx = 0xB4,                 -- X velocity (s32, 16.16 fixed)
    ENT_vy = 0xB8,                 -- Y velocity (s32, 16.16 fixed)

    -- Entity animation/sprite
    ENT_sprite_data_ptr = 0x78,    -- Pointer to sprite animation data
    ENT_sprite_id = 0xBC,          -- Current sprite ID (u32)
    ENT_anim_timer = 0xD8,         -- Animation timer (u16)
    ENT_anim_frame = 0xDA,         -- Current animation frame (u16)
    ENT_anim_speed = 0xDC,         -- Animation speed (u16)
    ENT_anim_end = 0xDE,           -- End frame of current anim (u16)

    -- Entity state machine
    ENT_state_ptr = 0xA0,          -- Current state data pointer
    ENT_state_timer = 0xA4,        -- State timer (u16)
    ENT_state_data = 0xA8,         -- State-specific data (varies)

    -- Entity collision box
    ENT_hitbox_x1 = 0x1C,          -- Hitbox left (s16)
    ENT_hitbox_y1 = 0x1E,          -- Hitbox top (s16)
    ENT_hitbox_x2 = 0x20,          -- Hitbox right (s16)
    ENT_hitbox_y2 = 0x22,          -- Hitbox bottom (s16)

    -- Player-specific offsets (within 0x1B4 byte player entity)
    PLAYER_health = 0xF0,          -- Player health/lives
    PLAYER_invuln_timer = 0xF4,    -- Invulnerability frames
    PLAYER_input = 0x100,          -- Current input state
    PLAYER_input_prev = 0x102,     -- Previous input state

    -- Key functions to watch
    FN_EntityTickLoop = 0x80020E1C,         -- Main entity update loop
    FN_EntitySetState = 0x8001EAAC,         -- Set entity state/callback
    FN_SetEntitySpriteId = 0x8001D080,      -- Change entity sprite
    FN_InitEntityAnimationState = 0x8001D1D0, -- Init animation state
    FN_TickEntityAnimation = 0x8001D290,    -- Advance animation frame
    FN_UpdateSpriteFrameData = 0x8001D748,  -- Update sprite frame ptr
    FN_CreatePlayerEntity = 0x800596A4,     -- Create player
    FN_PlayerTickCallback = 0x80059E10,     -- Player main tick
    FN_CheckEntityCollision = 0x800226F8,   -- Entity-entity collision
    FN_GetTileAttributeAtPosition = 0x800241F4, -- Tile collision lookup
    FN_LoadAssetContainer = 0x8007B074,     -- Load asset from BLB
    FN_InitializeAndLoadLevel = 0x8007D1D0, -- Main level init (verified)
    FN_SpawnPlayerAndEntities = 0x8007DF38, -- Spawn entities from defs
    FN_RemapEntityTypes = 0x8008150C,       -- Convert BLB→internal types
    FN_SeekToLevelInSequence = 0x8007A3AC,  -- Set level in playback sequence
    FN_LoadBLBHeader = 0x800208B0,          -- Load BLB header from disc (CRITICAL)
    FN_InitLevelDataContext = 0x8007A1BC,   -- Init context with header (SAFE POINT)
    FN_InitGameState = 0x8007CD34,          -- Called from main, loads header
    FN_UploadAudioToSPU = 0x8007C088,       -- Upload audio samples
    FN_LoadTileDataToVRAM = 0x80025240,     -- Load tile graphics

    -- BLB Header structure (loaded at 0x800AE3E0, verified via symbol_addrs.txt)
    BLB_level_count = 0x800AF311,           -- u8: number of levels (26) @ header+0xF31
    BLB_movie_count = 0x800AF312,           -- u8: number of movies (13) @ header+0xF32
    BLB_sector_count = 0x800AF313,          -- u8: sector table entries @ header+0xF33
    BLB_game_mode = 0x800AF316,             -- u8: 3=level, 6=special @ header+0xF36
    BLB_level_index = 0x800AF372,           -- u8: current level index @ header+0xF92
    BLB_stage_index = 0x800AF373,           -- u8: current stage index @ header+0xF93

    -- g_pPlayerState verified at 0x800A5754 (stores level/stage progression)
    g_pPlayerState = 0x800A5754,            -- u8* pointer to player state/save data

    -- Level metadata table (26 entries × 0x70 bytes starting at BLBHeader)
    BLB_level_table = 0x800AE3E0,           -- Level metadata table base
    LEVEL_ENTRY_SIZE = 0x70,                -- Size of each level entry
    LEVEL_ID_OFFSET = 0x56,                 -- char[5] level ID (e.g. "SCIE")
    LEVEL_NAME_OFFSET = 0x5B,               -- char[21] level name
    LEVEL_STAGE_COUNT_OFFSET = 0x0C,        -- u8 stage count
    LEVEL_FLAGS_OFFSET = 0x0D,              -- u8 flags
}

-- Player callback functions (state machine) - extended list
local PLAYER_CALLBACKS = {
    [0x8005BAD0] = "Idle",
    [0x8005BB64] = "IdleTick",
    [0x8005BB80] = "IdleBlink",
    [0x8005BBAC] = "IdleLook",
    [0x8005BBE0] = "Walk",
    [0x8005BC3C] = "WalkStart",
    [0x8005BD60] = "Jump",
    [0x8005BDB4] = "JumpAscend",
    [0x8005BE6C] = "JumpDescend",
    [0x8005BF5C] = "Fall",
    [0x8005C01C] = "Land",
    [0x8005C0A0] = "Crouch",
    [0x8005C160] = "CrouchWalk",
    [0x8005CC84] = "Climb",
    [0x8005CCE0] = "ClimbIdle",
    [0x8005CEC8] = "ClimbUp",
    [0x8005D0C8] = "ClimbDown",
    [0x8005D25C] = "Hang",
    [0x8005D404] = "Swing",
    [0x8005DAA4] = "Hurt",
    [0x8005DB74] = "Die",
    [0x8005DDE0] = "Respawn",
    [0x8005DEB0] = "Slide",
    [0x8005E170] = "Push",
    [0x8005E248] = "Pickup",
    [0x8005E450] = "Throw",
    [0x8005E510] = "Swim",
    [0x8005E574] = "SwimSurface",
    [0x8005E650] = "Bounce",
    [0x8005EA80] = "Special",
}

-- Entity type names (from decompilation)
local ENTITY_TYPES = {
    [0] = "None",
    [1] = "Player",
    [2] = "Camera",
    [3] = "HUD",
    [7] = "Collectible",
    [8] = "Swirly",
    [9] = "Checkpoint",
    [10] = "Platform",
    [11] = "Enemy",
    [24] = "Trigger",
}

-- =============================================================================
-- Memory Access Helpers
-- =============================================================================

local mem = PCSX.getMemPtr()
local PSX_RAM_SIZE = 0x200000  -- 2MB PSX RAM

-- Check if pointer is valid PSX RAM address
local function is_valid_ptr(addr)
    return addr >= 0x80000000 and addr < 0x80200000
end

-- Convert PSX address to memory offset (strip high bits for kuseg/kseg0/kseg1)
local function psx_to_offset(addr)
    return bit.band(addr, 0x1FFFFF)
end

local function read_u8(addr)
    if not is_valid_ptr(addr) then return 0 end
    local off = psx_to_offset(addr)
    if off < 0 or off >= PSX_RAM_SIZE then return 0 end
    return mem[off]
end

local function read_u16(addr)
    if not is_valid_ptr(addr) then return 0 end
    local off = psx_to_offset(addr)
    if off < 0 or off + 1 >= PSX_RAM_SIZE then return 0 end
    return mem[off] + mem[off + 1] * 256
end

local function read_s16(addr)
    local v = read_u16(addr)
    if v >= 0x8000 then v = v - 0x10000 end
    return v
end

local function read_u32(addr)
    if not is_valid_ptr(addr) then return 0 end
    local off = psx_to_offset(addr)
    if off < 0 or off + 3 >= PSX_RAM_SIZE then return 0 end
    return mem[off] + mem[off + 1] * 256 + mem[off + 2] * 65536 + mem[off + 3] * 16777216
end

local function read_s32(addr)
    local v = read_u32(addr)
    if v >= 0x80000000 then v = v - 0x100000000 end
    return v
end

-- Fixed-point 16.16 to float
local function fixed_to_float(v)
    return v / 65536.0
end

-- Write a byte to memory
local function write_u8(addr, value)
    if not is_valid_ptr(addr) then return false end
    local off = psx_to_offset(addr)
    if off < 0 or off >= PSX_RAM_SIZE then return false end
    mem[off] = bit.band(value, 0xFF)
    return true
end

-- Read a null-terminated string from memory
local function read_string(addr, max_len)
    max_len = max_len or 32
    local chars = {}
    for i = 0, max_len - 1 do
        local byte = read_u8(addr + i)
        if byte == 0 then break end
        table.insert(chars, string.char(byte))
    end
    return table.concat(chars)
end

-- =============================================================================
-- State Tracking
-- =============================================================================

local state = {
    frame_count = 0,
    log_entries = {},
    log_file = nil,              -- File handle for streaming logs
    log_file_path = nil,         -- Current log file path
    breakpoints = {},
    listeners = {},
    session_id = os.date("%Y%m%d_%H%M%S"),  -- Unique session identifier
    markers = {},  -- User-defined markers for parsing
    boot_override_applied = false,  -- Track if boot override was applied
    last_flush = 0,              -- Frame of last file flush

    -- Last known state for change detection
    last_player_state = nil,
    last_player_x = nil,
    last_player_y = nil,
    last_player_sprite = nil,
    last_anim_frame = nil,
    last_entity_count = 0,
    last_vx = nil,               -- Last velocity X for change detection
    last_vy = nil,               -- Last velocity Y for change detection

    -- Entity tracking
    entity_cache = {},           -- Cache of known entities
    entity_spawn_frame = {},     -- When entities were first seen

    -- Statistics
    stats = {
        total_entities_seen = 0,
        player_state_changes = 0,
        sprite_changes = 0,
        level_loads = 0,
    },
}

-- =============================================================================
-- JSON Serialization
-- =============================================================================

local function escape_json_string(s)
    return s:gsub('\\', '\\\\'):gsub('"', '\\"'):gsub('\n', '\\n')
end

local function to_json(value, depth)
    depth = depth or 0
    if depth > 10 then return '"<max depth>"' end

    local t = type(value)
    if t == "nil" then
        return "null"
    elseif t == "boolean" then
        return value and "true" or "false"
    elseif t == "number" then
        if value ~= value then return "null" end  -- NaN
        if value == math.huge or value == -math.huge then return "null" end
        return tostring(value)
    elseif t == "string" then
        return '"' .. escape_json_string(value) .. '"'
    elseif t == "table" then
        -- Check if array or object
        local is_array = true
        local max_idx = 0
        for k, v in pairs(value) do
            if type(k) ~= "number" or k < 1 or k ~= math.floor(k) then
                is_array = false
                break
            end
            max_idx = math.max(max_idx, k)
        end

        if is_array and max_idx > 0 then
            local parts = {}
            for i = 1, max_idx do
                parts[i] = to_json(value[i], depth + 1)
            end
            return "[" .. table.concat(parts, ",") .. "]"
        else
            local parts = {}
            for k, v in pairs(value) do
                local key = type(k) == "string" and k or tostring(k)
                table.insert(parts, '"' .. escape_json_string(key) .. '":' .. to_json(v, depth + 1))
            end
            return "{" .. table.concat(parts, ",") .. "}"
        end
    else
        return '"<' .. t .. '>"'
    end
end

-- =============================================================================
-- Logging
-- =============================================================================

local function log_entry(entry_type, data)
    if #state.log_entries >= CONFIG.max_log_entries then
        return false
    end

    local entry = {
        frame = state.frame_count,
        type = entry_type,
        data = data,
    }

    -- Stream to file if enabled, otherwise buffer in memory
    if CONFIG.stream_to_file and state.log_file then
        local json_line = to_json(entry)
        state.log_file:write(json_line .. "\n")

        -- Flush periodically to ensure data is written
        if state.frame_count - state.last_flush >= CONFIG.auto_flush_interval then
            state.log_file:flush()
            state.last_flush = state.frame_count
        end
    else
        -- Buffer in memory (old behavior)
        table.insert(state.log_entries, entry)
    end

    -- Check if this event type should be printed to stdout
    if CONFIG.stdout_events and CONFIG.stdout_events[entry_type] then
        print(string.format("[%d] %s: %s", state.frame_count, entry_type, to_json(data)))
    elseif CONFIG.log_to_console and not CONFIG.console_summary_only then
        -- Fallback to old behavior if stdout_events not configured
        print(string.format("[%d] %s: %s", state.frame_count, entry_type, to_json(data)))
    end

    return true
end

local function log_summary(msg)
    if CONFIG.console_summary_only then
        print(string.format("[%d] %s", state.frame_count, msg))
    end
end

-- =============================================================================
-- Game State Reading
-- =============================================================================

local function get_player_ptr()
    local gs = ADDR.GameState
    return read_u32(gs + ADDR.GS_player)
end

local function get_entity_tick_list_head()
    local gs = ADDR.GameState
    return read_u32(gs + ADDR.GS_entity_tick_list)
end

local function get_camera()
    local gs = ADDR.GameState
    return {
        x = read_s16(gs + ADDR.GS_camera_x),
        y = read_s16(gs + ADDR.GS_camera_y),
    }
end

local function get_callback_name(addr)
    return PLAYER_CALLBACKS[addr] or string.format("0x%08X", addr)
end

local function get_entity_type_name(type_id)
    return ENTITY_TYPES[type_id] or string.format("Type_%d", type_id)
end

-- Read full entity state from memory
local function read_entity(entity_ptr)
    if not is_valid_ptr(entity_ptr) then
        return nil
    end

    local ent = {
        ptr = string.format("0x%08X", entity_ptr),

        -- List linkage
        next = read_u32(entity_ptr + ADDR.ENT_next),

        -- Callbacks
        callback_main = read_u32(entity_ptr + ADDR.ENT_callback_main),
        callback_render = read_u32(entity_ptr + ADDR.ENT_callback_render),

        -- Identity
        entity_type = read_u16(entity_ptr + ADDR.ENT_entity_type),
        flags = read_u16(entity_ptr + ADDR.ENT_flags),
        layer = read_u16(entity_ptr + ADDR.ENT_layer),
        z_order = read_s16(entity_ptr + ADDR.ENT_z_order),

        -- Position (pixel coordinates for rendering)
        x = read_s16(entity_ptr + ADDR.ENT_x_pos),
        y = read_s16(entity_ptr + ADDR.ENT_y_pos),

        -- High-precision position (whole + frac)
        x_whole = read_s16(entity_ptr + ADDR.ENT_x_whole),
        y_whole = read_s16(entity_ptr + ADDR.ENT_y_whole),
        x_frac = read_u16(entity_ptr + ADDR.ENT_x_frac),
        y_frac = read_u16(entity_ptr + ADDR.ENT_y_frac),

        -- Velocity (fixed-point)
        vx = read_s32(entity_ptr + ADDR.ENT_vx),
        vy = read_s32(entity_ptr + ADDR.ENT_vy),

        -- Direction
        facing = read_u8(entity_ptr + ADDR.ENT_facing),

        -- Animation
        sprite_data_ptr = read_u32(entity_ptr + ADDR.ENT_sprite_data_ptr),
        sprite_id = read_u32(entity_ptr + ADDR.ENT_sprite_id),
        anim_timer = read_u16(entity_ptr + ADDR.ENT_anim_timer),
        anim_frame = read_u16(entity_ptr + ADDR.ENT_anim_frame),
        anim_speed = read_u16(entity_ptr + ADDR.ENT_anim_speed),
        anim_end = read_u16(entity_ptr + ADDR.ENT_anim_end),

        -- State machine
        state_ptr = read_u32(entity_ptr + ADDR.ENT_state_ptr),
        state_timer = read_u16(entity_ptr + ADDR.ENT_state_timer),

        -- Collision box
        hitbox_x1 = read_s16(entity_ptr + ADDR.ENT_hitbox_x1),
        hitbox_y1 = read_s16(entity_ptr + ADDR.ENT_hitbox_y1),
        hitbox_x2 = read_s16(entity_ptr + ADDR.ENT_hitbox_x2),
        hitbox_y2 = read_s16(entity_ptr + ADDR.ENT_hitbox_y2),
    }

    -- Add derived fields
    ent.type_name = get_entity_type_name(ent.entity_type)
    ent.callback_name = get_callback_name(ent.callback_main)
    ent.facing_dir = ent.facing == 0 and "right" or "left"
    ent.vx_float = fixed_to_float(ent.vx)
    ent.vy_float = fixed_to_float(ent.vy)

    return ent
end

-- Read player entity with player-specific fields
local function read_player_entity()
    local player_ptr = get_player_ptr()
    if not is_valid_ptr(player_ptr) then
        return nil
    end

    local ent = read_entity(player_ptr)
    if not ent then return nil end

    -- Add player-specific fields
    ent.is_player = true
    ent.health = read_u16(player_ptr + ADDR.PLAYER_health)
    ent.invuln_timer = read_u16(player_ptr + ADDR.PLAYER_invuln_timer)
    ent.input = read_u16(player_ptr + ADDR.PLAYER_input)
    ent.input_prev = read_u16(player_ptr + ADDR.PLAYER_input_prev)

    return ent
end

-- Iterate all entities in tick list
local function iterate_entity_list()
    local entities = {}
    local head = get_entity_tick_list_head()
    local current = head
    local count = 0
    local max_iter = CONFIG.max_entities_per_frame

    while is_valid_ptr(current) and count < max_iter do
        local ent = read_entity(current)
        if ent then
            table.insert(entities, ent)
            current = ent.next
            count = count + 1
        else
            break
        end
    end

    return entities
end

-- Get compact player state for frequent logging
local function get_player_state_compact()
    local player = read_player_entity()
    if not player then return nil end

    return {
        x = player.x,
        y = player.y,
        vx = player.vx_float,
        vy = player.vy_float,
        facing = player.facing_dir,
        state = player.callback_name,
        sprite_id = player.sprite_id,
        anim_frame = player.anim_frame,
        anim_end = player.anim_end,
        input = player.input,
    }
end

-- =============================================================================
-- BLB Metadata
-- =============================================================================

-- Get level metadata for a specific level index
local function get_level_info(level_idx)
    local base = ADDR.BLB_level_table + (level_idx * ADDR.LEVEL_ENTRY_SIZE)

    -- Read sector offsets for all stages (up to 6 stages)
    local stage_offsets = {}
    local stage_counts = {}
    for i = 0, 5 do
        local sec_off = read_u32(base + 0x1E + (i * 8))
        local sec_cnt = read_u16(base + 0x22 + (i * 8))
        if sec_cnt > 0 then
            table.insert(stage_offsets, sec_off)
            table.insert(stage_counts, sec_cnt)
        end
    end

    return {
        index = level_idx,
        id = read_string(base + ADDR.LEVEL_ID_OFFSET, 5),
        name = read_string(base + ADDR.LEVEL_NAME_OFFSET, 21),
        stage_count = read_u8(base + ADDR.LEVEL_STAGE_COUNT_OFFSET),
        flags = read_u8(base + ADDR.LEVEL_FLAGS_OFFSET),
        primary_size = read_u32(base + 0x04),
        entry1_offset = read_u32(base + 0x08),
        primary_sector = read_u32(base + 0x00),
        stage_sectors = stage_offsets,
        stage_sector_counts = stage_counts,
    }
end

-- Get current level and stage
local function get_current_level()
    return {
        level_index = read_u8(ADDR.BLB_level_index),
        stage_index = read_u8(ADDR.BLB_stage_index),
        game_mode = read_u8(ADDR.BLB_game_mode),
        level_count = read_u8(ADDR.BLB_level_count),
        movie_count = read_u8(ADDR.BLB_movie_count),
        sector_count = read_u8(ADDR.BLB_sector_count),
    }
end

-- Get comprehensive BLB metadata for current level
local function get_blb_metadata()
    local current = get_current_level()

    if current.level_index >= current.level_count then
        return {
            valid = false,
            current = current,
        }
    end

    local level_info = get_level_info(current.level_index)

    return {
        valid = true,
        current = current,
        level = level_info,
        header_addr = string.format("0x%08X", ADDR.BLB_level_table),
    }
end

-- =============================================================================
-- Breakpoint Handlers
-- =============================================================================

-- Wrapper to safely call breakpoint handlers
local function safe_breakpoint_handler(handler_name, handler_func)
    return function(address, width, cause)
        if CONFIG.debug_breakpoints then
            print(string.format("[BP] %s @ 0x%08X", handler_name, address))
        end
        local success, err = pcall(handler_func, address, width, cause)
        if not success then
            print(string.format("[ERROR] Breakpoint handler '%s' failed: %s", handler_name, tostring(err)))
            -- Log the error
            log_entry("BreakpointError", {
                handler = handler_name,
                error = tostring(err),
                address = string.format("0x%08X", address),
            })
        end
    end
end

-- Read complete GameState for replay (called at EntityTickLoop)
local function read_game_state_tick()
    local gs_base = ADDR.GameState

    -- Mode callback state (for level/mode transitions)
    local mode_offset = read_s16(gs_base + 0x00)
    local mode_index = read_s16(gs_base + 0x02)
    local mode_callback_ptr = read_u32(gs_base + 0x04)

    -- Background color change (RenderEntities clears this)
    local bg_change_flag = read_u8(gs_base + 0x130)
    local bg_r = read_u8(gs_base + 0x131)
    local bg_g = read_u8(gs_base + 0x132)
    local bg_b = read_u8(gs_base + 0x133)

    -- Camera position (critical for deterministic replay)
    local camera_x = read_s16(gs_base + 0x38)
    local camera_y = read_s16(gs_base + 0x3C)

    -- Input state pointers (to verify input capture)
    local p1_input_ptr = read_u32(gs_base + 0x50)

    -- Player entity reference
    local player_entity_ptr = read_u32(gs_base + 0x30)

    return {
        mode_offset = mode_offset,
        mode_index = mode_index,
        mode_callback = string.format("0x%08X", mode_callback_ptr),
        bg_change_flag = bg_change_flag,
        bg_color = (bg_change_flag ~= 0) and {r=bg_r, g=bg_g, b=bg_b} or nil,
        camera = {x=camera_x, y=camera_y},
        player_ptr = string.format("0x%08X", player_entity_ptr),
        p1_input_ptr = string.format("0x%08X", p1_input_ptr),
    }
end

-- Read controller input state (for replay)
local function read_input_state(input_ptr)
    if not is_valid_ptr(input_ptr) then return nil end

    -- Input structure offsets from UpdateInputState analysis:
    -- +0x00: u16 held_buttons (current frame)
    -- +0x01: u16 pressed_buttons (newly pressed this frame)
    -- +0x04: recording flag/pointer
    -- +0x10: u16 playback_index
    -- +0x12: u16 playback_timer

    local held = read_u16(input_ptr + 0x00)
    local pressed = read_u16(input_ptr + 0x02)

    return {
        held = held,
        pressed = pressed,
        -- PSX button mapping for reference in logs
        buttons = {
            select   = bit.band(held, 0x0001) ~= 0,
            l3       = bit.band(held, 0x0002) ~= 0,
            r3       = bit.band(held, 0x0004) ~= 0,
            start    = bit.band(held, 0x0008) ~= 0,
            up       = bit.band(held, 0x1000) ~= 0,
            right    = bit.band(held, 0x2000) ~= 0,
            down     = bit.band(held, 0x4000) ~= 0,
            left     = bit.band(held, 0x8000) ~= 0,
            l2       = bit.band(held, 0x0100) ~= 0,
            r2       = bit.band(held, 0x0200) ~= 0,
            l1       = bit.band(held, 0x0400) ~= 0,
            r1       = bit.band(held, 0x0800) ~= 0,
            triangle = bit.band(held, 0x0010) ~= 0,
            circle   = bit.band(held, 0x0020) ~= 0,
            cross    = bit.band(held, 0x0040) ~= 0,
            square   = bit.band(held, 0x0080) ~= 0,
        },
    }
end

-- Called when EntityTickLoop starts - dump full frame state for replay
local function on_entity_tick_loop(address, width, cause)
    if not CONFIG.watch_entity_tick then return end
    if state.frame_count % CONFIG.sample_every_n_frames ~= 0 then return end

    -- Core GameState (mode, camera, input)
    local game_state = read_game_state_tick()

    -- Input state (for deterministic replay)
    local p1_input_ptr = read_u32(ADDR.GameState + 0x50)
    local input = read_input_state(p1_input_ptr)

    -- Player entity
    local player = read_player_entity()

    -- All entities for full state capture
    local entities = {}
    if CONFIG.dump_all_entities then
        entities = iterate_entity_list()
    end

    -- Build complete frame state for replay
    local frame_data = {
        game_state = game_state,
        input = input,
        entity_count = #entities,
    }

    -- Player state (most important for replay)
    if player then
        frame_data.player = {
            ptr = player.ptr,
            x = player.x,
            y = player.y,
            x_whole = player.x_whole,
            y_whole = player.y_whole,
            x_frac = player.x_frac,
            y_frac = player.y_frac,
            vx = player.vx,
            vy = player.vy,
            vx_float = player.vx_float,
            vy_float = player.vy_float,
            facing = player.facing_dir,
            state = player.callback_name,
            callback_main = string.format("0x%08X", player.callback_main),
            sprite_id = player.sprite_id,
            anim_frame = player.anim_frame,
            anim_end = player.anim_end,
            anim_timer = player.anim_timer,
            flags = player.flags,
            layer = player.layer,
            z_order = player.z_order,
        }
    end

    -- All entities (compact form for full state)
    if CONFIG.dump_all_entities and #entities > 0 then
        local ent_list = {}
        for _, ent in ipairs(entities) do
            table.insert(ent_list, {
                ptr = ent.ptr,
                type = ent.entity_type,
                type_name = ent.type_name,
                x = ent.x,
                y = ent.y,
                vx = ent.vx,
                vy = ent.vy,
                facing = ent.facing,
                layer = ent.layer,
                z_order = ent.z_order,
                sprite_id = ent.sprite_id,
                anim_frame = ent.anim_frame,
                callback_main = string.format("0x%08X", ent.callback_main),
                flags = ent.flags,
            })
        end
        frame_data.entities = ent_list
    end

    log_entry("GameStateTick", frame_data)
end

-- Called when an entity's state/callback is changed
local function on_entity_set_state(address, width, cause)
    if not CONFIG.watch_animation then return end

    local regs = PCSX.getRegisters()
    local entity_ptr = regs.GPR.n.a0
    local state_data = regs.GPR.n.a1
    local callback_ptr = regs.GPR.n.a2

    -- Check if this is the player
    local player_ptr = get_player_ptr()
    local is_player = (entity_ptr == player_ptr)

    local cb_name = get_callback_name(callback_ptr)

    if is_player then
        state.stats.player_state_changes = state.stats.player_state_changes + 1
        log_entry("PlayerStateChange", {
            callback = cb_name,
            callback_addr = string.format("0x%08X", callback_ptr),
            state_data = string.format("0x%08X", state_data),
        })
        log_summary("Player state -> " .. cb_name)
    else
        -- Log other entity state changes too
        local ent = read_entity(entity_ptr)
        if ent and CONFIG.watch_entity_callbacks then
            log_entry("EntityStateChange", {
                entity = string.format("0x%08X", entity_ptr),
                entity_type = ent.entity_type,
                type_name = ent.type_name,
                callback = string.format("0x%08X", callback_ptr),
            })
        end
    end
end

-- Called when sprite ID is set
-- Throttle cache to prevent handler spam (entity_ptr → {frame, sprite_id})
local sprite_change_cache = {}
local SPRITE_THROTTLE_FRAMES = 5  -- Skip redundant changes within N frames

local function on_set_sprite_id(address, width, cause)
    if not CONFIG.watch_animation then return end

    local regs = PCSX.getRegisters()
    local entity_ptr = regs.GPR.n.a0
    local sprite_id = regs.GPR.n.a1
    local param = regs.GPR.n.a2

    local player_ptr = get_player_ptr()
    local is_player = (entity_ptr == player_ptr)

    if is_player then
        if sprite_id ~= state.last_player_sprite then
            state.stats.sprite_changes = state.stats.sprite_changes + 1
            log_entry("PlayerSpriteChange", {
                sprite_id = sprite_id,
                sprite_hex = string.format("0x%08X", sprite_id),
                param = param,
            })
            state.last_player_sprite = sprite_id
        end
    else
        -- Throttle non-player sprite changes to prevent overwhelming emulator
        -- (e.g., rapid pickups like clayballs trigger many sprite changes)
        if not CONFIG.watch_entity_callbacks then return end

        local cache_key = string.format("0x%08X_%04X", entity_ptr, sprite_id)
        local cached = sprite_change_cache[cache_key]
        local current_frame = state.frame_count

        -- Skip if same entity+sprite changed within throttle window
        if cached and (current_frame - cached.frame) < SPRITE_THROTTLE_FRAMES then
            return
        end

        -- Update cache before expensive read_entity call
        sprite_change_cache[cache_key] = {
            frame = current_frame,
            sprite_id = sprite_id,
        }

        -- Only read entity details if we're actually logging
        -- (reading entity does 15+ memory accesses, expensive!)
        local ent = read_entity(entity_ptr)
        if ent then
            log_entry("EntitySpriteChange", {
                entity = string.format("0x%08X", entity_ptr),
                entity_type = ent.entity_type,
                sprite_id = sprite_id,
            })
        end
    end
end

-- Called when animation is ticked
local function on_tick_animation(address, width, cause)
    if not CONFIG.watch_animation then return end

    local regs = PCSX.getRegisters()
    local entity_ptr = regs.GPR.n.a0

    local player_ptr = get_player_ptr()
    if entity_ptr == player_ptr then
        local player = read_player_entity()
        if player and player.anim_frame ~= state.last_anim_frame then
            log_entry("PlayerAnimTick", {
                frame = player.anim_frame,
                end_frame = player.anim_end,
                timer = player.anim_timer,
                speed = player.anim_speed,
            })
            state.last_anim_frame = player.anim_frame
        end
    end
end

-- Called when tile collision is checked
local function on_tile_attribute_check(address, width, cause)
    if not CONFIG.watch_collision then return end

    local regs = PCSX.getRegisters()
    -- Function signature: GetTileAttributeAtPosition(ctx, pixel_x, pixel_y)
    local _ctx = regs.GPR.n.a0  -- luacheck: ignore (part of documented signature)
    local pixel_x = regs.GPR.n.a1
    local pixel_y = regs.GPR.n.a2

    -- Only log occasionally to avoid massive spam
    if state.frame_count % 30 == 0 then
        log_entry("TileAttrCheck", {
            x = pixel_x,
            y = pixel_y,
            tile_x = math.floor(pixel_x / 16),
            tile_y = math.floor(pixel_y / 16),
        })
    end
end

-- Called when entity-entity collision is checked
local collision_check_cache = {}
local COLLISION_THROTTLE_FRAMES = 3  -- Skip redundant checks within N frames

local function on_entity_collision_check(address, width, cause)
    if not CONFIG.watch_collision then return end

    local regs = PCSX.getRegisters()
    local entity_a = regs.GPR.n.a0
    local entity_b = regs.GPR.n.a1

    local player_ptr = get_player_ptr()
    if entity_a == player_ptr or entity_b == player_ptr then
        local other_ptr = (entity_a == player_ptr) and entity_b or entity_a

        -- Throttle collision checks to prevent overwhelming emulator
        -- (rapid pickups like clayballs check collision many times)
        local cache_key = string.format("0x%08X", other_ptr)
        local cached = collision_check_cache[cache_key]
        local current_frame = state.frame_count

        if cached and (current_frame - cached.frame) < COLLISION_THROTTLE_FRAMES then
            return
        end

        collision_check_cache[cache_key] = {
            frame = current_frame,
        }

        local other = read_entity(other_ptr)
        if other then
            log_entry("PlayerCollisionCheck", {
                other_entity = string.format("0x%08X", other_ptr),
                other_type = other.entity_type,
                other_type_name = other.type_name,
                other_x = other.x,
                other_y = other.y,
            })
        end
    end
end

-- Called when level loading starts
local function on_level_load(address, width, cause)
    if not CONFIG.watch_level_load then return end

    local blb = get_blb_metadata()

    state.stats.level_loads = state.stats.level_loads + 1

    log_entry("LevelLoad", {
        blb_metadata = blb,
        timestamp = os.time(),
    })

    if blb.valid then
        log_summary(string.format("Loading %s (%s) Stage %d",
            blb.level.id, blb.level.name, blb.current.stage_index))
    else
        log_summary("Loading level (metadata invalid)")
    end

    -- Clear entity tracking on level load
    state.entity_cache = {}
    state.entity_spawn_frame = {}
end

-- Called when entities are spawned from definitions
local function on_spawn_entities(address, width, cause)
    if not CONFIG.watch_level_load then return end

    local blb = get_blb_metadata()

    log_entry("SpawnEntities", {
        level_id = blb.valid and blb.level.id or "UNKNOWN",
        stage_index = blb.valid and blb.current.stage_index or -1,
    })
    log_summary("Spawning entities from Asset 501")
end

-- Called when asset container is loaded
local function on_load_asset_container(address, width, cause)
    if not CONFIG.watch_blb_access then return end

    local regs = PCSX.getRegisters()
    local ctx_ptr = regs.GPR.n.a0
    local asset_index = regs.GPR.n.a1
    local load_mode = regs.GPR.n.a2

    local blb = get_blb_metadata()

    -- Decode load mode (1=secondary, 0=tertiary)
    local segment_type = (load_mode == 1) and "secondary" or "tertiary"

    log_entry("LoadAssetContainer", {
        level_id = blb.valid and blb.level.id or "UNKNOWN",
        stage_index = blb.valid and blb.current.stage_index or -1,
        asset_index = asset_index,
        segment_type = segment_type,
        load_mode = load_mode,
        ctx_ptr = string.format("0x%08X", ctx_ptr),
    })
end

-- Called when audio is uploaded to SPU
local function on_upload_audio(address, width, cause)
    if not CONFIG.watch_blb_access then return end

    local regs = PCSX.getRegisters()
    local sample_ptr = regs.GPR.n.a0
    local volume_ptr = regs.GPR.n.a1
    local size = regs.GPR.n.a2

    log_entry("UploadAudioToSPU", {
        sample_ptr = string.format("0x%08X", sample_ptr),
        volume_ptr = string.format("0x%08X", volume_ptr),
        size_bytes = size,
        size_kb = math.floor(size / 1024 * 10) / 10,
    })
end

-- Called when InitLevelDataContext completes (CRITICAL: BLB header is now loaded and safe to modify)
-- This is called from LoadBLBHeader @ 0x800208B0 AFTER the header is read from disc
-- Call chain: main → InitGameState → LoadBLBHeader → InitLevelDataContext → [HERE]
-- At this point: BLB header is in RAM, no videos have played, safe to patch level indices
local function on_header_loaded(address, width, cause)
    print(string.format("[BP-BOOT] on_header_loaded fired @ 0x%08X", address))

    -- Only apply boot override once
    if state.boot_override_applied then return end

    if CONFIG.boot_level_override then
        local level_idx = CONFIG.boot_level_override.level or 0
        local stage_idx = CONFIG.boot_level_override.stage or 0

        print(string.format("[BOOT] Applying level override: Level %d, Stage %d", level_idx, stage_idx))

        -- Set level/stage in BLB header (now safely loaded)
        write_u8(ADDR.BLB_game_mode, 3)  -- Set to level mode
        write_u8(ADDR.BLB_level_index, level_idx)
        write_u8(ADDR.BLB_stage_index, stage_idx)

        -- Also set in g_pPlayerState for consistency
        local player_state_ptr = read_u32(ADDR.g_pPlayerState)
        if is_valid_ptr(player_state_ptr) then
            write_u8(player_state_ptr, level_idx)
            write_u8(player_state_ptr + 1, stage_idx)
            print(string.format("[BOOT] Updated g_pPlayerState[0-1] = {%d, %d}", level_idx, stage_idx))
        end

        state.boot_override_applied = true

        log_entry("BootOverride", {
            level_index = level_idx,
            stage_index = stage_idx,
            timestamp = os.time(),
        })

        print("[BOOT] Override applied successfully. Game will load this level after intro.")
    end
end

-- Called at player tick callback
local function on_player_tick(address, width, cause)
    if not CONFIG.watch_player then return end

    local player = read_player_entity()
    if not player then return end

    -- Detect state changes
    if player.callback_main ~= state.last_player_state then
        log_entry("PlayerState", {
            callback = player.callback_name,
            x = player.x,
            y = player.y,
            vx = player.vx_float,
            vy = player.vy_float,
        })
        state.last_player_state = player.callback_main
    end

    -- Detect significant position changes
    local dx = math.abs((player.x or 0) - (state.last_player_x or 0))
    local dy = math.abs((player.y or 0) - (state.last_player_y or 0))

    if dx > 1 or dy > 1 then
        if not CONFIG.log_only_changes or dx > 0 or dy > 0 then
            log_entry("PlayerMove", {
                x = player.x,
                y = player.y,
                vx = player.vx_float,
                vy = player.vy_float,
                facing = player.facing_dir,
                input = player.input,
            })
        end
        state.last_player_x = player.x
        state.last_player_y = player.y
    end
end

-- =============================================================================
-- VSync Handler (per-frame tracking)
-- =============================================================================

local function on_vsync()
    state.frame_count = state.frame_count + 1

    -- Sample player velocity at high frequency for physics analysis
    if CONFIG.watch_player and CONFIG.sample_velocity_every_n_frames > 0 then
        if state.frame_count % CONFIG.sample_velocity_every_n_frames == 0 then
            local player = read_player_entity()
            if player then
                local should_log = true
                
                -- Optional: only log when velocity changes
                if CONFIG.log_velocity_changes_only then
                    if state.last_vx == player.vx and state.last_vy == player.vy then
                        should_log = false
                    end
                end
                
                if should_log then
                    log_entry("PlayerVelocity", {
                        vx = player.vx,
                        vy = player.vy,
                        vx_float = player.vx_float,
                        vy_float = player.vy_float,
                        x = player.x,
                        y = player.y,
                        state = player.callback_name,
                        facing = player.facing_dir,
                        on_ground = player.vy == 0 or player.vy == -65536,  -- Heuristic
                    })
                    state.last_vx = player.vx
                    state.last_vy = player.vy
                end
            end
        end
    end

    -- Clear throttle caches every 600 frames (~10 seconds) to prevent memory growth
    if state.frame_count % 600 == 0 then
        sprite_change_cache = {}
        collision_check_cache = {}
    end

    -- Auto-save periodically if enabled
    if CONFIG.auto_save_interval and CONFIG.auto_save_interval > 0 then
        if state.frame_count - state.last_auto_save >= CONFIG.auto_save_interval then
            if #state.log_entries > 0 then
                print(string.format("\n[AUTO-SAVE] Saving %d entries (frame %d)...", #state.log_entries, state.frame_count))
                dump_log()
                state.last_auto_save = state.frame_count
            end
        end
    end

    -- Auto-stop when log is full
    if #state.log_entries >= CONFIG.max_log_entries then
        if state.log_entries[#state.log_entries].type ~= "LogFull" then
            print(string.format("Log full at %d entries, auto-saving...", #state.log_entries))
            dump_log()
        end
    end
end

-- =============================================================================
-- Setup and Teardown
-- =============================================================================

local function setup_breakpoints()
    print("Setting up comprehensive Skullmonkeys watchers...")

    -- Set up boot-time level override guard
    -- This breakpoint fires AFTER InitLevelDataContext completes, when BLB header is safely loaded
    -- Call chain: main → InitGameState @ 0x8007CD34 → LoadBLBHeader @ 0x800208B0 →
    --             InitLevelDataContext @ 0x8007A1BC → [return address]
    -- Return address is 0x80020950 (inside LoadBLBHeader, right after jal InitLevelDataContext)
    -- Disassembly: 0x80020948: jal 0x8007a1bc
    --              0x8002094C: move a0,s1 (delay slot)
    --              0x80020950: clear a0  ← BREAKPOINT HERE
    if CONFIG.boot_level_override then
        print(string.format("  [!] Boot override configured: Level %d, Stage %d",
            CONFIG.boot_level_override.level or 0,
            CONFIG.boot_level_override.stage or 0))
        print("  [!] Setting breakpoint at InitLevelDataContext return (0x80020950)")

        -- Breakpoint at return address in LoadBLBHeader (0x80020950 = offset +0xA0 from function start)
        -- This is right after: jal InitLevelDataContext (0x80020948) + delay slot (0x8002094C)
        state.breakpoints.header_loaded = PCSX.addBreakpoint(
            0x80020950, 'Exec', 4,
            'BLBHeaderLoaded', safe_breakpoint_handler('BLBHeaderLoaded', on_header_loaded)
        )
    end

    -- EntityTickLoop - main game tick (full state dump)
    if CONFIG.watch_frame_state then
        state.breakpoints.tick_loop = PCSX.addBreakpoint(
            ADDR.FN_EntityTickLoop, 'Exec', 4,
            'EntityTickLoop', safe_breakpoint_handler('EntityTickLoop', on_entity_tick_loop)
        )
        print("  [x] Frame state capture (EntityTickLoop)")
    end

    -- Player tick callback
    if CONFIG.watch_player then
        state.breakpoints.player_tick = PCSX.addBreakpoint(
            ADDR.FN_PlayerTickCallback, 'Exec', 4,
            'PlayerTickCallback', safe_breakpoint_handler('PlayerTickCallback', on_player_tick)
        )
        print("  [x] Player movement tracking")
    end

    -- Animation/state changes
    if CONFIG.watch_animation then
        state.breakpoints.set_state = PCSX.addBreakpoint(
            ADDR.FN_EntitySetState, 'Exec', 4,
            'EntitySetState', safe_breakpoint_handler('EntitySetState', on_entity_set_state)
        )
        state.breakpoints.set_sprite = PCSX.addBreakpoint(
            ADDR.FN_SetEntitySpriteId, 'Exec', 4,
            'SetEntitySpriteId', safe_breakpoint_handler('SetEntitySpriteId', on_set_sprite_id)
        )
        state.breakpoints.tick_anim = PCSX.addBreakpoint(
            ADDR.FN_TickEntityAnimation, 'Exec', 4,
            'TickEntityAnimation', safe_breakpoint_handler('TickEntityAnimation', on_tick_animation)
        )
        print("  [x] Animation/state change tracking")
    end

    -- Collision
    if CONFIG.watch_collision then
        state.breakpoints.tile_attr = PCSX.addBreakpoint(
            ADDR.FN_GetTileAttributeAtPosition, 'Exec', 4,
            'GetTileAttributeAtPosition', safe_breakpoint_handler('TileAttrCheck', on_tile_attribute_check)
        )
        state.breakpoints.collision = PCSX.addBreakpoint(
            ADDR.FN_CheckEntityCollision, 'Exec', 4,
            'CheckEntityCollision', safe_breakpoint_handler('EntityCollision', on_entity_collision_check)
        )
        print("  [x] Collision tracking")
    end

    -- Level loading
    if CONFIG.watch_level_load then
        state.breakpoints.level_load = PCSX.addBreakpoint(
            ADDR.FN_InitializeAndLoadLevel, 'Exec', 4,
            'InitializeAndLoadLevel', safe_breakpoint_handler('LevelLoad', on_level_load)
        )
        state.breakpoints.spawn = PCSX.addBreakpoint(
            ADDR.FN_SpawnPlayerAndEntities, 'Exec', 4,
            'SpawnPlayerAndEntities', safe_breakpoint_handler('SpawnEntities', on_spawn_entities)
        )
        print("  [x] Level load tracking")
    end

    -- BLB asset access
    if CONFIG.watch_blb_access then
        state.breakpoints.load_asset = PCSX.addBreakpoint(
            ADDR.FN_LoadAssetContainer, 'Exec', 4,
            'LoadAssetContainer', safe_breakpoint_handler('LoadAsset', on_load_asset_container)
        )
        state.breakpoints.upload_audio = PCSX.addBreakpoint(
            ADDR.FN_UploadAudioToSPU, 'Exec', 4,
            'UploadAudioToSPU', safe_breakpoint_handler('UploadAudio', on_upload_audio)
        )
        print("  [x] BLB asset/audio tracking")
    end

    -- VSync listener for frame counting
    state.listeners.vsync = PCSX.Events.createEventListener('GPU::Vsync', on_vsync)

    print("")
    print("Watchers active! Use these commands:")
    print("  status()     - Show current game state")
    print("  stats()      - Show capture statistics")
    print("  test_logging() - Test log system (saves 5 test entries)")
    print("  snapshot()   - Get full state as table")
    print("  dump_log()   - Save log with auto-generated filename")
    print("  clear_log()  - Clear captured log")
    print("  mark('name') - Add parsing marker at current frame")
    print("  markers()    - List all markers")
    print("  entities()   - List all active entities")
    print("  cleanup()    - Remove all watchers (auto-saves)")
    print("")
    print("Level commands:")
    print("  levels()           - List all 26 levels")
    print("  level_info(idx)    - Show level details")
    print("  load_level(idx,stg) - Set level to load")
    print("  load_level_by_id('SCIE',0) - Load by ID")
    print("  set_boot_override(level, stage) - Override boot level")
    print("")
    print("Logs will auto-save to: " .. CONFIG.log_dir .. "/")
end

local function cleanup()
    print("Removing watchers...")

    -- Auto-save logs if enabled and there are entries
    if CONFIG.auto_save_on_exit and #state.log_entries > 0 then
        print(string.format("Auto-saving %d log entries...", #state.log_entries))
        dump_log()  -- Use auto-generated filename
    end

    for name, bp in pairs(state.breakpoints) do
        if bp then
            local success, err = pcall(function() bp:remove() end)
            if not success then
                print(string.format("Warning: Failed to remove breakpoint %s: %s", name, tostring(err)))
            end
        end
    end
    for name, listener in pairs(state.listeners) do
        if listener then
            local success, err = pcall(function() listener:remove() end)
            if not success then
                print(string.format("Warning: Failed to remove listener %s: %s", name, tostring(err)))
            end
        end
    end
    state.breakpoints = {}
    state.listeners = {}
    print("Watchers removed.")
end

-- =============================================================================
-- Commands
-- =============================================================================

function dump_log(filename)
    -- Generate timestamped filename if not provided
    if not filename then
        -- Create directory if it doesn't exist
        os.execute("mkdir -p " .. CONFIG.log_dir)

        -- Build filename with session info
        local blb = get_blb_metadata()
        local level_str = "unknown"
        if blb.valid then
            level_str = string.format("%s_stage%d", blb.level.id, blb.current.stage_index)
        end

        filename = string.format("%s/trace_%s_%s_f%d.jsonl",
            CONFIG.log_dir,
            state.session_id,
            level_str,
            state.frame_count)
    end

    local file = io.open(filename, "w")
    if not file then
        print("Failed to open " .. filename)
        return false
    end

    for _, entry in ipairs(state.log_entries) do
        file:write(to_json(entry) .. "\n")
    end

    file:close()
    print(string.format("Wrote %d entries to %s", #state.log_entries, filename))
    print(string.format("Stats: %d state changes, %d sprite changes, %d level loads",
        state.stats.player_state_changes,
        state.stats.sprite_changes,
        state.stats.level_loads))
    return true
end

function clear_log()
    state.log_entries = {}
    state.frame_count = 0
    state.last_player_state = nil
    state.last_player_x = nil
    state.last_player_y = nil
    state.last_player_sprite = nil
    state.last_anim_frame = nil
    state.entity_cache = {}
    state.markers = {}
    state.stats = {
        total_entities_seen = 0,
        player_state_changes = 0,
        sprite_changes = 0,
        level_loads = 0,
    }
    print("Log cleared.")
end

-- Add a marker for easy log parsing
function mark(label)
    label = label or "marker"
    local marker_data = {
        label = label,
        frame = state.frame_count,
        timestamp = os.time(),
        log_index = #state.log_entries,
    }
    table.insert(state.markers, marker_data)
    log_entry("Marker", marker_data)
    print(string.format("[MARKER] '%s' at frame %d (entry %d)", label, state.frame_count, #state.log_entries))
end

-- List all markers
function markers()
    if #state.markers == 0 then
        print("No markers set.")
        return
    end
    print("=== Markers ===")
    for i, m in ipairs(state.markers) do
        print(string.format("%2d. Frame %6d (entry %6d): %s", i, m.frame, m.log_index, m.label))
    end
end

function status()
    local player = read_player_entity()
    local camera = get_camera()
    local entities = iterate_entity_list()

    print("=== Skullmonkeys Game Watcher v2.0 ===")
    print(string.format("Frame: %d", state.frame_count))
    print(string.format("Log entries: %d / %d", #state.log_entries, CONFIG.max_log_entries))
    print(string.format("Camera: (%d, %d)", camera.x, camera.y))
    print(string.format("Active entities: %d", #entities))
    print("")

    if player then
        print("=== Player ===")
        print(string.format("  Pointer: %s", player.ptr))
        print(string.format("  Position: (%d, %d)", player.x, player.y))
        print(string.format("  Velocity: (%.2f, %.2f)", player.vx_float, player.vy_float))
        print(string.format("  Facing: %s", player.facing_dir))
        print(string.format("  State: %s", player.callback_name))
        print(string.format("  Animation: %d / %d", player.anim_frame, player.anim_end))
        print(string.format("  Sprite ID: 0x%08X", player.sprite_id))
        print(string.format("  Health: %d", player.health))
        print(string.format("  Input: 0x%04X", player.input))
    else
        print("Player: not loaded")
    end
end

function entities()
    local ent_list = iterate_entity_list()

    print(string.format("=== Active Entities (%d) ===", #ent_list))
    for i, ent in ipairs(ent_list) do
        print(string.format("%2d. %s Type=%d (%s) Pos=(%d,%d) Layer=%d Sprite=0x%X",
            i, ent.ptr, ent.entity_type, ent.type_name,
            ent.x, ent.y, ent.layer, ent.sprite_id))
    end
end

function stats()
    print("=== Capture Statistics ===")
    print(string.format("Frames captured: %d", state.frame_count))
    print(string.format("Log entries: %d / %d (%.1f%%)",
        #state.log_entries, CONFIG.max_log_entries,
        100 * #state.log_entries / CONFIG.max_log_entries))
    print(string.format("Player state changes: %d", state.stats.player_state_changes))
    print(string.format("Sprite changes: %d", state.stats.sprite_changes))
    print(string.format("Level loads: %d", state.stats.level_loads))
    print(string.format("Markers: %d", #state.markers))
    print(string.format("Session ID: %s", state.session_id))
    print(string.format("Log directory: %s", CONFIG.log_dir))
end

-- Test logging to verify it works
function test_logging()
    print("=== Testing Logging System ===")
    print("Adding test entries...")

    for i = 1, 5 do
        log_entry("TestEvent", {
            test_number = i,
            message = string.format("Test entry %d", i),
            timestamp = os.time(),
        })
    end

    print(string.format("Added 5 test entries. Total: %d", #state.log_entries))
    print("")
    print("Attempting to save...")
    local success = dump_log()

    if success then
        print("✓ Logging system is working!")
    else
        print("✗ Logging failed - check permissions for: " .. CONFIG.log_dir)
    end
end

function snapshot()
    local player = read_player_entity()
    local camera = get_camera()
    local ent_list = iterate_entity_list()

    local ent_compact = {}
    for _, ent in ipairs(ent_list) do
        table.insert(ent_compact, {
            ptr = ent.ptr,
            type = ent.entity_type,
            type_name = ent.type_name,
            x = ent.x,
            y = ent.y,
            sprite_id = ent.sprite_id,
            anim_frame = ent.anim_frame,
        })
    end

    return {
        frame = state.frame_count,
        camera = camera,
        player = player and {
            x = player.x,
            y = player.y,
            vx = player.vx_float,
            vy = player.vy_float,
            facing = player.facing_dir,
            state = player.callback_name,
            anim_frame = player.anim_frame,
            anim_end = player.anim_end,
            sprite_id = player.sprite_id,
            input = player.input,
            health = player.health,
        } or nil,
        entities = ent_compact,
    }
end

-- =============================================================================
-- Level Management Functions
-- =============================================================================

-- List all levels in the BLB
function levels()
    local level_count = read_u8(ADDR.BLB_level_count)
    local current = get_current_level()

    print(string.format("=== BLB Levels (%d total) ===", level_count))
    print(string.format("Current: Level %d, Stage %d, Mode %d",
        current.level_index, current.stage_index, current.game_mode))
    print("")
    print("Idx  ID    Stages  Name")
    print("---  ----  ------  --------------------")

    for i = 0, level_count - 1 do
        local info = get_level_info(i)
        local marker = (i == current.level_index) and "*" or " "
        print(string.format("%s%2d  %-4s  %d       %s",
            marker, i, info.id, info.stage_count, info.name))
    end
    print("")
    print("Use load_level(index, stage) to load a level")
end

-- Load a specific level and stage
-- This works by setting the level/stage index and triggering a level load
function load_level(level_idx, stage_idx)
    stage_idx = stage_idx or 0
    local level_count = read_u8(ADDR.BLB_level_count)

    if level_idx < 0 or level_idx >= level_count then
        print(string.format("Error: Level index must be 0-%d", level_count - 1))
        return false
    end

    local info = get_level_info(level_idx)
    if stage_idx < 0 or stage_idx >= info.stage_count then
        print(string.format("Error: Stage index must be 0-%d for %s",
            info.stage_count - 1, info.id))
        return false
    end

    print(string.format("Loading %s (%s) Stage %d...", info.id, info.name, stage_idx))

    -- Set game mode to level mode (3)
    write_u8(ADDR.BLB_game_mode, 3)

    -- Set level and stage index
    write_u8(ADDR.BLB_level_index, level_idx)
    write_u8(ADDR.BLB_stage_index, stage_idx)

    -- Note: The actual level load happens when the game loop processes the mode
    -- You may need to trigger a mode transition or reset
    print("Level index set. The game should load this level on next mode transition.")
    print("Tip: Use reset() or wait for a death/exit to trigger the load.")

    return true
end

-- Convenience function to load level by ID (e.g., "SCIE", "PHRO")
function load_level_by_id(level_id, stage_idx)
    stage_idx = stage_idx or 0
    local level_count = read_u8(ADDR.BLB_level_count)

    level_id = string.upper(level_id)

    for i = 0, level_count - 1 do
        local info = get_level_info(i)
        if info.id == level_id then
            return load_level(i, stage_idx)
        end
    end

    print(string.format("Error: Level '%s' not found", level_id))
    print("Use levels() to see available levels")
    return false
end

-- Show detailed info for a specific level
function level_info(level_idx)
    local level_count = read_u8(ADDR.BLB_level_count)

    if level_idx < 0 or level_idx >= level_count then
        print(string.format("Error: Level index must be 0-%d", level_count - 1))
        return
    end

    local info = get_level_info(level_idx)

    print(string.format("=== Level %d: %s ===", level_idx, info.id))
    print(string.format("  Name: %s", info.name))
    print(string.format("  Stages: %d", info.stage_count))
    print(string.format("  Flags: 0x%02X", info.flags))
    print(string.format("  Primary buffer size: %d bytes (%.1f KB)",
        info.primary_size, info.primary_size / 1024))
    print(string.format("  Primary sector: %d (0x%X in file)",
        info.primary_sector, info.primary_sector * 2048))
    print(string.format("  Entry[1] offset: 0x%X", info.entry1_offset))

    if #info.stage_sectors > 0 then
        print(string.format("  Stage sectors:"))
        for i, sector in ipairs(info.stage_sectors) do
            print(string.format("    Stage %d: Sector %d (%d sectors, 0x%X in file)",
                i - 1, sector, info.stage_sector_counts[i], sector * 2048))
        end
    end
end

-- Set boot-time level override (call before game reset)
function set_boot_override(level_idx, stage_idx)
    stage_idx = stage_idx or 0
    local level_count = read_u8(ADDR.BLB_level_count)

    if level_idx < 0 or level_idx >= level_count then
        print(string.format("Error: Level index must be 0-%d", level_count - 1))
        return false
    end

    local info = get_level_info(level_idx)
    if stage_idx < 0 or stage_idx >= info.stage_count then
        print(string.format("Error: Stage index must be 0-%d for %s",
            info.stage_count - 1, info.id))
        return false
    end

    CONFIG.boot_level_override = {level = level_idx, stage = stage_idx}
    print(string.format("Boot override set: %s (%s) Stage %d",
        info.id, info.name, stage_idx))
    print("Call reload_watchers() to apply the override")
    return true
end

-- Reload watchers with current config (apply boot override)
function reload_watchers()
    cleanup()
    setup_breakpoints()
end

-- Export trace as structured JSON with metadata
function export_trace(filename)
    filename = filename or (CONFIG.log_file_path .. ".export.json")

    local blb = get_blb_metadata()
    local current_level = get_current_level()

    local export_data = {
        metadata = {
            version = "2.0",
            game = "Skullmonkeys",
            region = "PAL",
            game_version = "SLES-01090",
            export_time = os.time(),
            frame_count = state.frame_count,
            log_entries = #state.log_entries,
        },
        statistics = state.stats,
        blb_info = blb,
        current_state = {
            level = current_level,
            player = get_player_state_compact(),
            camera = get_camera(),
        },
        config = {
            sample_rate = CONFIG.sample_every_n_frames,
            dump_all_entities = CONFIG.dump_all_entities,
            dump_blb_metadata = CONFIG.dump_blb_metadata,
        },
        entries = state.log_entries,
    }

    local file = io.open(filename, "w")
    if not file then
        print("Failed to open " .. filename)
        return false
    end

    -- Write structured JSON
    file:write(to_json(export_data))
    file:close()

    print(string.format("Exported %d entries to %s (%.1f KB)",
        #state.log_entries, filename,
        io.popen("stat -c%s " .. filename):read("*n") / 1024))
    return true
end

-- =============================================================================
-- Initialization
-- =============================================================================

print("╔══════════════════════════════════════════════╗")
print("║  Skullmonkeys Game Watcher v2.5              ║")
print("║  PAL version (SLES-01090)                    ║")
print("║  Comprehensive runtime debugging             ║")
print("║                                              ║")
print("║  Features:                                   ║")
print("║  • Full frame-by-frame entity tracking       ║")
print("║  • BLB asset/level metadata logging          ║")
print("║  • Boot-time level selection                 ║")
print("║  • Structured JSONL trace export             ║")
print("╚══════════════════════════════════════════════╝")
print("")

-- Show current BLB state
local current = get_current_level()
print(string.format("BLB Header: %d levels, %d movies, %d sector entries",
    current.level_count, current.movie_count, current.sector_count))

if current.level_index < current.level_count then
    local info = get_level_info(current.level_index)
    print(string.format("Current Level: %s (%s) Stage %d/%d",
        info.id, info.name, current.stage_index, info.stage_count - 1))
else
    print(string.format("Current Level: Index %d (not loaded)", current.level_index))
end

print("")

-- Open log file for streaming if enabled
if CONFIG.stream_to_file then
    local blb_info = get_current_level()
    local level_id = "unknown"
    if blb_info.level_index < blb_info.level_count then
        local info = get_level_info(blb_info.level_index)
        level_id = info.id or "unknown"
    end

    local filename = string.format("trace_%s_%s_stage%d_f%d.jsonl",
        os.date("%Y%m%d_%H%M%S"),
        level_id,
        blb_info.stage_index,
        state.frame_count)

    state.log_file_path = CONFIG.log_dir .. "/" .. filename
    state.log_file = io.open(state.log_file_path, "w")

    if state.log_file then
        print(string.format("[STREAM] Logging to: %s", state.log_file_path))
    else
        print(string.format("[ERROR] Failed to open log file: %s", state.log_file_path))
        CONFIG.stream_to_file = false
    end
end

setup_breakpoints()

-- Log initial startup for testing
log_entry("WatcherStarted", {
    session_id = state.session_id,
    timestamp = os.time(),
    log_file = state.log_file_path,
    config = {
        frame_state = CONFIG.watch_frame_state,
        entity_tick = CONFIG.watch_entity_tick,
        player = CONFIG.watch_player,
        animation = CONFIG.watch_animation,
        level_load = CONFIG.watch_level_load,
        blb_access = CONFIG.watch_blb_access,
        stream_to_file = CONFIG.stream_to_file,
    }
})

print(string.format("[INIT] Watcher started with session ID: %s", state.session_id))
print(string.format("[INIT] %d log entries captured so far", #state.log_entries))

-- Register exit handler to save logs when emulator quits
state.listeners.quitting = PCSX.Events.createEventListener('Quitting', function()
    print(string.format("\n[EXIT] Emulator closing..."))

    -- Close streaming log file if open
    if state.log_file then
        state.log_file:flush()
        state.log_file:close()
        print(string.format("[EXIT] Stream closed: %s", state.log_file_path))
        print(string.format("[EXIT] %d frames logged", state.frame_count))
    end

    -- Save buffered entries if streaming was disabled
    if not CONFIG.stream_to_file and #state.log_entries > 0 then
        print(string.format("[EXIT] Saving %d buffered log entries...", #state.log_entries))
        local success = dump_log()
        if success then
            print("[EXIT] Buffered logs saved successfully")
        end
    end
end)

print("\n[TIP] Logs stream to file continuously (flushed every 60 frames)")
print("[TIP] Use mark('label') to add parsing markers for easy log analysis")
print("[TIP] Use test_logging() to verify logging system works")
