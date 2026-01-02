-- trace_blb_simple.lua - Reusable function tracing framework
-- For PCSX-Redux Lua scripting API
--
-- Usage: Define functions in TRACE_FUNCTIONS table with:
--   addr = function address
--   args = list of argument descriptors: {name, register, format}
--          format: "hex", "dec", "str" (reads null-terminated string at pointer)
--
-- Example:
--   { addr = 0x80010000, args = {{"ptr", "a0", "hex"}, {"count", "a1", "dec"}} }
--
-- Current Focus: Trace 0xED0 (unknown u32 array) and 0xF10 (credits table) regions
-- BLB header is loaded at 0x800AE3E0 (PAL)
-- 0xED0 in header = 0x800AE3E0 + 0xED0 = 0x800AF2B0
-- 0xF10 in header = 0x800AE3E0 + 0xF10 = 0x800AF2F0

print("===========================================")
print("BLB Header Trace Script")
print("Focus: 0xED0 array and 0xF10 credits table")
print("===========================================")

-- ============================================
-- CONFIGURATION: Add functions to trace here
-- ============================================

-- GameState structure offsets (discovered from trace + code analysis)
local GAMESTATE_OFFSETS = {
    headerBuffer = 0x5C,  -- Pointer to BLB header in RAM
    stateOffset = 0x60,   -- Current offset into state sliding window
}

-- Header offsets for state data
local HEADER_OFFSETS = {
    gameModeBase = 0xF36,   -- Mode byte at this + stateOffset
    assetIndexBase = 0xF92, -- Index byte at this + stateOffset
}

local TRACE_FUNCTIONS = {
    {
        name = "LoadBLBHeader",
        addr = 0x800208B0,
        args = {{"gameState", "a0", "hex"}}
    },
    {
        name = "GetCurrentSectorOffset",
        addr = 0x8007ABCC,
        args = {{"gameState", "a0", "hex"}},
        -- Read stateOffset from gameState+0x60, mode/index from header
        readState = true
    },
    {
        name = "GetCurrentSectorCount",
        addr = 0x8007AC54,
        args = {{"gameState", "a0", "hex"}},
        readState = true
    },
    {
        name = "getLevelName",
        addr = 0x8007AA08,
        args = {{"gameState", "a0", "hex"}, {"levelIndex", "a1", "dec"}}
    },
    {
        name = "GetMovieSectorCount",
        addr = 0x8007AE58,
        args = {{"gameState", "a0", "hex"}, {"movieIndex", "a1", "dec"}},
        readState = true
    },
    {
        name = "GetMovieUnknown00",
        addr = 0x8007AE14,
        args = {{"gameState", "a0", "hex"}, {"movieIndex", "a1", "dec"}},
        readState = true
    },
    {
        name = "LevelDataParser",
        addr = 0x8007A62C,
        args = {{"gameState", "a0", "hex"}}
    },
    {
        name = "GetLevelCount",
        addr = 0x8007A9B0,
        args = {{"gameState", "a0", "hex"}}
    },
    {
        name = "GetAssetCount",
        addr = 0x8007ACDC,
        args = {{"gameState", "a0", "hex"}}
    },
    {
        name = "GetCurrentMovieFilename",
        addr = 0x8007ADC8,
        args = {{"gameState", "a0", "hex"}},
        readState = true
    },
    {
        name = "GetCurrentMovieShortName",
        addr = 0x8007AD7C,
        args = {{"gameState", "a0", "hex"}},
        readState = true
    },
    {
        name = "GetCurrentMovieReserved",
        addr = 0x8007AD30,
        args = {{"gameState", "a0", "hex"}},
        readState = true
    },
    -- Unknown caller functions from trace analysis
    {
        name = "func_8007CF08",
        addr = 0x8007CF08,
        args = {{"a0", "a0", "hex"}, {"a1", "a1", "dec"}}
    },
    {
        name = "func_8008299C", 
        addr = 0x8008299C,
        args = {{"a0", "a0", "hex"}, {"a1", "a1", "dec"}}
    },
    {
        name = "func_8007D2F0",
        addr = 0x8007D2F0,
        args = {{"a0", "a0", "hex"}}
    },
    {
        name = "func_8007D53C",
        addr = 0x8007D53C,
        args = {{"a0", "a0", "hex"}}
    },
}

-- ============================================
-- Framework (no need to modify below)
-- ============================================
local breakpoints = {}
local logCount = 0
local callCounts = {}  -- Track per-function call counts

-- Helper to format hex
local function hex(val)
    if val then
        return string.format("0x%08X", val)
    end
    return "nil"
end

-- Read null-terminated string from memory
local function readString(addr, maxLen)
    maxLen = maxLen or 32
    local mem = PCSX.getMemPtr()
    local physAddr = bit.band(addr, 0x1FFFFF)
    local str = ""
    for i = 0, maxLen - 1 do
        local byte = mem[physAddr + i]
        if byte == 0 then break end
        if byte >= 32 and byte < 127 then
            str = str .. string.char(byte)
        else
            str = str .. "."
        end
    end
    return str
end

-- Read u8 from memory
local function readU8(addr)
    local mem = PCSX.getMemPtr()
    local physAddr = bit.band(addr, 0x1FFFFF)
    return mem[physAddr]
end

-- Read u32 from memory (little-endian)
local function readU32(addr)
    local mem = PCSX.getMemPtr()
    local physAddr = bit.band(addr, 0x1FFFFF)
    return mem[physAddr] + mem[physAddr + 1] * 0x100 + 
           mem[physAddr + 2] * 0x10000 + mem[physAddr + 3] * 0x1000000
end

-- Read GameState sliding window state (mode + index)
local function readGameState(gameStateAddr)
    -- Read headerBuffer pointer from gameState+0x5C
    local headerPtr = readU32(gameStateAddr + GAMESTATE_OFFSETS.headerBuffer)
    -- Read stateOffset from gameState+0x60
    local stateOffset = readU8(gameStateAddr + GAMESTATE_OFFSETS.stateOffset)
    
    -- Read mode and index from header
    local mode = readU8(headerPtr + HEADER_OFFSETS.gameModeBase + stateOffset)
    local index = readU8(headerPtr + HEADER_OFFSETS.assetIndexBase + stateOffset)
    
    return {
        headerPtr = headerPtr,
        stateOffset = stateOffset,
        mode = mode,
        index = index
    }
end

-- Get register value by name
local function getReg(regs, regName)
    if regName == "a0" then return regs.GPR.n.a0
    elseif regName == "a1" then return regs.GPR.n.a1
    elseif regName == "a2" then return regs.GPR.n.a2
    elseif regName == "a3" then return regs.GPR.n.a3
    elseif regName == "v0" then return regs.GPR.n.v0
    elseif regName == "v1" then return regs.GPR.n.v1
    elseif regName == "ra" then return regs.GPR.n.ra
    elseif regName == "sp" then return regs.GPR.n.sp
    else return 0
    end
end

-- Format argument value based on format type
local function formatArg(regs, argDef)
    local name, reg, fmt = argDef[1], argDef[2], argDef[3]
    local val = getReg(regs, reg)
    
    if fmt == "hex" then
        return string.format("%s=%s", name, hex(val))
    elseif fmt == "dec" then
        return string.format("%s=%d", name, val)
    elseif fmt == "str" then
        return string.format("%s=\"%s\"", name, readString(val))
    else
        return string.format("%s=%s", name, hex(val))
    end
end

-- Create breakpoint callback for a function
local function createCallback(funcDef)
    return function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        callCounts[funcDef.name] = (callCounts[funcDef.name] or 0) + 1
        
        -- Format arguments
        local argStrs = {}
        for _, argDef in ipairs(funcDef.args or {}) do
            table.insert(argStrs, formatArg(regs, argDef))
        end
        
        -- If readState is true, read the sliding window state from GameState
        local stateStr = ""
        if funcDef.readState then
            local gameStateAddr = regs.GPR.n.a0
            local success, state = pcall(readGameState, gameStateAddr)
            if success and state then
                stateStr = string.format(" [stateOffset=%d, mode=%d, index=%d]", 
                    state.stateOffset, state.mode, state.index)
            end
        end
        
        local argStr = table.concat(argStrs, ", ")
        print(string.format("[%04d] %s(%s)%s RA=%s", 
            logCount, funcDef.name, argStr, stateStr, hex(regs.GPR.n.ra)))
    end
end

-- Set up breakpoints
print("\nSetting up function breakpoints...")
for _, funcDef in ipairs(TRACE_FUNCTIONS) do
    local bp = PCSX.addBreakpoint(funcDef.addr, 'Exec', 4, funcDef.name, createCallback(funcDef))
    table.insert(breakpoints, bp)
    print(string.format("  %s @ %s", funcDef.name, hex(funcDef.addr)))
end

-- ============================================
-- NOTE: Memory breakpoints (Read type) crash PCSX-Redux!
-- Even small targeted breakpoints cause segfaults.
-- To trace 0xED0/0xF10 access, we need to find and trace
-- the functions that read from those regions instead.
-- ============================================

-- ============================================
-- Interactive commands
-- ============================================

function printSummary()
    print("\n===========================================")
    print("CALL SUMMARY")
    print("===========================================")
    
    -- Sort by count descending
    local sorted = {}
    for name, count in pairs(callCounts) do
        table.insert(sorted, {name = name, count = count})
    end
    table.sort(sorted, function(a, b) return a.count > b.count end)
    
    for _, entry in ipairs(sorted) do
        print(string.format("  %-30s %d calls", entry.name, entry.count))
    end
    print("-------------------------------------------")
    print(string.format("  %-30s %d", "TOTAL", logCount))
    print("===========================================\n")
end

function clearLog()
    logCount = 0
    callCounts = {}
    print("Log cleared.")
end

-- Add a new function to trace at runtime
function addTrace(name, addr, args)
    local funcDef = { name = name, addr = addr, args = args or {} }
    local bp = PCSX.addBreakpoint(addr, 'Exec', 4, name, createCallback(funcDef))
    table.insert(breakpoints, bp)
    table.insert(TRACE_FUNCTIONS, funcDef)
    print(string.format("Added trace: %s @ %s", name, hex(addr)))
end

print("\n===========================================")
print("Commands:")
print("  printSummary()  - Show call counts")
print("  clearLog()      - Reset counters")
print("  addTrace(name, addr, args) - Add new trace")
print("")
print("Example: addTrace('MyFunc', 0x80010000, {{'arg1', 'a0', 'hex'}})")
print("===========================================\n")
