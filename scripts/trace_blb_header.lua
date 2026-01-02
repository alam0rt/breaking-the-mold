-- trace_blb_header.lua - Trace functions that interact with the BLB header
-- For PCSX-Redux Lua scripting API
--
-- This script sets up memory read breakpoints on the BLB header region
-- to identify which functions access which parts of the header.
--
-- BLB Header Layout (0x1000 bytes = 2 sectors, loaded at GameState+0x40):
-- =======================================================================
-- Offset  Size   Description
-- ------  ----   -----------
-- 0x000   2912   Level metadata table (0x70 bytes × 26 entries)
-- 0xB60    364   Movie table (0x1C bytes × 13 entries)
-- 0xCD0    512   Sector/loading screen table (0x10 bytes × 32 entries)
-- 0xED0     64   Unknown u32 array (16 entries) - per-world data?
-- 0xF10     33   Credits sequence table (0x0C bytes × 2-3 entries)
-- 0xF31      1   Level count (u8)
-- 0xF32      1   Asset/movie count (u8)
-- 0xF33      1   Sector table entry count (u8)
-- 0xF34    204   State/config data region
--
-- State/Config Data (0xF34-0x1000):
--   0xF36: Game mode (1=movie, 2=credits, 4-5=level)
--   0xF37: Secondary state flag
--   0xF92: Current asset/movie/entry index for table lookups
--
-- Level Metadata Entry (0x70 bytes each):
--   +0x00: u16 sector_offset, +0x02: u16 sector_count (primary data)
--   +0x0C: u8 level_index, +0x0D: u8 flag
--   +0x1E: u16 secondary_offset, +0x2C: u16 secondary_count
--   +0x40: u16 tertiary_offset, +0x4E: u16 tertiary_count
--   +0x56: 5-byte level_id (e.g., "MENU")
--   +0x5B: 21-byte level name
--
-- Movie Entry (0x1C bytes each):
--   +0x00: u16 reserved (always 0), +0x02: u16 sector_count
--   +0x04: 5-byte movie_id, +0x09: 3-byte short_name
--   +0x0C: 16-byte filename (CD path)
--
-- Sector Table Entry (0x10 bytes each):
--   +0x00: u8 level_index, +0x01: u8 entry_flags
--   +0x02: u8 unknown, +0x03: 5-byte code
--   +0x08: 4-byte short_name
--   +0x0C: u16 sector_offset, +0x0E: u16 sector_count

local ffi = require('ffi')

-- ============================================
-- Configuration
-- ============================================

-- Known memory addresses from symbol_addrs.txt
local ADDRESSES = {
    -- Pointers to find the header at runtime
    D_800A5960 = 0x800A5960,  -- Points to GameState (headerBuffer at +0x40)
    D_800A5954 = 0x800A5954,  -- Points to EngineContext (headerBufferBase at +0xA650)
    
    -- Known BLB header accessor functions
    GetMovieUnknown00 = 0x8007AE14,
    GetMovieSectorCount = 0x8007AE58,
    getLevelName = 0x8007AA08,
    LoadBLBHeader = 0x800208B0,
    func_8007A62C = 0x8007A62C,  -- Level data parser
    func_8007A9B0 = 0x8007A9B0,  -- Level asset enumeration (uses level_count at 0xF31)
    func_8007ABCC = 0x8007ABCC,  -- Credits accessor (when state 0xF36 == 2)
    func_8007AC54 = 0x8007AC54,  -- Credits accessor (when state 0xF36 == 2)
    func_8007ACDC = 0x8007ACDC,  -- Uses asset_count at 0xF32
    func_8007ACF0 = 0x8007ACF0,  -- Another header accessor
    func_8007ADC8 = 0x8007ADC8,  -- GetMovieFilename
    
    -- CD/BLB functions
    CdBLB_ReadSectors = 0x80038BA0,
    CdBLB_WaitReadComplete = 0x80038C14,
}

-- BLB Header offsets and their meanings
local HEADER_REGIONS = {
    -- Level metadata table: 26 entries × 0x70 bytes = 2912 bytes
    { start = 0x000, stop = 0xB5F, name = "LevelMetadata", desc = "26 level entries × 0x70 bytes" },
    -- Movie table: 13 entries × 0x1C bytes = 364 bytes
    { start = 0xB60, stop = 0xCCB, name = "MovieTable", desc = "13 movie entries × 0x1C bytes" },
    -- Sector/loading screen table: 32 entries × 0x10 bytes = 512 bytes
    { start = 0xCD0, stop = 0xECF, name = "SectorTable", desc = "32 sector entries × 0x10 bytes" },
    -- Unknown u32 array: 16 entries = 64 bytes
    { start = 0xED0, stop = 0xF0F, name = "UnknownArray", desc = "16 u32 values, per-world data?" },
    -- Credits sequence table: 2-3 entries × 0x0C bytes
    { start = 0xF10, stop = 0xF30, name = "CreditsTable", desc = "Credits entries × 0x0C bytes" },
    -- Count fields
    { start = 0xF31, stop = 0xF31, name = "LevelCount", desc = "Number of levels (u8)" },
    { start = 0xF32, stop = 0xF32, name = "AssetCount", desc = "Number of movies (u8)" },
    { start = 0xF33, stop = 0xF33, name = "SectorTableCount", desc = "Number of sector table entries (u8)" },
    -- State/config region
    { start = 0xF34, stop = 0xF35, name = "UnknownF34", desc = "Unknown 2 bytes" },
    { start = 0xF36, stop = 0xF36, name = "GameMode", desc = "1=movie, 2=credits, 4-5=level" },
    { start = 0xF37, stop = 0xF37, name = "SecondaryFlag", desc = "Secondary state flag" },
    { start = 0xF38, stop = 0xF90, name = "LevelProgressData", desc = "Level progression data (89 bytes)" },
    { start = 0xF91, stop = 0xF91, name = "UnknownF91", desc = "Credits-related? (u8)" },
    { start = 0xF92, stop = 0xF92, name = "AssetIndex", desc = "Current table entry index (u8)" },
    { start = 0xF93, stop = 0xFFF, name = "WorldMappingData", desc = "Level/world mapping (109 bytes)" },
}

-- Known functions that access the header (for reverse lookup)
local KNOWN_FUNCTIONS = {
    [0x8007AE14] = "GetMovieUnknown00",
    [0x8007AE58] = "GetMovieSectorCount",
    [0x8007AA08] = "getLevelName",
    [0x800208B0] = "LoadBLBHeader",
    [0x8007A62C] = "func_8007A62C (LevelDataParser)",
    [0x8007A9B0] = "func_8007A9B0 (LevelAssetEnum)",
    [0x8007ABCC] = "func_8007ABCC (CreditsAccessor)",
    [0x8007AC54] = "func_8007AC54 (CreditsAccessor)",
    [0x8007ACDC] = "func_8007ACDC (AssetCountAccessor)",
    [0x8007ACF0] = "func_8007ACF0",
    [0x8007ADC8] = "func_8007ADC8 (GetMovieFilename)",
    [0x80038BA0] = "CdBLB_ReadSectors",
    [0x80038C14] = "CdBLB_WaitReadComplete",
}

-- ============================================
-- State
-- ============================================
local breakpoints = {}
local logCount = 0
local headerBaseAddr = nil
local accessLog = {}  -- Track unique (function, offset) pairs
local verboseMode = false  -- Set to true for detailed logging
local logNewAccessesOnly = true  -- Only log first occurrence of each access
local suppressUnknownFunctions = true  -- Suppress accesses from unknown functions

-- ============================================
-- Helper functions
-- ============================================

local function hex(val)
    if val == nil then return "nil" end
    if val < 0 then val = val + 0x100000000 end
    return string.format("0x%08X", val)
end

local function hex16(val)
    return string.format("0x%04X", bit.band(val, 0xFFFF))
end

-- Read a 32-bit value from PSX RAM
local function readU32(addr)
    local mem = PCSX.getMemPtr()
    local offset = bit.band(addr, 0x1FFFFF)
    return mem[offset] + 
           mem[offset + 1] * 0x100 + 
           mem[offset + 2] * 0x10000 + 
           mem[offset + 3] * 0x1000000
end

-- Find which region a header offset belongs to
local function getRegionName(headerOffset)
    for _, region in ipairs(HEADER_REGIONS) do
        if headerOffset >= region.start and headerOffset <= region.stop then
            return region.name .. " (" .. region.desc .. ")"
        end
    end
    return string.format("Unknown (0x%03X)", headerOffset)
end

-- Get detailed field info for an offset within a table entry
local function getFieldDetail(headerOffset)
    -- Level metadata entry fields (0x70 bytes each, starting at 0x000)
    if headerOffset >= 0x000 and headerOffset < 0xB60 then
        local entryIndex = math.floor(headerOffset / 0x70)
        local fieldOffset = headerOffset % 0x70
        local fieldName = ""
        
        if fieldOffset == 0x00 then fieldName = "sector_offset (u16)"
        elseif fieldOffset == 0x02 then fieldName = "sector_count (u16)"
        elseif fieldOffset >= 0x04 and fieldOffset < 0x0C then fieldName = "static_data"
        elseif fieldOffset == 0x0C then fieldName = "level_index (u8)"
        elseif fieldOffset == 0x0D then fieldName = "flag (u8)"
        elseif fieldOffset == 0x1E then fieldName = "secondary_offset (u16)"
        elseif fieldOffset == 0x2C then fieldName = "secondary_count (u16)"
        elseif fieldOffset == 0x40 then fieldName = "tertiary_offset (u16)"
        elseif fieldOffset == 0x4E then fieldName = "tertiary_count (u16)"
        elseif fieldOffset >= 0x56 and fieldOffset < 0x5B then fieldName = "level_id (5 bytes)"
        elseif fieldOffset >= 0x5B and fieldOffset < 0x70 then fieldName = "level_name (21 bytes)"
        else fieldName = string.format("unknown+0x%02X", fieldOffset)
        end
        
        return string.format("Level[%d].%s", entryIndex, fieldName)
    end
    
    -- Movie entry fields (0x1C bytes each, starting at 0xB60)
    if headerOffset >= 0xB60 and headerOffset < 0xCD0 then
        local entryIndex = math.floor((headerOffset - 0xB60) / 0x1C)
        local fieldOffset = (headerOffset - 0xB60) % 0x1C
        local fieldName = ""
        
        if fieldOffset == 0x00 then fieldName = "reserved (u16, always 0)"
        elseif fieldOffset == 0x02 then fieldName = "sector_count (u16)"
        elseif fieldOffset >= 0x04 and fieldOffset < 0x09 then fieldName = "movie_id (5 bytes)"
        elseif fieldOffset >= 0x09 and fieldOffset < 0x0C then fieldName = "short_name (3 bytes)"
        elseif fieldOffset >= 0x0C and fieldOffset < 0x1C then fieldName = "filename (16 bytes)"
        else fieldName = string.format("unknown+0x%02X", fieldOffset)
        end
        
        return string.format("Movie[%d].%s", entryIndex, fieldName)
    end
    
    -- Sector table entry fields (0x10 bytes each, starting at 0xCD0)
    if headerOffset >= 0xCD0 and headerOffset < 0xED0 then
        local entryIndex = math.floor((headerOffset - 0xCD0) / 0x10)
        local fieldOffset = (headerOffset - 0xCD0) % 0x10
        local fieldName = ""
        
        if fieldOffset == 0x00 then fieldName = "level_index (u8)"
        elseif fieldOffset == 0x01 then fieldName = "entry_flags (u8)"
        elseif fieldOffset == 0x02 then fieldName = "unknown_byte (u8)"
        elseif fieldOffset >= 0x03 and fieldOffset < 0x08 then fieldName = "code (5 bytes)"
        elseif fieldOffset >= 0x08 and fieldOffset < 0x0C then fieldName = "short_name (4 bytes)"
        elseif fieldOffset == 0x0C then fieldName = "sector_offset (u16)"
        elseif fieldOffset == 0x0E then fieldName = "sector_count (u16)"
        else fieldName = string.format("unknown+0x%02X", fieldOffset)
        end
        
        return string.format("Sector[%d].%s", entryIndex, fieldName)
    end
    
    -- Credits entry fields (0x0C bytes each, starting at 0xF10)
    if headerOffset >= 0xF10 and headerOffset < 0xF31 then
        local entryIndex = math.floor((headerOffset - 0xF10) / 0x0C)
        local fieldOffset = (headerOffset - 0xF10) % 0x0C
        local fieldName = ""
        
        if fieldOffset >= 0x00 and fieldOffset < 0x04 then fieldName = "code (4 bytes)"
        elseif fieldOffset >= 0x04 and fieldOffset < 0x08 then fieldName = "padding (4 bytes)"
        elseif fieldOffset == 0x08 then fieldName = "param_a (u16)"
        elseif fieldOffset == 0x0A then fieldName = "param_b (u16)"
        else fieldName = string.format("unknown+0x%02X", fieldOffset)
        end
        
        return string.format("Credits[%d].%s", entryIndex, fieldName)
    end
    
    return nil
end

-- Look up function name from PC address
local function getFunctionName(pc)
    -- Check exact matches first
    if KNOWN_FUNCTIONS[pc] then
        return KNOWN_FUNCTIONS[pc]
    end
    
    -- Check if PC is within a known function's range
    -- Sizes estimated from symbol_addrs.txt where available
    local functionRanges = {
        { start = 0x8007AE14, size = 0x44, name = "GetMovieUnknown00" },
        { start = 0x8007AE58, size = 0x44, name = "GetMovieSectorCount" },
        { start = 0x8007AA08, size = 0x20, name = "getLevelName" },
        { start = 0x800208B0, size = 0xC4, name = "LoadBLBHeader" },
        { start = 0x8007A62C, size = 0x254, name = "func_8007A62C (LevelDataParser)" },
        { start = 0x8007A9B0, size = 0x80, name = "func_8007A9B0 (LevelAssetEnum)" },
        { start = 0x8007ABCC, size = 0x44, name = "func_8007ABCC (CreditsAccessor)" },
        { start = 0x8007AC54, size = 0x44, name = "func_8007AC54 (CreditsAccessor)" },
        { start = 0x8007ACDC, size = 0x14, name = "func_8007ACDC (AssetCountAccessor)" },
        { start = 0x8007ACF0, size = 0x44, name = "func_8007ACF0" },
        { start = 0x8007ADC8, size = 0x4C, name = "func_8007ADC8 (GetMovieFilename)" },
        { start = 0x80038BA0, size = 0x74, name = "CdBLB_ReadSectors" },
        { start = 0x80038C14, size = 0x40, name = "CdBLB_WaitReadComplete" },
    }
    
    for _, func in ipairs(functionRanges) do
        if pc >= func.start and pc < func.start + func.size then
            return func.name .. string.format(" +0x%X", pc - func.start)
        end
    end
    
    return hex(pc)
end

-- ============================================
-- Dynamic header address discovery
-- ============================================

local function discoverHeaderAddress()
    -- Method 1: Read from D_800A5960 -> GameState+0x40
    local gameStatePtr = readU32(ADDRESSES.D_800A5960)
    if gameStatePtr ~= 0 and gameStatePtr >= 0x80000000 then
        local headerBuffer = readU32(gameStatePtr + 0x40)
        if headerBuffer ~= 0 and headerBuffer >= 0x80000000 then
            print("[INFO] Found headerBuffer via GameState: " .. hex(headerBuffer))
            return headerBuffer
        end
    end
    
    -- Method 2: Read from D_800A5954 -> EngineContext+0xA650
    local enginePtr = readU32(ADDRESSES.D_800A5954)
    if enginePtr ~= 0 and enginePtr >= 0x80000000 then
        local headerBufferBase = readU32(enginePtr + 0xA650)
        if headerBufferBase ~= 0 and headerBufferBase >= 0x80000000 then
            print("[INFO] Found headerBufferBase via EngineContext: " .. hex(headerBufferBase))
            return headerBufferBase
        end
    end
    
    -- Fallback: Known PAL address from NOTES.md
    print("[INFO] Using fallback PAL header address: 0x800AE3E0")
    return 0x800AE3E0
end

-- ============================================
-- Breakpoint callback for header reads
-- ============================================

local function onHeaderRead(address, width, cause)
    local regs = PCSX.getRegisters()
    local pc = regs.pc
    
    -- Calculate offset within header
    local headerOffset = address - headerBaseAddr
    
    -- Skip if outside header bounds
    if headerOffset < 0 or headerOffset >= 0x1000 then
        return
    end
    
    -- Get function name
    local funcName = getFunctionName(pc)
    
    -- Skip if from unknown function and suppression is enabled
    local isKnownFunction = KNOWN_FUNCTIONS[pc] ~= nil
    for _, func in ipairs({
        {0x8007AE14, 0x44}, {0x8007AE58, 0x44}, {0x8007AA08, 0x20},
        {0x800208B0, 0xC4}, {0x8007A62C, 0x254}, {0x8007A9B0, 0x80},
        {0x8007ABCC, 0x44}, {0x8007AC54, 0x44}, {0x8007ACDC, 0x14},
        {0x8007ACF0, 0x44}, {0x8007ADC8, 0x4C}, {0x80038BA0, 0x74},
        {0x80038C14, 0x40}
    }) do
        if pc >= func[1] and pc < func[1] + func[2] then
            isKnownFunction = true
            break
        end
    end
    
    if suppressUnknownFunctions and not isKnownFunction then
        -- Still count it but don't log
        local key = string.format("%s:0x%03X", funcName, headerOffset)
        if not accessLog[key] then
            accessLog[key] = { count = 0, firstLog = logCount, isKnown = false }
        end
        accessLog[key].count = accessLog[key].count + 1
        logCount = logCount + 1
        return
    end
    
    -- Get region name and field detail
    local regionName = getRegionName(headerOffset)
    local fieldDetail = getFieldDetail(headerOffset)
    
    -- Create unique key for deduplication
    local key = string.format("%s:0x%03X", funcName, headerOffset)
    
    -- Log the access
    logCount = logCount + 1
    
    -- Track unique accesses
    if not accessLog[key] then
        accessLog[key] = { count = 0, firstLog = logCount, isKnown = true }
    end
    accessLog[key].count = accessLog[key].count + 1
    
    -- Only print first occurrence (or all if verbose mode)
    local shouldPrint = false
    if verboseMode then
        shouldPrint = true
    elseif logNewAccessesOnly then
        shouldPrint = (accessLog[key].count == 1)
    else
        shouldPrint = (accessLog[key].count == 1 or accessLog[key].count % 1000 == 0)
    end
    
    if shouldPrint then
        local suffix = ""
        if accessLog[key].count > 1 then
            suffix = string.format(" (x%d)", accessLog[key].count)
        end
        
        print(string.format("[%04d] READ header+0x%03X (%d bytes) by %s%s",
            logCount, headerOffset, width, funcName, suffix))
        if fieldDetail then
            print(string.format("       -> %s", fieldDetail))
        end
    end
end

local function onHeaderWrite(address, width, cause)
    local regs = PCSX.getRegisters()
    local pc = regs.pc
    
    -- Calculate offset within header
    local headerOffset = address - headerBaseAddr
    
    -- Skip if outside header bounds
    if headerOffset < 0 or headerOffset >= 0x1000 then
        return
    end
    
    -- Get function name
    local funcName = getFunctionName(pc)
    
    -- Get region name and field detail
    local regionName = getRegionName(headerOffset)
    local fieldDetail = getFieldDetail(headerOffset)
    
    logCount = logCount + 1
    print(string.format("[%04d] WRITE header+0x%03X (%d bytes) by %s",
        logCount, headerOffset, width, funcName))
    print(string.format("       Region: %s", regionName))
    if fieldDetail then
        print(string.format("       Field:  %s", fieldDetail))
    end
    print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
end

-- ============================================
-- Hook known BLB functions
-- ============================================

local function hookKnownFunctions()
    print("\n=== Hooking known BLB functions ===")
    
    -- Hook LoadBLBHeader
    local bp = PCSX.addBreakpoint(ADDRESSES.LoadBLBHeader, 'Exec', 4, "LoadBLBHeader", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("\n[%04d] >>> LoadBLBHeader called", logCount))
        print(string.format("       a0 (GameState*): %s", hex(regs.GPR.n.a0)))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
        
        -- Update header address after this returns
        -- (we'll rediscover it on next access)
    end)
    table.insert(breakpoints, bp)
    print("  Hooked LoadBLBHeader @ " .. hex(ADDRESSES.LoadBLBHeader))
    
    -- Hook level data parser
    bp = PCSX.addBreakpoint(ADDRESSES.func_8007A62C, 'Exec', 4, "LevelDataParser", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("\n[%04d] >>> func_8007A62C (LevelDataParser) called", logCount))
        print(string.format("       a0 (LevelDataContext*): %s", hex(regs.GPR.n.a0)))
        print(string.format("       a1 (buffer*): %s", hex(regs.GPR.n.a1)))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("  Hooked func_8007A62C @ " .. hex(ADDRESSES.func_8007A62C))
    
    -- Hook getLevelName
    bp = PCSX.addBreakpoint(ADDRESSES.getLevelName, 'Exec', 4, "getLevelName", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("\n[%04d] >>> getLevelName called", logCount))
        print(string.format("       a0 (header*): %s", hex(regs.GPR.n.a0)))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("  Hooked getLevelName @ " .. hex(ADDRESSES.getLevelName))
    
    -- Hook GetMovieUnknown00
    bp = PCSX.addBreakpoint(ADDRESSES.GetMovieUnknown00, 'Exec', 4, "GetMovieUnknown00", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("\n[%04d] >>> GetMovieUnknown00 called", logCount))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("  Hooked GetMovieUnknown00 @ " .. hex(ADDRESSES.GetMovieUnknown00))
    
    -- Hook GetMovieSectorCount
    bp = PCSX.addBreakpoint(ADDRESSES.GetMovieSectorCount, 'Exec', 4, "GetMovieSectorCount", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("\n[%04d] >>> GetMovieSectorCount called", logCount))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("  Hooked GetMovieSectorCount @ " .. hex(ADDRESSES.GetMovieSectorCount))
    
    -- Hook func_8007A9B0 (LevelAssetEnum - uses level_count at 0xF31)
    bp = PCSX.addBreakpoint(ADDRESSES.func_8007A9B0, 'Exec', 4, "LevelAssetEnum", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("\n[%04d] >>> func_8007A9B0 (LevelAssetEnum) called", logCount))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("  Hooked func_8007A9B0 @ " .. hex(ADDRESSES.func_8007A9B0))
    
    -- Hook func_8007ABCC (CreditsAccessor - when state 0xF36 == 2)
    bp = PCSX.addBreakpoint(ADDRESSES.func_8007ABCC, 'Exec', 4, "CreditsAccessor", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("\n[%04d] >>> func_8007ABCC (CreditsAccessor) called", logCount))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("  Hooked func_8007ABCC @ " .. hex(ADDRESSES.func_8007ABCC))
    
    -- Hook func_8007AC54 (CreditsAccessor - when state 0xF36 == 2)
    bp = PCSX.addBreakpoint(ADDRESSES.func_8007AC54, 'Exec', 4, "CreditsAccessor2", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("\n[%04d] >>> func_8007AC54 (CreditsAccessor) called", logCount))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("  Hooked func_8007AC54 @ " .. hex(ADDRESSES.func_8007AC54))
    
    -- Hook func_8007ADC8 (GetMovieFilename)
    bp = PCSX.addBreakpoint(ADDRESSES.func_8007ADC8, 'Exec', 4, "GetMovieFilename", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("\n[%04d] >>> func_8007ADC8 (GetMovieFilename) called", logCount))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("  Hooked func_8007ADC8 @ " .. hex(ADDRESSES.func_8007ADC8))
    
    -- Hook CdBLB_ReadSectors
    bp = PCSX.addBreakpoint(ADDRESSES.CdBLB_ReadSectors, 'Exec', 4, "CdBLB_ReadSectors", function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        local sectorOffset = regs.GPR.n.a0
        local numSectors = regs.GPR.n.a1
        local destBuffer = regs.GPR.n.a2
        local byteOffset = sectorOffset * 2048
        local byteSize = numSectors * 2048
        
        print(string.format("\n[%04d] >>> CdBLB_ReadSectors(sector=%d, count=%d, dest=%s)",
            logCount, sectorOffset, numSectors, hex(destBuffer)))
        print(string.format("       BLB byte offset: 0x%X, size: 0x%X", byteOffset, byteSize))
        print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("  Hooked CdBLB_ReadSectors @ " .. hex(ADDRESSES.CdBLB_ReadSectors))
end

-- ============================================
-- Setup memory breakpoints on header regions
-- ============================================

local function setupHeaderBreakpoints()
    print("\n=== Setting up BLB header memory breakpoints ===")
    
    -- Discover header address
    headerBaseAddr = discoverHeaderAddress()
    print("Header base address: " .. hex(headerBaseAddr))
    
    -- Set breakpoints on key header regions
    -- Note: Setting one large breakpoint is more efficient than many small ones
    
    -- Full header read monitor (0x1000 bytes)
    local bp = PCSX.addBreakpoint(headerBaseAddr, 'Read', 0x1000, "BLB Header Read", 
        function(address, width, cause)
            local success, msg = pcall(function()
                onHeaderRead(address, width, cause)
            end)
            if not success then
                print("[ERROR] Breakpoint callback failed: " .. tostring(msg))
            end
        end)
    table.insert(breakpoints, bp)
    print("  Set READ breakpoint on header region: " .. hex(headerBaseAddr) .. " - " .. hex(headerBaseAddr + 0xFFF))
    
    -- Full header write monitor (to catch header updates)
    bp = PCSX.addBreakpoint(headerBaseAddr, 'Write', 0x1000, "BLB Header Write",
        function(address, width, cause)
            local success, msg = pcall(function()
                onHeaderWrite(address, width, cause)
            end)
            if not success then
                print("[ERROR] Breakpoint callback failed: " .. tostring(msg))
            end
        end)
    table.insert(breakpoints, bp)
    print("  Set WRITE breakpoint on header region: " .. hex(headerBaseAddr) .. " - " .. hex(headerBaseAddr + 0xFFF))
    
    -- Also monitor the mirror region (header + 0x1000) - but only log unique accesses
    local mirrorAddr = headerBaseAddr + 0x1000
    local mirrorLog = {}
    bp = PCSX.addBreakpoint(mirrorAddr, 'Read', 0x1000, "BLB Header Mirror Read",
        function(address, width, cause)
            local regs = PCSX.getRegisters()
            local pc = regs.pc
            local mirrorOffset = address - mirrorAddr
            local funcName = getFunctionName(pc)
            local key = string.format("%s:mirror+0x%03X", funcName, mirrorOffset)
            
            logCount = logCount + 1
            
            if not mirrorLog[key] then
                mirrorLog[key] = { count = 0 }
            end
            mirrorLog[key].count = mirrorLog[key].count + 1
            
            -- Only log first occurrence from known functions
            if mirrorLog[key].count == 1 and not suppressUnknownFunctions then
                print(string.format("[%04d] READ mirror+0x%03X (%d bytes) by %s",
                    logCount, mirrorOffset, width, funcName))
            end
        end)
    table.insert(breakpoints, bp)
    print("  Set READ breakpoint on mirror region: " .. hex(mirrorAddr) .. " - " .. hex(mirrorAddr + 0xFFF))
    print("  (Mirror reads are counted but not logged to reduce noise)")
end

-- ============================================
-- Summary command
-- ============================================

function printSummary()
    print("\n======================================")
    print("BLB Header Access Summary")
    print("======================================")
    print(string.format("Total accesses logged: %d", logCount))
    
    -- Count unique pairs, separate known vs unknown
    local knownCount = 0
    local unknownCount = 0
    local sortedKnown = {}
    local sortedUnknown = {}
    
    for key, data in pairs(accessLog) do
        if data.isKnown then
            knownCount = knownCount + 1
            table.insert(sortedKnown, { key = key, count = data.count })
        else
            unknownCount = unknownCount + 1
            table.insert(sortedUnknown, { key = key, count = data.count })
        end
    end
    
    -- Sort by count descending
    table.sort(sortedKnown, function(a, b) return a.count > b.count end)
    table.sort(sortedUnknown, function(a, b) return a.count > b.count end)
    
    print(string.format("\nKnown function accesses: %d unique patterns", knownCount))
    for i = 1, math.min(20, #sortedKnown) do
        local entry = sortedKnown[i]
        print(string.format("  %6dx  %s", entry.count, entry.key))
    end
    
    print(string.format("\nUnknown function accesses: %d unique patterns", unknownCount))
    print("Top 10 unknown (sorted by frequency):")
    for i = 1, math.min(10, #sortedUnknown) do
        local entry = sortedUnknown[i]
        print(string.format("  %6dx  %s", entry.count, entry.key))
    end
    print("======================================")
end

-- Toggle verbose mode
function setVerbose(enabled)
    verboseMode = enabled
    print("[INFO] Verbose mode: " .. (enabled and "ON" or "OFF"))
end

-- Toggle unknown function suppression
function showUnknown(enabled)
    suppressUnknownFunctions = not enabled
    print("[INFO] Show unknown functions: " .. (enabled and "ON" or "OFF"))
end

-- ============================================
-- Initialization
-- ============================================

print("===========================================")
print("BLB Header Trace Script")
print("===========================================")
print("This script monitors reads/writes to the BLB header")
print("to identify which functions access which parts.\n")

-- Hook known functions first
hookKnownFunctions()

-- Set up memory breakpoints (delayed slightly to allow game to initialize)
print("\n[INFO] Memory breakpoints will be set up on first LoadBLBHeader call")
print("[INFO] Or call setupHeaderBreakpoints() manually after game loads\n")

-- Auto-setup after LoadBLBHeader completes
-- We'll modify the LoadBLBHeader hook to call setup afterwards
local originalLoadBLBHeaderBP = breakpoints[1]  -- First one we added
if originalLoadBLBHeaderBP then
    originalLoadBLBHeaderBP:remove()
    table.remove(breakpoints, 1)
    
    local setupDone = false
    local bp = PCSX.addBreakpoint(ADDRESSES.LoadBLBHeader + 0xC0, 'Exec', 4, "LoadBLBHeader Return", function()
        if not setupDone then
            print("\n[INFO] LoadBLBHeader completed, setting up header breakpoints...")
            setupHeaderBreakpoints()
            setupDone = true
        end
    end)
    table.insert(breakpoints, bp)
    print("  Will auto-setup header breakpoints after LoadBLBHeader completes")
end

-- Re-add LoadBLBHeader entry hook
local bp = PCSX.addBreakpoint(ADDRESSES.LoadBLBHeader, 'Exec', 4, "LoadBLBHeader Entry", function()
    local regs = PCSX.getRegisters()
    logCount = logCount + 1
    print(string.format("\n[%04d] >>> LoadBLBHeader called", logCount))
    print(string.format("       a0 (GameState*): %s", hex(regs.GPR.n.a0)))
    print(string.format("       RA: %s", hex(regs.GPR.n.ra)))
end)
table.insert(breakpoints, bp)

print("\n===========================================")
print("Commands:")
print("  printSummary()           - Print access summary")
print("  setVerbose(true/false)   - Toggle verbose logging")
print("  showUnknown(true/false)  - Show/hide unknown function accesses")
print("  setupHeaderBreakpoints() - Manually set up header breakpoints")
print("")
print("Current settings:")
print("  Verbose mode: " .. (verboseMode and "ON" or "OFF"))
print("  Show unknown: " .. (suppressUnknownFunctions and "OFF" or "ON"))
print("===========================================\n")
