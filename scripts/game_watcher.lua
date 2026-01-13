--[[
  Skullmonkeys (SLES-01090 PAL) Game Watcher
  
  This script sets up watches on key game functions to log behavior
  for replication in evil-engine.
  
  Load in PCSX-Redux via: Debug > Lua Console or Editor
  Or use: pcsx-redux -dofile scripts/game_watcher.lua
  
  Output is written to the Lua console and optionally to a JSON file.
]]

local ffi = require('ffi')

-- =============================================================================
-- Configuration
-- =============================================================================

local CONFIG = {
    -- Enable/disable specific watchers
    watch_player = true,
    watch_entities = true,
    watch_animation = true,
    watch_collision = true,
    watch_level_load = true,
    
    -- Logging
    log_to_console = true,
    log_to_file = false,
    log_file_path = "/tmp/skullmonkeys_trace.jsonl",
    
    -- Sampling
    sample_every_n_frames = 1,  -- Log every N vsync frames
    max_log_entries = 10000,
}

-- =============================================================================
-- Memory Addresses (PAL / SLES-01090)
-- =============================================================================

local ADDR = {
    -- Core structures
    GameState = 0x8009DC40,
    LevelDataContext = 0x8009DCC4,
    BLBHeader = 0x800AE3E0,
    
    -- GameState offsets
    GS_entity_tick_list = 0x1C,
    GS_entity_render_list = 0x20,
    GS_player = 0x30,
    GS_camera_x = 0x38,
    GS_camera_y = 0x3C,
    
    -- Player entity offsets
    PLAYER_callback = 0x04,
    PLAYER_x = 0x48,
    PLAYER_y = 0x4A,
    PLAYER_vx = 0x4C,
    PLAYER_vy = 0x4E,
    PLAYER_facing = 0x74,
    PLAYER_sprite_id = 0xBC,
    PLAYER_anim_frame = 0xDA,
    PLAYER_anim_end = 0xDE,
    PLAYER_state_offset = 0xA0,
    
    -- Key functions to watch
    FN_EntityTickLoop = 0x80020E1C,
    FN_EntitySetState = 0x8001EAAC,
    FN_SetEntitySpriteId = 0x8001D080,
    FN_UpdateSpriteFrameData = 0x8001D748,
    FN_TickEntityAnimation = 0x8001D290,
    FN_CreatePlayerEntity = 0x800596A4,
    FN_PlayerTickCallback = 0x80059E10,
    FN_CheckEntityCollision = 0x800226F8,
    FN_GetTileAttributeAtPosition = 0x8007B63C,
    FN_LoadAssetContainer = 0x8007B074,
    FN_LoadLevelFromBLB = 0x8007E474,
    FN_SpawnPlayerAndEntities = 0x8007DF38,
}

-- Player callback functions (state machine)
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

-- =============================================================================
-- Memory Access Helpers
-- =============================================================================

local mem = PCSX.getMemPtr()

-- Convert PSX address to memory offset (strip high bits)
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
    last_anim_frame = nil,
}

-- =============================================================================
-- Logging
-- =============================================================================

local function log_entry(entry_type, data)
    if #state.log_entries >= CONFIG.max_log_entries then
        return
    end
    
    local entry = {
        frame = state.frame_count,
        type = entry_type,
        data = data,
    }
    
    table.insert(state.log_entries, entry)
    
    if CONFIG.log_to_console then
        local json_str = string.format('[%d] %s: ', state.frame_count, entry_type)
        for k, v in pairs(data) do
            json_str = json_str .. string.format('%s=%s ', k, tostring(v))
        end
        print(json_str)
    end
end

-- =============================================================================
-- Game State Reading
-- =============================================================================

local function get_player_ptr()
    local gs = ADDR.GameState
    return read_u32(gs + ADDR.GS_player)
end

local function get_player_state()
    local player_ptr = get_player_ptr()
    if player_ptr == 0 or player_ptr < 0x80000000 then
        return nil
    end
    
    return {
        ptr = player_ptr,
        x = read_s16(player_ptr + ADDR.PLAYER_x),
        y = read_s16(player_ptr + ADDR.PLAYER_y),
        vx = read_s16(player_ptr + ADDR.PLAYER_vx),
        vy = read_s16(player_ptr + ADDR.PLAYER_vy),
        facing = read_u8(player_ptr + ADDR.PLAYER_facing),
        sprite_id = read_u32(player_ptr + ADDR.PLAYER_sprite_id),
        anim_frame = read_u16(player_ptr + ADDR.PLAYER_anim_frame),
        anim_end = read_u16(player_ptr + ADDR.PLAYER_anim_end),
        state_offset = read_u16(player_ptr + ADDR.PLAYER_state_offset),
        callback = read_u32(player_ptr + ADDR.PLAYER_callback),
    }
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

-- =============================================================================
-- Breakpoint Handlers
-- =============================================================================

local function on_entity_set_state(address, width, cause)
    local regs = PCSX.getRegisters()
    local entity_ptr = regs.GPR.n.a0
    local state_data = regs.GPR.n.a1
    local callback_ptr = regs.GPR.n.a2
    
    -- Check if this is the player
    local player_ptr = get_player_ptr()
    if entity_ptr == player_ptr then
        local cb_name = get_callback_name(callback_ptr)
        log_entry("PlayerStateChange", {
            entity = string.format("0x%08X", entity_ptr),
            new_callback = cb_name,
            state_data = string.format("0x%08X", state_data),
        })
    end
end

local function on_set_sprite_id(address, width, cause)
    local regs = PCSX.getRegisters()
    local entity_ptr = regs.GPR.n.a0
    local sprite_id = regs.GPR.n.a1
    local param = regs.GPR.n.a2
    
    local player_ptr = get_player_ptr()
    if entity_ptr == player_ptr then
        log_entry("PlayerSpriteChange", {
            sprite_id = string.format("0x%08X", sprite_id),
            param = param,
        })
    end
end

local function on_update_sprite_frame(address, width, cause)
    local regs = PCSX.getRegisters()
    local entity_ptr = regs.GPR.n.a0
    
    local player_ptr = get_player_ptr()
    if entity_ptr == player_ptr then
        local ps = get_player_state()
        if ps then
            log_entry("PlayerAnimFrame", {
                frame = ps.anim_frame,
                end_frame = ps.anim_end,
            })
        end
    end
end

local function on_tile_attribute_check(address, width, cause)
    local regs = PCSX.getRegisters()
    local x = regs.GPR.n.a0
    local y = regs.GPR.n.a1
    
    -- Only log occasionally to avoid spam
    if state.frame_count % 60 == 0 then
        log_entry("TileAttrCheck", {
            x = x,
            y = y,
        })
    end
end

local function on_collision_check(address, width, cause)
    local regs = PCSX.getRegisters()
    local entity_a = regs.GPR.n.a0
    local entity_b = regs.GPR.n.a1
    
    local player_ptr = get_player_ptr()
    if entity_a == player_ptr or entity_b == player_ptr then
        log_entry("PlayerCollision", {
            entity_a = string.format("0x%08X", entity_a),
            entity_b = string.format("0x%08X", entity_b),
        })
    end
end

local function on_level_load(address, width, cause)
    local regs = PCSX.getRegisters()
    local level_idx = regs.GPR.n.a0
    local stage_idx = regs.GPR.n.a1
    
    log_entry("LevelLoad", {
        level = level_idx,
        stage = stage_idx,
    })
end

local function on_spawn_entities(address, width, cause)
    log_entry("SpawnEntities", {
        pc = string.format("0x%08X", address),
    })
end

local function on_player_tick(address, width, cause)
    local regs = PCSX.getRegisters()
    local player_ptr = regs.GPR.n.a0
    
    local ps = get_player_state()
    if ps then
        -- Detect state changes
        local cb_name = get_callback_name(ps.callback)
        
        if ps.callback ~= state.last_player_state then
            log_entry("PlayerState", {
                callback = cb_name,
                x = ps.x,
                y = ps.y,
                vx = ps.vx,
                vy = ps.vy,
            })
            state.last_player_state = ps.callback
        end
        
        -- Detect position changes (log every N frames)
        if state.frame_count % CONFIG.sample_every_n_frames == 0 then
            if ps.x ~= state.last_player_x or ps.y ~= state.last_player_y then
                log_entry("PlayerMove", {
                    x = ps.x,
                    y = ps.y,
                    vx = ps.vx,
                    vy = ps.vy,
                    facing = ps.facing == 0 and "right" or "left",
                })
                state.last_player_x = ps.x
                state.last_player_y = ps.y
            end
        end
        
        -- Detect animation changes
        if ps.anim_frame ~= state.last_anim_frame then
            log_entry("PlayerAnim", {
                frame = ps.anim_frame,
                end_frame = ps.anim_end,
                sprite_id = string.format("0x%08X", ps.sprite_id),
            })
            state.last_anim_frame = ps.anim_frame
        end
    end
end

-- =============================================================================
-- VSync Handler (per-frame sampling)
-- =============================================================================

local function on_vsync()
    state.frame_count = state.frame_count + 1
end

-- =============================================================================
-- Setup and Teardown
-- =============================================================================

local function setup_breakpoints()
    print("Setting up Skullmonkeys game watchers...")
    
    if CONFIG.watch_player then
        state.breakpoints.player_tick = PCSX.addBreakpoint(
            ADDR.FN_PlayerTickCallback, 'Exec', 4, 
            'PlayerTickCallback', on_player_tick
        )
        print("  - Player tick watcher enabled")
    end
    
    if CONFIG.watch_animation then
        state.breakpoints.set_state = PCSX.addBreakpoint(
            ADDR.FN_EntitySetState, 'Exec', 4,
            'EntitySetState', on_entity_set_state
        )
        state.breakpoints.set_sprite = PCSX.addBreakpoint(
            ADDR.FN_SetEntitySpriteId, 'Exec', 4,
            'SetEntitySpriteId', on_set_sprite_id
        )
        print("  - Animation watchers enabled")
    end
    
    if CONFIG.watch_collision then
        state.breakpoints.collision = PCSX.addBreakpoint(
            ADDR.FN_CheckEntityCollision, 'Exec', 4,
            'CheckEntityCollision', on_collision_check
        )
        print("  - Collision watcher enabled")
    end
    
    if CONFIG.watch_level_load then
        state.breakpoints.level_load = PCSX.addBreakpoint(
            ADDR.FN_LoadLevelFromBLB, 'Exec', 4,
            'LoadLevelFromBLB', on_level_load
        )
        state.breakpoints.spawn = PCSX.addBreakpoint(
            ADDR.FN_SpawnPlayerAndEntities, 'Exec', 4,
            'SpawnPlayerAndEntities', on_spawn_entities
        )
        print("  - Level load watchers enabled")
    end
    
    -- VSync listener for frame counting
    state.listeners.vsync = PCSX.Events.createEventListener('GPU::Vsync', on_vsync)
    
    print("Watchers active. Use dump_log() to save captured data.")
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
        -- Simple JSON serialization
        local json = string.format('{"frame":%d,"type":"%s","data":{', 
            entry.frame, entry.type)
        local first = true
        for k, v in pairs(entry.data) do
            if not first then json = json .. "," end
            first = false
            if type(v) == "string" then
                json = json .. string.format('"%s":"%s"', k, v)
            else
                json = json .. string.format('"%s":%s', k, tostring(v))
            end
        end
        json = json .. "}}\n"
        file:write(json)
    end
    
    file:close()
    print(string.format("Wrote %d entries to %s", #state.log_entries, filename))
end

function clear_log()
    state.log_entries = {}
    state.frame_count = 0
    print("Log cleared.")
end

function status()
    local ps = get_player_state()
    local cam = get_camera()
    
    print("=== Skullmonkeys Status ===")
    print(string.format("Frame: %d", state.frame_count))
    print(string.format("Log entries: %d", #state.log_entries))
    print(string.format("Camera: (%d, %d)", cam.x, cam.y))
    
    if ps then
        print(string.format("Player: 0x%08X", ps.ptr))
        print(string.format("  Position: (%d, %d)", ps.x, ps.y))
        print(string.format("  Velocity: (%d, %d)", ps.vx, ps.vy))
        print(string.format("  Facing: %s", ps.facing == 0 and "right" or "left"))
        print(string.format("  State: %s", get_callback_name(ps.callback)))
        print(string.format("  Animation: %d/%d", ps.anim_frame, ps.anim_end))
        print(string.format("  Sprite: 0x%08X", ps.sprite_id))
    else
        print("Player: not loaded")
    end
end

function snapshot()
    -- Return current state as a table (can be serialized to JSON)
    local ps = get_player_state()
    local cam = get_camera()
    
    return {
        frame = state.frame_count,
        camera = cam,
        player = ps and {
            x = ps.x,
            y = ps.y,
            vx = ps.vx,
            vy = ps.vy,
            facing = ps.facing == 0 and "right" or "left",
            state = get_callback_name(ps.callback),
            anim_frame = ps.anim_frame,
            anim_end = ps.anim_end,
            sprite_id = string.format("0x%08X", ps.sprite_id),
        } or nil,
    }
end

-- =============================================================================
-- Initialization
-- =============================================================================

print("========================================")
print("Skullmonkeys Game Watcher v1.0")
print("PAL version (SLES-01090)")
print("========================================")
print("")
print("Commands:")
print("  status()    - Show current game state")
print("  snapshot()  - Get state as table")
print("  dump_log()  - Save log to file")
print("  clear_log() - Clear captured log")
print("  cleanup()   - Remove all watchers")
print("")

setup_breakpoints()
