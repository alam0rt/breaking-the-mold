--[[
  Raw RAM Dumper for Skullmonkeys (SLES-01090 PAL)

  Philosophy: the in-emulator job is DUMB and FAST. Copy N bytes to a file per
  frame and return. Zero JSON, zero struct parsing, zero per-entity iteration.
  All interpretation happens offline in tools/pcsx_trace.py, which parses the
  raw frames against the struct schema in include/Game/*.h.

  This replaces the 2000-line game_watcher.lua for reference-trace capture: that
  script did its analysis *inside* the emulator's single-threaded Lua VM every
  frame, and the JSON string-building + GC churn is what made it slow and crashy.

  Output: one raw binary file per sampled frame in an output directory, plus a
  manifest.json describing the capture (base address, region, frame list, level).

  Configuration is via environment variables so `make` never has to rewrite this
  file:
    RAMDUMP_OUT        output dir              (default: game_watcher/dumps/<ts>)
    RAMDUMP_INTERVAL   dump every N frames     (default: 1)
    RAMDUMP_MAX_FRAMES auto-exit after N dumps (default: 0 = run forever)
    RAMDUMP_REGION     full | gamestate | 0xADDR:SIZE
                       (default: full = the whole 2MB main RAM)
    RAMDUMP_LEVEL      boot override level idx (optional)
    RAMDUMP_STAGE      boot override stage idx (default: 0)

  Run via: make trace [LEVEL=1 STAGE=0] [REGION=full] [MAXFRAMES=600]
  Or directly: RAMDUMP_LEVEL=1 pcsx-redux -interpreter -debugger -lua_stdout \
                 -iso disks/pal/sles-01090.iso01.iso -dofile scripts/ram_dumper.lua -run
]]

local ffi = require("ffi")

-- ---------------------------------------------------------------------------
-- Config (env-driven, with CONFIG table override support for `make`)
-- ---------------------------------------------------------------------------
local function env(name, default)
    local v = os.getenv(name)
    if v == nil or v == "" then return default end
    return v
end

CONFIG = CONFIG or {}
CONFIG.out        = CONFIG.out        or env("RAMDUMP_OUT", nil)
CONFIG.fifo       = CONFIG.fifo       or env("RAMDUMP_FIFO", nil)
CONFIG.interval   = CONFIG.interval   or tonumber(env("RAMDUMP_INTERVAL", "1"))
CONFIG.max_frames = CONFIG.max_frames or tonumber(env("RAMDUMP_MAX_FRAMES", "0"))
CONFIG.region     = CONFIG.region     or env("RAMDUMP_REGION", "full")
CONFIG.state      = CONFIG.state      or env("RAMDUMP_STATE", nil)
CONFIG.level      = CONFIG.level      or tonumber(env("RAMDUMP_LEVEL", "") or "")
CONFIG.stage      = CONFIG.stage      or tonumber(env("RAMDUMP_STAGE", "0"))

-- ---------------------------------------------------------------------------
-- Known addresses / regions (PAL, SLES-01090)
-- ---------------------------------------------------------------------------
local RAM_BASE   = 0x80000000
local RAM_SIZE   = 0x200000           -- 2 MB main RAM
local GS_ADDR    = 0x8009DC40         -- g_GameStateBase
local GS_SIZE    = 0x1A0

-- BLB header addresses for boot override (see gamestate_dumper.lua)
local ADDR = {
    BLB_game_mode    = 0x800AF316,
    BLB_level_index  = 0x800AF372,
    BLB_stage_index  = 0x800AF373,
    LoadLevelFromBLB = 0x8007E474,
}

-- Curated "gamestate" slice: from GameState base through the end of the entity
-- heap region that offline analysis usually needs. Covers g_GameStateBase, the
-- BLB header buffer (0x800AE3E0) and the entity pool that follows.
local GAMESTATE_SLICE = { start = 0x8009DC40, size = 0x00013000 }

local function parse_region(spec)
    if spec == "full" then
        return { start = RAM_BASE, size = RAM_SIZE }
    elseif spec == "gamestate" then
        return GAMESTATE_SLICE
    else
        -- 0xADDR:SIZE  (SIZE may be hex 0x.. or decimal)
        local a, s = spec:match("^(0?x?%x+):(%x*x?%x*)$")
        a = spec:match("^([^:]+):")
        s = spec:match(":([^:]+)$")
        assert(a and s, "bad RAMDUMP_REGION: " .. tostring(spec) ..
            " (want 'full', 'gamestate', or '0xADDR:SIZE')")
        return { start = tonumber(a), size = tonumber(s) }
    end
end

local region = parse_region(CONFIG.region)
local mem    = PCSX.getMemPtr()

local function off(addr) return bit.band(addr, 0x1FFFFF) end
local function write_u8(addr, value) mem[off(addr)] = bit.band(value, 0xFF) end

-- ---------------------------------------------------------------------------
-- State
-- ---------------------------------------------------------------------------
local state = {
    frame        = 0,
    dumped       = 0,
    last_dump    = -1,
    session      = os.date("%Y%m%d_%H%M%S"),
    out_dir      = nil,
    frames       = {},          -- list of frame numbers dumped
    boot_applied = false,
    vsync        = nil,
    quit         = nil,
    bp           = nil,
}

-- Two sink modes:
--   FIFO mode  (RAMDUMP_FIFO): stream framed records to a pipe for a live
--                              external consumer (tools/pcsx_stream.py).
--   file mode  (default):      one raw .bin per frame + manifest.json.
state.fifo_file = nil
if CONFIG.fifo then
    -- opening the write end blocks until the consumer opens the read end
    state.fifo_file = io.open(CONFIG.fifo, "wb")
    if not state.fifo_file then
        error("could not open FIFO for writing: " .. CONFIG.fifo)
    end
else
    state.out_dir = CONFIG.out or ("game_watcher/dumps/" .. state.session)
    os.execute("mkdir -p '" .. state.out_dir .. "'")
end

-- little-endian u32 as a 4-byte string (no string.pack in this Lua)
local function u32le(n)
    n = n % 0x100000000
    return string.char(n % 256,
                       math.floor(n / 256) % 256,
                       math.floor(n / 65536) % 256,
                       math.floor(n / 16777216) % 256)
end

-- sink(frame_no, blob): where a captured frame goes
local function sink(frame_no, blob)
    if state.fifo_file then
        -- wire record: "FRM1" + frame:u32le + size:u32le + payload
        state.fifo_file:write("FRM1")
        state.fifo_file:write(u32le(frame_no))
        state.fifo_file:write(u32le(#blob))
        state.fifo_file:write(blob)
        state.fifo_file:flush()
    else
        local path = string.format("%s/frame_%08d.bin", state.out_dir, frame_no)
        local f = io.open(path, "wb")
        if f then f:write(blob); f:close() end
    end
    state.frames[#state.frames + 1] = frame_no
end

-- ---------------------------------------------------------------------------
-- Manifest
-- ---------------------------------------------------------------------------
local function write_manifest(final)
    if state.fifo_file then return end   -- FIFO mode: consumer owns metadata
    local f = io.open(state.out_dir .. "/manifest.json", "w")
    if not f then return end
    f:write("{\n")
    f:write(string.format('  "session": "%s",\n', state.session))
    f:write(string.format('  "region_base": %d,\n', region.start))
    f:write(string.format('  "region_base_hex": "0x%08X",\n', region.start))
    f:write(string.format('  "region_size": %d,\n', region.size))
    f:write(string.format('  "interval": %d,\n', CONFIG.interval))
    f:write(string.format('  "gamestate_addr": %d,\n', GS_ADDR))
    if CONFIG.level ~= nil then
        f:write(string.format('  "level": %d,\n', CONFIG.level))
        f:write(string.format('  "stage": %d,\n', CONFIG.stage or 0))
    end
    f:write(string.format('  "complete": %s,\n', final and "true" or "false"))
    f:write('  "frames": [')
    for i, fr in ipairs(state.frames) do
        f:write(tostring(fr))
        if i < #state.frames then f:write(", ") end
    end
    f:write("]\n}\n")
    f:close()
end

-- ---------------------------------------------------------------------------
-- Boot override
-- ---------------------------------------------------------------------------
-- Breakpoint invoker: patch the BLB header, then return false so the emulator
-- does NOT halt -- we just want to intercept, not pause.
local function apply_boot_override(address, width, cause)
    if not state.boot_applied and CONFIG.level ~= nil then
        write_u8(ADDR.BLB_game_mode, 3)                 -- level mode
        write_u8(ADDR.BLB_level_index, CONFIG.level)
        write_u8(ADDR.BLB_stage_index, CONFIG.stage or 0)
        state.boot_applied = true
        print(string.format("[BOOT] override -> level %d stage %d",
            CONFIG.level, CONFIG.stage or 0))
    end
    return false
end

-- ---------------------------------------------------------------------------
-- Hot path: copy region -> sink. This is ALL the emulator does per frame.
-- ---------------------------------------------------------------------------
local base_off = off(region.start)
local dest = CONFIG.fifo or state.out_dir

-- Finalize: flush metadata, close the sink so a FIFO consumer sees EOF, stop.
local function finish(tag)
    if state.finished then return end
    state.finished = true
    if state.vsync then state.vsync:remove(); state.vsync = nil end
    write_manifest(true)
    if state.fifo_file then state.fifo_file:close(); state.fifo_file = nil end
    print(string.format("[%s] %d frames -> %s", tag, state.dumped, dest))
    if CONFIG.max_frames > 0 and PCSX.quit then PCSX.quit(0) end
end

local function on_frame()
    -- Load a savestate on the first tick (emulation is now running), then run
    -- from there under dynarec -- the fast path that avoids the interpreter.
    if CONFIG.state and not state.state_loaded then
        state.state_loaded = true
        local sf = Support.File.open(CONFIG.state)
        PCSX.loadSaveState(sf)
        if sf.close then sf:close() end
        print("[STATE] loaded " .. CONFIG.state)
        return   -- don't capture the pre-load frame
    end

    state.frame = state.frame + 1
    if (state.frame - state.last_dump) < CONFIG.interval then return end
    state.last_dump = state.frame

    -- one alloc (ffi.string), one write. no parsing, no json, no gc-heavy loop.
    local blob = ffi.string(mem + base_off, region.size)
    sink(state.frame, blob)
    state.dumped = state.dumped + 1

    if CONFIG.max_frames > 0 and state.dumped >= CONFIG.max_frames then
        finish("DONE")
    elseif (state.dumped % 60) == 0 then
        write_manifest(false)   -- periodic manifest refresh so a killed run is usable
    end
end

-- ---------------------------------------------------------------------------
-- Setup
-- ---------------------------------------------------------------------------
print("=== ram_dumper ===")
print(string.format("  sink:     %s (%s mode)", dest,
    state.fifo_file and "fifo" or "file"))
print(string.format("  region:   0x%08X .. 0x%08X  (%d bytes)",
    region.start, region.start + region.size, region.size))
print(string.format("  interval: every %d frame(s)", CONFIG.interval))
print(string.format("  max:      %s",
    CONFIG.max_frames > 0 and tostring(CONFIG.max_frames) or "unbounded"))
if CONFIG.level ~= nil then
    print(string.format("  boot:     level %d stage %d", CONFIG.level, CONFIG.stage or 0))
end
write_manifest(false)

state.vsync = PCSX.Events.createEventListener("GPU::Vsync", on_frame)
state.quit  = PCSX.Events.createEventListener("Quitting", function()
    finish("EXIT")
end)

if CONFIG.level ~= nil then
    -- signature: addBreakpoint(address, type, width, cause, invoker, label)
    state.bp = PCSX.addBreakpoint(ADDR.LoadLevelFromBLB, "Exec", 4,
        "ram_dumper_boot", apply_boot_override, "ram_dumper_boot")
end

-- Manual stop from the Lua console.
function stop() finish("STOP") end
