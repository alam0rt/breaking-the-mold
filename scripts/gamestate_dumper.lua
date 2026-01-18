--[[
  GameState Raw Byte Dumper for Skullmonkeys (SLES-01090 PAL)
  
  Dumps the entire GameState struct (0x1A0 = 416 bytes) as raw hex
  every N frames for offline analysis.
  
  Load in PCSX-Redux via: Debug > Lua Console
  Or: pcsx-redux -dofile scripts/gamestate_dumper.lua
  
  Output: game_watcher/logs/gamestate_raw_<timestamp>.txt
  Format: One line per dump: "frame_number hex_bytes..."
]]

-- =============================================================================
-- Configuration
-- =============================================================================

local CONFIG = {
    dump_interval = 25,  -- Every ~0.5 seconds at 50fps
    log_dir = "game_watcher/logs",
    verbose = true,
    boot_level_override = nil,  -- Set to {level=1, stage=0} to force specific level
}

-- =============================================================================
-- Memory
-- =============================================================================

local GAMESTATE_ADDR = 0x8009DC40
local GAMESTATE_SIZE = 0x1A0  -- 416 bytes

-- BLB header addresses for boot override
local ADDR = {
    BLB_game_mode = 0x800AF316,
    BLB_level_index = 0x800AF372,
    BLB_stage_index = 0x800AF373,
    LoadLevelFromBLB = 0x8007E474,  -- Breakpoint for boot override
}

local mem = PCSX.getMemPtr()

local function psx_to_offset(addr)
    return bit.band(addr, 0x1FFFFF)
end

local function read_bytes_hex(addr, size)
    local off = psx_to_offset(addr)
    local hex = {}
    for i = 0, size - 1 do
        hex[i + 1] = string.format("%02X", mem[off + i])
    end
    return table.concat(hex)
end

local function write_u8(addr, value)
    local off = psx_to_offset(addr)
    mem[off] = bit.band(value, 0xFF)
end

-- =============================================================================
-- State
-- =============================================================================

local state = {
    frame_count = 0,
    log_file = nil,
    log_filename = nil,
    session_id = os.date("%Y%m%d_%H%M%S"),
    last_dump_frame = -999,
    vsync_listener = nil,
    exit_listener = nil,
    boot_override_applied = false,
    boot_breakpoint = nil,
}

-- =============================================================================
-- Boot Override (skip intro, load specific level)
-- =============================================================================

local function on_level_load(address, width, cause)
    if state.boot_override_applied then return end
    if not CONFIG.boot_level_override then return end
    
    local level_idx = CONFIG.boot_level_override.level or 0
    local stage_idx = CONFIG.boot_level_override.stage or 0
    
    print(string.format("[BOOT] Applying level override: Level %d, Stage %d", level_idx, stage_idx))
    
    -- Set level/stage in BLB header
    write_u8(ADDR.BLB_game_mode, 3)  -- Level mode
    write_u8(ADDR.BLB_level_index, level_idx)
    write_u8(ADDR.BLB_stage_index, stage_idx)
    
    state.boot_override_applied = true
    print("[BOOT] Override applied. Game will load this level.")
end

-- =============================================================================
-- Frame Handler
-- =============================================================================

local function on_frame()
    state.frame_count = state.frame_count + 1
    
    if state.frame_count - state.last_dump_frame < CONFIG.dump_interval then
        return
    end
    state.last_dump_frame = state.frame_count
    
    -- Dump raw bytes
    local hex = read_bytes_hex(GAMESTATE_ADDR, GAMESTATE_SIZE)
    
    if state.log_file then
        state.log_file:write(string.format("%d %s\n", state.frame_count, hex))
        state.log_file:flush()
    end
    
    if CONFIG.verbose then
        -- Print first 32 bytes for quick view
        print(string.format("[%d] %s...", state.frame_count, hex:sub(1, 64)))
    end
end

-- =============================================================================
-- Setup
-- =============================================================================

local function init()
    os.execute("mkdir -p " .. CONFIG.log_dir)
    
    local filename = string.format("%s/gamestate_raw_%s.txt", 
        CONFIG.log_dir, state.session_id)
    state.log_file = io.open(filename, "w")
    state.log_filename = filename
    
    if state.log_file then
        -- Write header
        state.log_file:write(string.format("# GameState dump: addr=0x%08X size=0x%X (%d bytes)\n",
            GAMESTATE_ADDR, GAMESTATE_SIZE, GAMESTATE_SIZE))
        state.log_file:write("# Format: frame_number hex_bytes\n")
        state.log_file:flush()
        
        print("GameState Raw Dumper started")
        print("  Output: " .. filename)
        print("  Interval: every " .. CONFIG.dump_interval .. " frames")
        print("  Address: 0x" .. string.format("%08X", GAMESTATE_ADDR))
        print("  Size: " .. GAMESTATE_SIZE .. " bytes")
    else
        print("ERROR: Could not open log file: " .. filename)
        return
    end
    
    -- GPU::Vsync fires every frame
    state.vsync_listener = PCSX.Events.createEventListener('GPU::Vsync', function()
        on_frame()
    end)
    
    state.exit_listener = PCSX.Events.createEventListener('Quitting', function()
        print("\n[EXIT] Closing...")
        if state.log_file then
            state.log_file:close()
            print("[EXIT] Saved: " .. state.log_filename)
        end
    end)
    
    -- Boot override breakpoint (fires when LoadLevelFromBLB is called)
    if CONFIG.boot_level_override then
        print(string.format("[BOOT] Will override to Level %d, Stage %d",
            CONFIG.boot_level_override.level or 0,
            CONFIG.boot_level_override.stage or 0))
        state.boot_breakpoint = PCSX.addBreakpoint(ADDR.LoadLevelFromBLB, "Exec", 4, 
            on_level_load, "boot_override")
    end
    
    print("Active. Use cleanup() to stop.")
end

-- =============================================================================
-- Cleanup
-- =============================================================================

function cleanup()
    if state.vsync_listener then
        state.vsync_listener:remove()
        state.vsync_listener = nil
    end
    if state.log_file then
        state.log_file:close()
        state.log_file = nil
        print("Stopped. Log: " .. state.log_filename)
    end
end

-- =============================================================================
-- Manual dump with field annotations
-- =============================================================================

function dump_now()
    local hex = read_bytes_hex(GAMESTATE_ADDR, GAMESTATE_SIZE)
    print("=== GameState @ 0x" .. string.format("%08X", GAMESTATE_ADDR) .. " ===")
    
    -- Print in 16-byte rows with offset
    for i = 0, GAMESTATE_SIZE - 1, 16 do
        local row = hex:sub(i*2 + 1, i*2 + 32)
        -- Add spaces between bytes
        local spaced = row:gsub("(..)", "%1 ")
        print(string.format("  %03X: %s", i, spaced))
    end
    
    if state.log_file then
        state.log_file:write(string.format("%d %s\n", state.frame_count, hex))
        state.log_file:flush()
    end
end

init()
