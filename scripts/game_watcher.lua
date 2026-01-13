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

local ffi = require('ffi')

-- =============================================================================
-- Configuration
-- =============================================================================

local CONFIG = {
    -- Master enable/disable for watcher categories
    watch_frame_state = true,     -- Full state dump each frame
    watch_entity_tick = true,     -- EntityTickLoop breakpoint
    watch_entity_callbacks = true, -- Individual entity callbacks
    watch_player = true,          -- Player-specific state changes
    watch_animation = true,       -- Sprite/animation changes
    watch_collision = true,       -- Tile and entity collision
    watch_level_load = true,      -- Level loading events
    
    -- Frame state dump options
    dump_all_entities = true,     -- Include ALL active entities each frame
    dump_player_only = false,     -- Only dump player state (faster)
    max_entities_per_frame = 64,  -- Cap entity dumps per frame
    
    -- Logging
    log_to_console = false,       -- Print to Lua console (verbose!)
    console_summary_only = true,  -- Only print summaries to console
    log_file_path = "/tmp/skullmonkeys_trace.jsonl",
    
    -- Sampling
    sample_every_n_frames = 1,    -- Full dump every N frames (1 = every frame)
    max_log_entries = 50000,      -- Max entries before auto-stop
    
    -- Change detection (reduce noise)
    log_only_changes = false,     -- Only log when state changes
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
    FN_LoadLevelFromBLB = 0x8007E474,       -- Load level
    FN_SpawnPlayerAndEntities = 0x8007DF38, -- Spawn entities from defs
    FN_RemapEntityTypes = 0x8008150C,       -- Convert BLB→internal types
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

-- Convert PSX address to memory offset (strip high bits for kuseg/kseg0/kseg1)
local function psx_to_offset(addr)
    return bit.band(addr, 0x1FFFFF)
end

local function read_u8(addr)
    return mem[psx_to_offset(addr)]
end

local function read_u16(addr)
    local off = psx_to_offset(addr)
    return mem[off] + mem[off + 1] * 256
end

local function read_s16(addr)
    local v = read_u16(addr)
    if v >= 0x8000 then v = v - 0x10000 end
    return v
end

local function read_u32(addr)
    local off = psx_to_offset(addr)
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

-- Check if pointer is valid PSX RAM address
local function is_valid_ptr(addr)
    return addr >= 0x80000000 and addr < 0x80200000
end

-- =============================================================================
-- State Tracking
-- =============================================================================

local state = {
    frame_count = 0,
    log_entries = {},
    breakpoints = {},
    listeners = {},
    
    -- Last known state for change detection
    last_player_state = nil,
    last_player_x = nil,
    last_player_y = nil,
    last_player_sprite = nil,
    last_anim_frame = nil,
    last_entity_count = 0,
    
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
    
    table.insert(state.log_entries, entry)
    
    if CONFIG.log_to_console and not CONFIG.console_summary_only then
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
-- Breakpoint Handlers
-- =============================================================================

-- Called when EntityTickLoop starts - dump full frame state
local function on_entity_tick_loop(address, width, cause)
    if not CONFIG.watch_entity_tick then return end
    if state.frame_count % CONFIG.sample_every_n_frames ~= 0 then return end
    
    local player = read_player_entity()
    local camera = get_camera()
    local entities = {}
    
    if CONFIG.dump_all_entities then
        entities = iterate_entity_list()
    end
    
    -- Build frame state
    local frame_data = {
        camera = camera,
        entity_count = #entities,
    }
    
    -- Player state
    if player then
        frame_data.player = {
            ptr = player.ptr,
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
            health = player.health,
            invuln = player.invuln_timer,
        }
    end
    
    -- All entities (compact form)
    if CONFIG.dump_all_entities and #entities > 0 then
        local ent_list = {}
        for _, ent in ipairs(entities) do
            table.insert(ent_list, {
                ptr = ent.ptr,
                type = ent.entity_type,
                type_name = ent.type_name,
                x = ent.x,
                y = ent.y,
                layer = ent.layer,
                sprite_id = ent.sprite_id,
                anim_frame = ent.anim_frame,
                callback = string.format("0x%08X", ent.callback_main),
            })
        end
        frame_data.entities = ent_list
    end
    
    log_entry("FrameState", frame_data)
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
        -- Track other entity sprite changes
        local ent = read_entity(entity_ptr)
        if ent and CONFIG.watch_entity_callbacks then
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
    local ctx = regs.GPR.n.a0
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
local function on_entity_collision_check(address, width, cause)
    if not CONFIG.watch_collision then return end
    
    local regs = PCSX.getRegisters()
    local entity_a = regs.GPR.n.a0
    local entity_b = regs.GPR.n.a1
    
    local player_ptr = get_player_ptr()
    if entity_a == player_ptr or entity_b == player_ptr then
        local other_ptr = (entity_a == player_ptr) and entity_b or entity_a
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
    
    local regs = PCSX.getRegisters()
    local level_idx = regs.GPR.n.a0
    local stage_idx = regs.GPR.n.a1
    
    state.stats.level_loads = state.stats.level_loads + 1
    
    log_entry("LevelLoad", {
        level = level_idx,
        stage = stage_idx,
    })
    log_summary(string.format("Loading level %d stage %d", level_idx, stage_idx))
    
    -- Clear entity tracking on level load
    state.entity_cache = {}
    state.entity_spawn_frame = {}
end

-- Called when entities are spawned from definitions
local function on_spawn_entities(address, width, cause)
    if not CONFIG.watch_level_load then return end
    
    log_entry("SpawnEntities", {})
    log_summary("Spawning entities from definitions")
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
    
    -- EntityTickLoop - main game tick (full state dump)
    if CONFIG.watch_frame_state then
        state.breakpoints.tick_loop = PCSX.addBreakpoint(
            ADDR.FN_EntityTickLoop, 'Exec', 4,
            'EntityTickLoop', on_entity_tick_loop
        )
        print("  [x] Frame state capture (EntityTickLoop)")
    end
    
    -- Player tick callback
    if CONFIG.watch_player then
        state.breakpoints.player_tick = PCSX.addBreakpoint(
            ADDR.FN_PlayerTickCallback, 'Exec', 4, 
            'PlayerTickCallback', on_player_tick
        )
        print("  [x] Player movement tracking")
    end
    
    -- Animation/state changes
    if CONFIG.watch_animation then
        state.breakpoints.set_state = PCSX.addBreakpoint(
            ADDR.FN_EntitySetState, 'Exec', 4,
            'EntitySetState', on_entity_set_state
        )
        state.breakpoints.set_sprite = PCSX.addBreakpoint(
            ADDR.FN_SetEntitySpriteId, 'Exec', 4,
            'SetEntitySpriteId', on_set_sprite_id
        )
        state.breakpoints.tick_anim = PCSX.addBreakpoint(
            ADDR.FN_TickEntityAnimation, 'Exec', 4,
            'TickEntityAnimation', on_tick_animation
        )
        print("  [x] Animation/state change tracking")
    end
    
    -- Collision
    if CONFIG.watch_collision then
        state.breakpoints.tile_attr = PCSX.addBreakpoint(
            ADDR.FN_GetTileAttributeAtPosition, 'Exec', 4,
            'GetTileAttributeAtPosition', on_tile_attribute_check
        )
        state.breakpoints.collision = PCSX.addBreakpoint(
            ADDR.FN_CheckEntityCollision, 'Exec', 4,
            'CheckEntityCollision', on_entity_collision_check
        )
        print("  [x] Collision tracking")
    end
    
    -- Level loading
    if CONFIG.watch_level_load then
        state.breakpoints.level_load = PCSX.addBreakpoint(
            ADDR.FN_LoadLevelFromBLB, 'Exec', 4,
            'LoadLevelFromBLB', on_level_load
        )
        state.breakpoints.spawn = PCSX.addBreakpoint(
            ADDR.FN_SpawnPlayerAndEntities, 'Exec', 4,
            'SpawnPlayerAndEntities', on_spawn_entities
        )
        print("  [x] Level load tracking")
    end
    
    -- VSync listener for frame counting
    state.listeners.vsync = PCSX.Events.createEventListener('GPU::Vsync', on_vsync)
    
    print("")
    print("Watchers active! Use these commands:")
    print("  status()     - Show current game state")
    print("  snapshot()   - Get full state as table")
    print("  dump_log()   - Save log to " .. CONFIG.log_file_path)
    print("  clear_log()  - Clear captured log")
    print("  entities()   - List all active entities")
    print("  stats()      - Show capture statistics")
    print("  cleanup()    - Remove all watchers")
end

local function cleanup()
    print("Removing watchers...")
    for name, bp in pairs(state.breakpoints) do
        if bp then bp:remove() end
    end
    for name, listener in pairs(state.listeners) do
        if listener then listener:remove() end
    end
    state.breakpoints = {}
    state.listeners = {}
    print("Watchers removed.")
end

-- =============================================================================
-- Commands
-- =============================================================================

function dump_log()
    local filename = CONFIG.log_file_path
    local file = io.open(filename, "w")
    if not file then
        print("Failed to open " .. filename)
        return
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
    state.stats = {
        total_entities_seen = 0,
        player_state_changes = 0,
        sprite_changes = 0,
        level_loads = 0,
    }
    print("Log cleared.")
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
-- Initialization
-- =============================================================================

print("╔══════════════════════════════════════════════╗")
print("║  Skullmonkeys Game Watcher v2.0              ║")
print("║  PAL version (SLES-01090)                    ║")
print("║  Comprehensive entity & state tracking       ║")
print("╚══════════════════════════════════════════════╝")
print("")

setup_breakpoints()
