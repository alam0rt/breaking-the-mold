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
-- Coverage Report Unknown Regions Summary:
-- ========================================
-- 1. unknown_ed0 (0xED0-0xF0F): 64 bytes - 16 u32 values
-- 2. Credits table (0xF10-0xF30): 33 bytes - partially documented  
-- 3. state_unknown_f34_f35 (0xF34-0xF35): 2 bytes
-- 4. state_array (0xF38-0xF90): 89 bytes - state bytes for sliding window
-- 5. state_unknown_f91: 1 byte
-- 6. index_array (0xF93-0xFFF): 109 bytes - index bytes for sliding window
--
-- Level Entry Unknown Fields (per entry, 26 entries):
-- - static_data (+0x04, 8 bytes): Unknown static data
-- - flag (+0x0D, 1 byte): Purpose TBD
-- - unknown_0e (+0x0E, 14 bytes): Large unknown region
-- - unknown_1c (+0x1C, 2 bytes): Before secondary_offset  
-- - unknown_20 (+0x20, 2 bytes): After secondary_offset
-- - dynamic_data_1 (+0x22, 10 bytes): May contain pointers
-- - unknown_2e (+0x2E, 16 bytes): Large unknown region
-- - unknown_3e (+0x3E, 2 bytes): Before tertiary_offset
-- - unknown_42 (+0x42, 2 bytes): After tertiary_offset
-- - dynamic_data_2 (+0x44, 10 bytes): May contain pointers
-- - unknown_50 (+0x50, 6 bytes): Before level_id
--
-- Movie Entry Unknown Fields (per entry, 13 entries):
-- - unknown_00 (+0x00, 2 bytes): Always 0, reserved?
--
-- Sector Entry Unknown Fields (per entry, 32 entries):
-- - unknown_byte (+0x02, 1 byte): 0x0A for loading, 0x63 for game over

local HEADER_REGIONS = {
    -- Level metadata table: 26 entries × 0x70 bytes = 2912 bytes
    { start = 0x000, stop = 0xB5F, name = "LevelMetadata", desc = "26 level entries × 0x70 bytes", known = true },
    -- Movie table: 13 entries × 0x1C bytes = 364 bytes  
    { start = 0xB60, stop = 0xCCB, name = "MovieTable", desc = "13 movie entries × 0x1C bytes", known = true },
    -- Gap between movie and sector table
    { start = 0xCCC, stop = 0xCCF, name = "GapMovieSector", desc = "4 bytes gap (UNKNOWN)", known = false },
    -- Sector/loading screen table: 32 entries × 0x10 bytes = 512 bytes
    { start = 0xCD0, stop = 0xECF, name = "SectorTable", desc = "32 sector entries × 0x10 bytes", known = true },
    -- Unknown u32 array: 16 entries = 64 bytes (UNKNOWN - need to trace!)
    { start = 0xED0, stop = 0xF0F, name = "UnknownArray", desc = "16 u32 values (UNKNOWN)", known = false },
    -- Credits sequence table: 2-3 entries × 0x0C bytes (PARTIALLY UNKNOWN)
    { start = 0xF10, stop = 0xF30, name = "CreditsTable", desc = "Credits entries × 0x0C (UNKNOWN)", known = false },
    -- Count fields
    { start = 0xF31, stop = 0xF31, name = "LevelCount", desc = "Number of levels (u8)", known = true },
    { start = 0xF32, stop = 0xF32, name = "AssetCount", desc = "Number of movies (u8)", known = true },
    { start = 0xF33, stop = 0xF33, name = "SectorTableCount", desc = "Number of sector table entries (u8)", known = true },
    -- State/config region (MOSTLY UNKNOWN - key tracing target!)
    { start = 0xF34, stop = 0xF35, name = "UnknownF34", desc = "2 bytes (UNKNOWN)", known = false },
    { start = 0xF36, stop = 0xF36, name = "GameMode", desc = "1=movie, 2=credits, 4-5=level", known = true },
    { start = 0xF37, stop = 0xF37, name = "SecondaryFlag", desc = "Secondary state flag", known = true },
    { start = 0xF38, stop = 0xF90, name = "StateArray", desc = "89 bytes, sliding window states (UNKNOWN)", known = false },
    { start = 0xF91, stop = 0xF91, name = "UnknownF91", desc = "1 byte (UNKNOWN)", known = false },
    { start = 0xF92, stop = 0xF92, name = "AssetIndex", desc = "Current table entry index (u8)", known = true },
    { start = 0xF93, stop = 0xFFF, name = "IndexArray", desc = "109 bytes, sliding window indices (UNKNOWN)", known = false },
}

-- Known functions that access the header (for reverse lookup)
-- Updated from symbol_addrs.txt with correct names
local KNOWN_FUNCTIONS = {
    [0x8007AE14] = "GetMovieUnknown00",
    [0x8007AE58] = "GetMovieSectorCount",
    [0x8007AA08] = "getLevelName",
    [0x800208B0] = "LoadBLBHeader",
    [0x8007A62C] = "LevelDataParser",
    [0x8007A9B0] = "GetLevelCount",
    [0x8007A9C4] = "GetLevelAssetIndex",
    [0x8007A9E8] = "GetLevelDisplayName",
    [0x8007ABCC] = "GetCurrentSectorOffset",
    [0x8007AC54] = "GetCurrentSectorCount",
    [0x8007ACDC] = "GetAssetCount",
    [0x8007ACF0] = "GetMovieReservedByIndex",
    [0x8007AD10] = "GetMovieShortNameByIndex",
    [0x8007AD30] = "GetCurrentMovieReserved",
    [0x8007AD7C] = "GetCurrentMovieShortName",
    [0x8007ADC8] = "GetCurrentMovieFilename",
    [0x8007B074] = "func_8007B074", -- Called from LevelDataParser
    [0x80038BA0] = "CdBLB_ReadSectors",
    [0x80038C14] = "CdBLB_WaitReadComplete",
}

-- Track accesses to unknown regions specifically
local unknownAccessLog = {}  -- {offset -> {functions = {}, count = 0, firstValue = nil}}

-- Feature flag: Set to false to disable memory breakpoints (can cause crashes/slowdowns)
local ENABLE_MEMORY_BREAKPOINTS = false

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
        local isUnknown = false
        
        -- Known fields
        if fieldOffset == 0x00 then fieldName = "sector_offset (u16)"
        elseif fieldOffset == 0x02 then fieldName = "sector_count (u16)"
        -- Unknown: static_data (+0x04, 8 bytes)
        elseif fieldOffset >= 0x04 and fieldOffset < 0x0C then 
            fieldName = string.format("UNKNOWN static_data[%d] (8 bytes)", fieldOffset - 0x04)
            isUnknown = true
        elseif fieldOffset == 0x0C then fieldName = "level_index (u8)"
        -- Unknown: flag (+0x0D, 1 byte)
        elseif fieldOffset == 0x0D then 
            fieldName = "UNKNOWN flag (u8)"
            isUnknown = true
        -- Unknown: unknown_0e (+0x0E, 14 bytes)
        elseif fieldOffset >= 0x0E and fieldOffset < 0x1C then
            fieldName = string.format("UNKNOWN unknown_0e[%d] (14 bytes)", fieldOffset - 0x0E)
            isUnknown = true
        -- Unknown: unknown_1c (+0x1C, 2 bytes)
        elseif fieldOffset >= 0x1C and fieldOffset < 0x1E then
            fieldName = string.format("UNKNOWN unknown_1c[%d] (2 bytes)", fieldOffset - 0x1C)
            isUnknown = true
        elseif fieldOffset == 0x1E then fieldName = "secondary_offset (u16)"
        -- Unknown: unknown_20 (+0x20, 2 bytes)
        elseif fieldOffset >= 0x20 and fieldOffset < 0x22 then
            fieldName = string.format("UNKNOWN unknown_20[%d] (2 bytes)", fieldOffset - 0x20)
            isUnknown = true
        -- Unknown: dynamic_data_1 (+0x22, 10 bytes)
        elseif fieldOffset >= 0x22 and fieldOffset < 0x2C then
            fieldName = string.format("UNKNOWN dynamic_data_1[%d] (10 bytes)", fieldOffset - 0x22)
            isUnknown = true
        elseif fieldOffset == 0x2C then fieldName = "secondary_count (u16)"
        -- Unknown: unknown_2e (+0x2E, 16 bytes)
        elseif fieldOffset >= 0x2E and fieldOffset < 0x3E then
            fieldName = string.format("UNKNOWN unknown_2e[%d] (16 bytes)", fieldOffset - 0x2E)
            isUnknown = true
        -- Unknown: unknown_3e (+0x3E, 2 bytes)
        elseif fieldOffset >= 0x3E and fieldOffset < 0x40 then
            fieldName = string.format("UNKNOWN unknown_3e[%d] (2 bytes)", fieldOffset - 0x3E)
            isUnknown = true
        elseif fieldOffset == 0x40 then fieldName = "tertiary_offset (u16)"
        -- Unknown: unknown_42 (+0x42, 2 bytes)
        elseif fieldOffset >= 0x42 and fieldOffset < 0x44 then
            fieldName = string.format("UNKNOWN unknown_42[%d] (2 bytes)", fieldOffset - 0x42)
            isUnknown = true
        -- Unknown: dynamic_data_2 (+0x44, 10 bytes)
        elseif fieldOffset >= 0x44 and fieldOffset < 0x4E then
            fieldName = string.format("UNKNOWN dynamic_data_2[%d] (10 bytes)", fieldOffset - 0x44)
            isUnknown = true
        elseif fieldOffset == 0x4E then fieldName = "tertiary_count (u16)"
        -- Unknown: unknown_50 (+0x50, 6 bytes)
        elseif fieldOffset >= 0x50 and fieldOffset < 0x56 then
            fieldName = string.format("UNKNOWN unknown_50[%d] (6 bytes)", fieldOffset - 0x50)
            isUnknown = true
        elseif fieldOffset >= 0x56 and fieldOffset < 0x5B then fieldName = "level_id (5 bytes)"
        elseif fieldOffset >= 0x5B and fieldOffset < 0x70 then fieldName = "level_name (21 bytes)"
        else fieldName = string.format("UNKNOWN +0x%02X", fieldOffset)
        end
        
        local result = string.format("Level[%d].%s", entryIndex, fieldName)
        return result, isUnknown
    end
    
    -- Movie entry fields (0x1C bytes each, starting at 0xB60)
    if headerOffset >= 0xB60 and headerOffset < 0xCD0 then
        local entryIndex = math.floor((headerOffset - 0xB60) / 0x1C)
        local fieldOffset = (headerOffset - 0xB60) % 0x1C
        local fieldName = ""
        local isUnknown = false
        
        -- Unknown: unknown_00 (+0x00, 2 bytes) - always 0, reserved?
        if fieldOffset >= 0x00 and fieldOffset < 0x02 then 
            fieldName = string.format("UNKNOWN reserved[%d] (always 0)", fieldOffset)
            isUnknown = true
        elseif fieldOffset >= 0x02 and fieldOffset < 0x04 then fieldName = "sector_count (u16)"
        elseif fieldOffset >= 0x04 and fieldOffset < 0x09 then fieldName = "movie_id (5 bytes)"
        elseif fieldOffset >= 0x09 and fieldOffset < 0x0C then fieldName = "short_name (3 bytes)"
        elseif fieldOffset >= 0x0C and fieldOffset < 0x1C then fieldName = "filename (16 bytes)"
        else fieldName = string.format("UNKNOWN +0x%02X", fieldOffset)
        end
        
        local result = string.format("Movie[%d].%s", entryIndex, fieldName)
        return result, isUnknown
    end
    
    -- Gap between movie table and sector table (0xCCC-0xCCF, 4 bytes)
    if headerOffset >= 0xCCC and headerOffset < 0xCD0 then
        local offset = headerOffset - 0xCCC
        return string.format("UNKNOWN gap_movie_sector[%d] (4 bytes)", offset), true
    end
    
    -- Sector table entry fields (0x10 bytes each, starting at 0xCD0)
    if headerOffset >= 0xCD0 and headerOffset < 0xED0 then
        local entryIndex = math.floor((headerOffset - 0xCD0) / 0x10)
        local fieldOffset = (headerOffset - 0xCD0) % 0x10
        local fieldName = ""
        local isUnknown = false
        
        if fieldOffset == 0x00 then fieldName = "level_index (u8)"
        elseif fieldOffset == 0x01 then fieldName = "entry_flags (u8)"
        -- Unknown: unknown_byte (+0x02, 1 byte)
        elseif fieldOffset == 0x02 then 
            fieldName = "UNKNOWN unknown_byte (0x0A=loading, 0x63=gameover)"
            isUnknown = true
        elseif fieldOffset >= 0x03 and fieldOffset < 0x08 then fieldName = "code (5 bytes)"
        elseif fieldOffset >= 0x08 and fieldOffset < 0x0C then fieldName = "short_name (4 bytes)"
        elseif fieldOffset >= 0x0C and fieldOffset < 0x0E then fieldName = "sector_offset (u16)"
        elseif fieldOffset >= 0x0E and fieldOffset < 0x10 then fieldName = "sector_count (u16)"
        else 
            fieldName = string.format("UNKNOWN +0x%02X", fieldOffset)
            isUnknown = true
        end
        
        local result = string.format("Sector[%d].%s", entryIndex, fieldName)
        return result, isUnknown
    end
    
    -- Unknown u32 array (0xED0-0xF0F, 64 bytes = 16 u32s) - KEY UNKNOWN!
    if headerOffset >= 0xED0 and headerOffset < 0xF10 then
        local entryIndex = math.floor((headerOffset - 0xED0) / 4)
        local byteOffset = (headerOffset - 0xED0) % 4
        return string.format("UNKNOWN UnknownArray[%d]+%d (16 u32s, per-world?)", entryIndex, byteOffset), true
    end
    
    -- Credits entry fields (0x0C bytes each, starting at 0xF10) - KEY UNKNOWN!
    if headerOffset >= 0xF10 and headerOffset < 0xF31 then
        local entryIndex = math.floor((headerOffset - 0xF10) / 0x0C)
        local fieldOffset = (headerOffset - 0xF10) % 0x0C
        local fieldName = ""
        
        -- All credits fields are UNKNOWN - need to trace their usage!
        if fieldOffset >= 0x00 and fieldOffset < 0x04 then 
            fieldName = string.format("UNKNOWN code[%d] (4 bytes)", fieldOffset)
        elseif fieldOffset >= 0x04 and fieldOffset < 0x08 then 
            fieldName = string.format("UNKNOWN padding[%d] (4 bytes)", fieldOffset - 0x04)
        elseif fieldOffset >= 0x08 and fieldOffset < 0x0A then 
            fieldName = string.format("UNKNOWN param_a+%d (u16)", fieldOffset - 0x08)
        elseif fieldOffset >= 0x0A and fieldOffset < 0x0C then 
            fieldName = string.format("UNKNOWN param_b+%d (u16)", fieldOffset - 0x0A)
        else 
            fieldName = string.format("UNKNOWN +0x%02X", fieldOffset)
        end
        
        return string.format("Credits[%d].%s", entryIndex, fieldName), true
    end
    
    -- State/config region (0xF34-0xFFF) - KEY UNKNOWN AREA!
    if headerOffset >= 0xF34 and headerOffset < 0x1000 then
        local stateOffset = headerOffset - 0xF34
        local fieldName = ""
        local isUnknown = false
        
        if stateOffset >= 0x00 and stateOffset < 0x02 then
            -- 0xF34-0xF35: Unknown 2 bytes
            fieldName = string.format("UNKNOWN f34[%d] (2 bytes)", stateOffset)
            isUnknown = true
        elseif stateOffset == 0x02 then
            -- 0xF36: Game mode
            fieldName = "GameMode (1=movie, 2=credits, 3-5=level)"
        elseif stateOffset == 0x03 then
            -- 0xF37: Secondary flag  
            fieldName = "SecondaryFlag"
        elseif stateOffset >= 0x04 and stateOffset < 0x5D then
            -- 0xF38-0xF90: State array (89 bytes) - sliding window states
            local idx = stateOffset - 0x04
            fieldName = string.format("UNKNOWN StateArray[%d] (slot %d state byte)", idx, idx)
            isUnknown = true
        elseif stateOffset == 0x5D then
            -- 0xF91: Unknown
            fieldName = "UNKNOWN f91 (u8)"
            isUnknown = true
        elseif stateOffset == 0x5E then
            -- 0xF92: Asset index (base)
            fieldName = "AssetIndex (base index)"
        elseif stateOffset >= 0x5F and stateOffset < 0xCC then
            -- 0xF93-0xFFF: Index array (109 bytes) - sliding window indices
            local idx = stateOffset - 0x5E  -- 0 = 0xF92 (base), 1 = 0xF93, etc.
            fieldName = string.format("UNKNOWN IndexArray[%d] (slot %d index byte)", idx, idx)
            isUnknown = true
        else
            fieldName = string.format("UNKNOWN state+0x%02X", stateOffset)
            isUnknown = true
        end
        
        return string.format("State.%s (0x%03X)", fieldName, headerOffset), isUnknown
    end
    
    return nil, false
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
        { start = 0x8007A9B0, size = 0x14, name = "GetLevelCount" },
        { start = 0x8007A9C4, size = 0x24, name = "GetLevelAssetIndex" },
        { start = 0x8007A9E8, size = 0x20, name = "GetLevelDisplayName" },
        { start = 0x8007ABCC, size = 0x88, name = "GetCurrentSectorOffset" },
        { start = 0x8007AC54, size = 0x88, name = "GetCurrentSectorCount" },
        { start = 0x8007ACDC, size = 0x14, name = "GetAssetCount" },
        { start = 0x8007ACF0, size = 0x20, name = "GetMovieReservedByIndex" },
        { start = 0x8007AD10, size = 0x20, name = "GetMovieShortNameByIndex" },
        { start = 0x8007AD30, size = 0x4C, name = "GetCurrentMovieReserved" },
        { start = 0x8007AD7C, size = 0x4C, name = "GetCurrentMovieShortName" },
        { start = 0x8007ADC8, size = 0x4C, name = "GetCurrentMovieFilename" },
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

-- Read memory value at given address with specified width
local function readMemValue(addr, width)
    local mem = PCSX.getMemPtr()
    local offset = bit.band(addr, 0x1FFFFF)
    if width == 1 then
        return mem[offset]
    elseif width == 2 then
        return mem[offset] + mem[offset + 1] * 0x100
    elseif width == 4 then
        return mem[offset] + mem[offset + 1] * 0x100 + 
               mem[offset + 2] * 0x10000 + mem[offset + 3] * 0x1000000
    end
    return 0
end

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
    
    -- Check if from a known function (using the updated function ranges)
    local isKnownFunction = KNOWN_FUNCTIONS[pc] ~= nil
    local knownFuncRanges = {
        {0x8007AE14, 0x44}, {0x8007AE58, 0x44}, {0x8007AA08, 0x20},
        {0x800208B0, 0xC4}, {0x8007A62C, 0x254}, {0x8007A9B0, 0x14},
        {0x8007A9C4, 0x24}, {0x8007A9E8, 0x20}, {0x8007ABCC, 0x88},
        {0x8007AC54, 0x88}, {0x8007ACDC, 0x14}, {0x8007ACF0, 0x20},
        {0x8007AD10, 0x20}, {0x8007AD30, 0x4C}, {0x8007AD7C, 0x4C},
        {0x8007ADC8, 0x4C}, {0x8007B074, 0x100}, {0x80038BA0, 0x74},
        {0x80038C14, 0x40}
    }
    for _, func in ipairs(knownFuncRanges) do
        if pc >= func[1] and pc < func[1] + func[2] then
            isKnownFunction = true
            break
        end
    end
    
    -- Get field detail and check if it's an unknown field
    local fieldDetail, isUnknownField = getFieldDetail(headerOffset)
    
    -- Always track unknown field accesses, even from unknown functions!
    if isUnknownField then
        local offsetKey = string.format("0x%03X", headerOffset)
        if not unknownAccessLog[offsetKey] then
            unknownAccessLog[offsetKey] = { 
                functions = {}, 
                count = 0, 
                values = {},
                field = fieldDetail
            }
        end
        unknownAccessLog[offsetKey].count = unknownAccessLog[offsetKey].count + 1
        unknownAccessLog[offsetKey].functions[funcName] = 
            (unknownAccessLog[offsetKey].functions[funcName] or 0) + 1
        
        -- Record the value being read (first few unique values)
        local value = readMemValue(address, width)
        local valueKey = string.format("0x%X", value)
        if not unknownAccessLog[offsetKey].values[valueKey] then
            unknownAccessLog[offsetKey].values[valueKey] = 0
        end
        unknownAccessLog[offsetKey].values[valueKey] = 
            unknownAccessLog[offsetKey].values[valueKey] + 1
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
    
    -- Get region name
    local regionName = getRegionName(headerOffset)
    
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
        
        local unknownMarker = ""
        if isUnknownField then
            unknownMarker = " [UNKNOWN]"
        end
        
        print(string.format("[%04d] READ header+0x%03X (%d bytes) by %s%s%s",
            logCount, headerOffset, width, funcName, suffix, unknownMarker))
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
    local fieldDetail, isUnknownField = getFieldDetail(headerOffset)
    
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
    print("\n=== Setting up BLB header breakpoints ===")
    
    -- Discover header address
    headerBaseAddr = discoverHeaderAddress()
    print("Header base address: " .. hex(headerBaseAddr))
    
    if not ENABLE_MEMORY_BREAKPOINTS then
        print("  Memory breakpoints DISABLED (set ENABLE_MEMORY_BREAKPOINTS = true to enable)")
        print("  Using function-only tracing mode for stability")
        return
    end
    
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
-- Dump unknown regions from current header
-- ============================================

function dumpUnknownRegions()
    if not headerBaseAddr then
        print("[ERROR] Header address not set. Call setupHeaderBreakpoints() first.")
        return
    end
    
    local mem = PCSX.getMemPtr()
    local base = bit.band(headerBaseAddr, 0x1FFFFF)
    
    print("\n==============================================")
    print("UNKNOWN REGION HEX DUMP")
    print("==============================================")
    print(string.format("Header base: %s\n", hex(headerBaseAddr)))
    
    -- Unknown u32 array (0xED0-0xF0F)
    print("Unknown Array (0xED0-0xF0F, 16 u32s):")
    for i = 0, 15 do
        local offset = 0xED0 + i * 4
        local val = mem[base + offset] + 
                    mem[base + offset + 1] * 0x100 + 
                    mem[base + offset + 2] * 0x10000 + 
                    mem[base + offset + 3] * 0x1000000
        print(string.format("  [%2d] 0x%03X: 0x%08X (%d)", i, offset, val, val))
    end
    
    -- Credits table (0xF10-0xF30)
    print("\nCredits Table (0xF10-0xF30, ~3 entries of 0x0C bytes):")
    for i = 0, 2 do
        local entryOffset = 0xF10 + i * 0x0C
        if entryOffset >= 0xF31 then break end
        
        -- Read entry bytes
        local bytes = {}
        for j = 0, 0x0B do
            if entryOffset + j < 0xF31 then
                bytes[j] = mem[base + entryOffset + j]
            else
                bytes[j] = 0
            end
        end
        
        -- Format as code (4 bytes), padding (4 bytes), param_a (u16), param_b (u16)
        local code = ""
        for j = 0, 3 do
            if bytes[j] >= 32 and bytes[j] < 127 then
                code = code .. string.char(bytes[j])
            else
                code = code .. "."
            end
        end
        local param_a = bytes[8] + bytes[9] * 0x100
        local param_b = bytes[10] + bytes[11] * 0x100
        
        print(string.format("  [%d] 0x%03X: code='%s' param_a=0x%04X param_b=0x%04X",
            i, entryOffset, code, param_a, param_b))
    end
    
    -- State region summary (0xF34-0xFFF)
    print("\nState Region (0xF34-0xFFF):")
    print(string.format("  0xF34-0xF35 (unknown): 0x%02X 0x%02X", 
        mem[base + 0xF34], mem[base + 0xF35]))
    print(string.format("  0xF36 (GameMode): %d", mem[base + 0xF36]))
    print(string.format("  0xF37 (SecondaryFlag): %d", mem[base + 0xF37]))
    print(string.format("  0xF91 (unknown): 0x%02X", mem[base + 0xF91]))
    print(string.format("  0xF92 (AssetIndex): %d", mem[base + 0xF92]))
    
    -- Print first 16 bytes of state array and index array
    print("\n  State Array (0xF38-0xF47, first 16 bytes):")
    local stateHex = "    "
    for i = 0, 15 do
        stateHex = stateHex .. string.format("%02X ", mem[base + 0xF38 + i])
    end
    print(stateHex)
    
    print("\n  Index Array (0xF93-0xFA2, first 16 bytes):")
    local indexHex = "    "
    for i = 0, 15 do
        indexHex = indexHex .. string.format("%02X ", mem[base + 0xF93 + i])
    end
    print(indexHex)
    
    print("\n==============================================\n")
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

-- Print summary of unknown field accesses
function printUnknownFieldSummary()
    print("\n==============================================")
    print("UNKNOWN FIELD ACCESS SUMMARY")
    print("==============================================")
    print("These are accesses to fields marked as UNKNOWN")
    print("in the BLB header coverage report.\n")
    
    -- Sort by offset
    local sortedOffsets = {}
    for offset, data in pairs(unknownAccessLog) do
        table.insert(sortedOffsets, { offset = offset, data = data })
    end
    table.sort(sortedOffsets, function(a, b) return a.offset < b.offset end)
    
    if #sortedOffsets == 0 then
        print("No unknown fields accessed yet.")
        print("Run the game and play through different scenarios to trigger accesses.")
    else
        print(string.format("Found %d unknown field offsets accessed:\n", #sortedOffsets))
        
        for _, entry in ipairs(sortedOffsets) do
            local offset = entry.offset
            local data = entry.data
            
            print(string.format("  Offset %s (%dx):", offset, data.count))
            print(string.format("    Field: %s", data.field or "Unknown"))
            
            -- List functions that accessed it
            print("    Accessed by:")
            local funcList = {}
            for func, count in pairs(data.functions) do
                table.insert(funcList, { name = func, count = count })
            end
            table.sort(funcList, function(a, b) return a.count > b.count end)
            for i, f in ipairs(funcList) do
                if i <= 5 then  -- Top 5 functions
                    print(string.format("      - %s (%dx)", f.name, f.count))
                end
            end
            
            -- List observed values
            print("    Values observed:")
            local valueList = {}
            for val, count in pairs(data.values) do
                table.insert(valueList, { value = val, count = count })
            end
            table.sort(valueList, function(a, b) return a.count > b.count end)
            for i, v in ipairs(valueList) do
                if i <= 5 then  -- Top 5 values
                    print(string.format("      %s (%dx)", v.value, v.count))
                end
            end
            print("")
        end
    end
    
    print("==============================================")
    print("Key unknown regions to investigate:")
    print("  0xED0-0xF0F: UnknownArray (16 u32s)")
    print("  0xF10-0xF30: Credits table entries")
    print("  0xF34-0xF35: Unknown 2 bytes") 
    print("  0xF38-0xF90: State array (89 bytes)")
    print("  0xF91:       Unknown byte")
    print("  0xF93-0xFFF: Index array (109 bytes)")
    print("")
    print("Level entry unknown fields:")
    print("  +0x04-0x0B: static_data (8 bytes)")
    print("  +0x0D:      flag (1 byte)")
    print("  +0x0E-0x1B: unknown_0e (14 bytes)")
    print("  +0x1C-0x1D: unknown_1c (2 bytes)")
    print("  +0x20-0x21: unknown_20 (2 bytes)")
    print("  +0x22-0x2B: dynamic_data_1 (10 bytes)")
    print("  +0x2E-0x3D: unknown_2e (16 bytes)")
    print("  +0x3E-0x3F: unknown_3e (2 bytes)")
    print("  +0x42-0x43: unknown_42 (2 bytes)")
    print("  +0x44-0x4D: dynamic_data_2 (10 bytes)")
    print("  +0x50-0x55: unknown_50 (6 bytes)")
    print("==============================================\n")
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
print("  printSummary()             - Print general access summary")
print("  printUnknownFieldSummary() - Print UNKNOWN field accesses (key for RE!)")
print("  dumpUnknownRegions()       - Hex dump unknown header regions")
print("  setVerbose(true/false)     - Toggle verbose logging")
print("  showUnknown(true/false)    - Show/hide unknown function accesses")
print("  setupHeaderBreakpoints()   - Manually set up header breakpoints")
print("")
print("Workflow for reverse engineering:")
print("  1. Run game through various scenarios (menu, levels, movies, credits)")
print("  2. Call printSummary() to see function call patterns")
print("  3. Note the offset parameter values to understand slot usage")
print("  4. Use this info to update blb.py field definitions")
print("")
print("Current settings:")
print("  Verbose mode: " .. (verboseMode and "ON" or "OFF"))
print("  Show unknown: " .. (suppressUnknownFunctions and "OFF" or "ON"))
print("  Memory breakpoints: " .. (ENABLE_MEMORY_BREAKPOINTS and "ON" or "OFF (stable mode)"))
print("===========================================\n")
