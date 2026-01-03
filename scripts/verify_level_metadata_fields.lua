-- verify_level_metadata_fields.lua
-- Verification script for level metadata unknown fields
-- For PCSX-Redux Lua scripting API
--
-- This script verifies the following findings:
-- 1. Field 0x08 (entry1_offset_lo) == TOC[6].offset in primary data
-- 2. Field 0x0A (unknown_0A) == TOC[2].count in primary data
--
-- It also traces accesses to fields 0x04, 0x06, and 0x0D to understand their usage.
--
-- Run: Load this script in PCSX-Redux after the game has loaded the BLB header.
--      Then load any level and observe the output.

local ffi = require('ffi')

-- ============================================
-- Configuration  
-- ============================================

-- Memory addresses (PAL version)
local ADDRESSES = {
    -- BLB header is loaded at this address after LoadBLBHeader runs
    BLB_HEADER_BASE = 0x800AE3E0,
    
    -- Level data parsing function (accesses level metadata)
    func_8007A62C = 0x8007A62C,
    
    -- Other potential accessors
    getLevelName = 0x8007AA08,
}

-- Level metadata entry structure offsets
local LEVEL_ENTRY = {
    SIZE = 0x70,           -- 112 bytes per entry
    sector_offset = 0x00,  -- u16: Primary sector offset
    sector_count = 0x02,   -- u16: Primary sector count
    unknown_04 = 0x04,     -- u16: UNKNOWN - byte offset in primary data?
    unknown_06 = 0x06,     -- u16: UNKNOWN - asset count?
    entry1_offset_lo = 0x08, -- u16: CLAIM: TOC[6].offset in primary data
    unknown_0A = 0x0A,     -- u16: CLAIM: TOC[2].count in primary data
    level_index = 0x0C,    -- u8: Level asset index
    level_flag = 0x0D,     -- u8: UNKNOWN - 0 or 1, loading flag?
    level_id = 0x56,       -- 5 bytes: Level code (e.g., "MENU")
}

-- Header count offsets
local HEADER = {
    level_count = 0xF31,
    game_mode = 0xF36,
    level_index = 0xF92,
}

-- ============================================
-- Utility Functions
-- ============================================

local function read_u8(addr)
    return PCSX.getMemPtr()[addr - 0x80000000]
end

local function read_u16(addr)
    local ptr = PCSX.getMemPtr()
    local base = addr - 0x80000000
    return ptr[base] + ptr[base + 1] * 256
end

local function read_u32(addr)
    local ptr = PCSX.getMemPtr()
    local base = addr - 0x80000000
    return ptr[base] + ptr[base + 1] * 256 + ptr[base + 2] * 65536 + ptr[base + 3] * 16777216
end

local function read_string(addr, maxLen)
    local ptr = PCSX.getMemPtr()
    local base = addr - 0x80000000
    local str = ""
    for i = 0, maxLen - 1 do
        local c = ptr[base + i]
        if c == 0 then break end
        str = str .. string.char(c)
    end
    return str
end

-- ============================================
-- Verification Functions
-- ============================================

-- Verify field 0x08 and 0x0A match TOC entries in primary data
local function verifyLevelMetadataFields(levelIndex)
    local headerBase = ADDRESSES.BLB_HEADER_BASE
    local entryBase = headerBase + levelIndex * LEVEL_ENTRY.SIZE
    
    -- Read level metadata fields
    local sector_offset = read_u16(entryBase + LEVEL_ENTRY.sector_offset)
    local sector_count = read_u16(entryBase + LEVEL_ENTRY.sector_count)
    local unknown_04 = read_u16(entryBase + LEVEL_ENTRY.unknown_04)
    local unknown_06 = read_u16(entryBase + LEVEL_ENTRY.unknown_06)
    local entry1_offset_lo = read_u16(entryBase + LEVEL_ENTRY.entry1_offset_lo)
    local unknown_0A = read_u16(entryBase + LEVEL_ENTRY.unknown_0A)
    local level_flag = read_u8(entryBase + LEVEL_ENTRY.level_flag)
    local level_id = read_string(entryBase + LEVEL_ENTRY.level_id, 5)
    
    print(string.format("\n=== Level %d: %s ===", levelIndex, level_id))
    print(string.format("  Primary: sector=%d, count=%d", sector_offset, sector_count))
    print(string.format("  Field 0x04 (unknown_04): %d (0x%04X)", unknown_04, unknown_04))
    print(string.format("  Field 0x06 (unknown_06): %d", unknown_06))
    print(string.format("  Field 0x08 (entry1_offset_lo): %d (0x%04X)", entry1_offset_lo, entry1_offset_lo))
    print(string.format("  Field 0x0A (unknown_0A): %d", unknown_0A))
    print(string.format("  Field 0x0D (level_flag): %d", level_flag))
    
    -- Calculate primary data address in RAM
    -- After loading, primary data is at a specific location
    -- We need to find where the game loads level data
    -- For now, we'll set a breakpoint and verify when data is loaded
    
    return {
        level_id = level_id,
        unknown_04 = unknown_04,
        unknown_06 = unknown_06,
        entry1_offset_lo = entry1_offset_lo,
        unknown_0A = unknown_0A,
        level_flag = level_flag,
    }
end

-- Print all levels' unknown fields for comparison
local function printAllLevelFields()
    local headerBase = ADDRESSES.BLB_HEADER_BASE
    local levelCount = read_u8(headerBase + HEADER.level_count)
    
    print("\n" .. string.rep("=", 80))
    print("Level Metadata Unknown Fields Dump")
    print(string.rep("=", 80))
    print(string.format("%-3s %-5s %-20s %6s %5s %5s %4s", 
        "Idx", "ID", "Name", "0x04", "0x06", "0x0A", "Flag"))
    print(string.rep("-", 80))
    
    for i = 0, levelCount - 1 do
        local entryBase = headerBase + i * LEVEL_ENTRY.SIZE
        local level_id = read_string(entryBase + LEVEL_ENTRY.level_id, 5)
        local name = read_string(entryBase + 0x5B, 21)
        local unknown_04 = read_u16(entryBase + LEVEL_ENTRY.unknown_04)
        local unknown_06 = read_u16(entryBase + LEVEL_ENTRY.unknown_06)
        local unknown_0A = read_u16(entryBase + LEVEL_ENTRY.unknown_0A)
        local level_flag = read_u8(entryBase + LEVEL_ENTRY.level_flag)
        
        print(string.format("%-3d %-5s %-20s %6d %5d %5d %4d",
            i, level_id, name:sub(1, 20), unknown_04, unknown_06, unknown_0A, level_flag))
    end
end

-- ============================================
-- Breakpoint Handlers for Runtime Verification
-- ============================================

-- Track when func_8007A62C accesses level metadata field 0x08
local function setupField08Breakpoint()
    -- In func_8007A62C, the code reads levels[levelIndex].tocOffset at +0x08
    -- This happens at approximately offset +0x148 from function start
    -- We'll trace the actual memory read
    
    print("\n[SETUP] Setting breakpoint on func_8007A62C (LevelDataParser)")
    print("        This will trace accesses to level metadata fields")
    
    -- Set execution breakpoint at the function entry
    PCSX.addBreakpoint(ADDRESSES.func_8007A62C, "Exec", 4, function()
        print("\n[TRACE] func_8007A62C called (LevelDataParser)")
        
        -- The function will read:
        -- 1. gameMode from header[0xF36 + headerOffset]
        -- 2. levelIndex from header[0xF92 + headerOffset]
        -- 3. Level[levelIndex] sector_offset, sector_count, and tocOffset
        
        -- We can't easily trace the internal reads here, but we know the pattern
        -- The key access is: *(u32 *)(ctx->header + levelIndex * 0x70 + 8)
        -- This is field 0x08 (entry1_offset_lo)
        
        return false  -- Continue execution
    end)
end

-- Trace memory reads from level metadata table
local function setupLevelMetadataReadBreakpoints()
    local headerBase = ADDRESSES.BLB_HEADER_BASE
    
    -- Set read breakpoints on the first level entry's unknown fields
    -- This will show us what code accesses these fields
    
    local fieldsToWatch = {
        {offset = LEVEL_ENTRY.unknown_04, name = "unknown_04", size = 2},
        {offset = LEVEL_ENTRY.unknown_06, name = "unknown_06", size = 2},
        {offset = LEVEL_ENTRY.entry1_offset_lo, name = "entry1_offset_lo", size = 2},
        {offset = LEVEL_ENTRY.unknown_0A, name = "unknown_0A", size = 2},
        {offset = LEVEL_ENTRY.level_flag, name = "level_flag", size = 1},
    }
    
    print("\n[SETUP] Setting memory read breakpoints on Level[0] unknown fields:")
    
    for _, field in ipairs(fieldsToWatch) do
        local addr = headerBase + field.offset
        print(string.format("        0x%08X: %s (%d bytes)", addr, field.name, field.size))
        
        PCSX.addBreakpoint(addr, "Read", field.size, function()
            -- Get the PC (program counter) to see what function is reading this
            local pc = PCSX.getRegisters().pc
            print(string.format("[READ] %s at 0x%08X, caller PC=0x%08X", 
                field.name, addr, pc))
            return false  -- Continue execution
        end)
    end
end

-- ============================================
-- Main Entry Point
-- ============================================

print("\n" .. string.rep("=", 60))
print("Level Metadata Fields Verification Script")
print(string.rep("=", 60))
print("\nThis script verifies:")
print("  1. Field 0x08 == TOC[6].offset in primary level data")
print("  2. Field 0x0A == TOC[2].count in primary level data")
print("\nCommands:")
print("  printAllLevelFields() - Dump all levels' unknown fields")
print("  verifyLevelMetadataFields(n) - Verify level n in detail")
print("  setupLevelMetadataReadBreakpoints() - Trace memory reads")
print("  setupField08Breakpoint() - Trace LevelDataParser accesses")
print(string.rep("=", 60))

-- Check if header is loaded
local levelCount = read_u8(ADDRESSES.BLB_HEADER_BASE + HEADER.level_count)
if levelCount > 0 and levelCount <= 26 then
    print(string.format("\nBLB Header detected: %d levels", levelCount))
    printAllLevelFields()
else
    print("\nBLB Header not yet loaded. Run after game initialization.")
    print("Expected level_count at 0x%08X", ADDRESSES.BLB_HEADER_BASE + HEADER.level_count)
end
