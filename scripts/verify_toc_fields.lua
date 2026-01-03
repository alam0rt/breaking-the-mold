-- verify_toc_fields.lua
-- Verification script to confirm level metadata fields match TOC entries
-- For PCSX-Redux Lua scripting API
--
-- VERIFICATION TARGETS:
-- =====================
-- 1. Field 0x08 (entry1_offset_lo) == TOC[6].offset in loaded primary data
-- 2. Field 0x0A (unknown_0A) == TOC[2].count in loaded primary data
--
-- METHODOLOGY:
-- ============
-- 1. Set breakpoint after level data is loaded into RAM
-- 2. Read the TOC from the loaded primary data
-- 3. Compare TOC entries with level metadata fields
-- 4. Report match/mismatch
--
-- The primary level data has a TOC structure at the start:
--   - First u16: entry count (usually 3)
--   - Then 12-byte entries: [offset:u16, padding:u16, type:u32, size:u32]
--
-- Based on decompiled LevelDataParser.c, the TOC structure is:
--   Offset 0x00: count (u16)
--   Offset 0x04 + i*12: entry i
--     +0x00: type (u32) - 0x258, 0x259, 0x25A
--     +0x04: size (u32)
--     +0x08: offset (u32) from TOC start
--
-- Wait, let me re-check the raw dumps...

local ffi = require('ffi')

-- ============================================
-- Memory Addresses (PAL version)
-- ============================================

local ADDRESSES = {
    BLB_HEADER_BASE = 0x800AE3E0,
    
    -- func_8007A62C loads level data and parses TOC
    func_8007A62C = 0x8007A62C,
    
    -- Context structure location (GameState + 0x84)
    LEVEL_DATA_CONTEXT = 0x8009DCC4,
    
    -- Buffer where primary data is loaded
    -- This is passed as 2nd arg to func_8007A62C
    -- From trace: 0x800AF3E0
    PRIMARY_DATA_BUFFER = 0x800AF3E0,
}

local LEVEL_ENTRY_SIZE = 0x70

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
-- TOC Parsing
-- ============================================

-- Parse the TOC from loaded primary data
-- Returns table of {offset, count} pairs for each 4-byte slot
local function parsePrimaryTOC(bufferAddr)
    local toc = {}
    
    -- Read first 80 bytes (20 entries × 4 bytes each for offset/count pairs)
    -- Based on our Python analysis, the structure is:
    -- Each 4 bytes: [offset:u16, count:u16]
    for i = 0, 19 do
        local addr = bufferAddr + i * 4
        local offset = read_u16(addr)
        local count = read_u16(addr + 2)
        toc[i] = {offset = offset, count = count}
    end
    
    return toc
end

-- ============================================
-- Verification Logic
-- ============================================

local function verifyFieldsAfterLoad(levelIndex, bufferAddr)
    local headerBase = ADDRESSES.BLB_HEADER_BASE
    local entryBase = headerBase + levelIndex * LEVEL_ENTRY_SIZE
    
    -- Read metadata fields from header
    local entry1_offset_lo = read_u16(entryBase + 0x08)
    local unknown_0A = read_u16(entryBase + 0x0A)
    local level_id = read_string(entryBase + 0x56, 5)
    
    -- Parse TOC from loaded data
    local toc = parsePrimaryTOC(bufferAddr)
    
    print(string.format("\n=== Verification for Level %d: %s ===", levelIndex, level_id))
    print(string.format("Buffer address: 0x%08X", bufferAddr))
    
    -- Dump first 10 TOC entries
    print("\nTOC Entries (offset, count):")
    for i = 0, 9 do
        local entry = toc[i]
        local note = ""
        if i == 2 then
            note = string.format(" <-- Expect count=%d (unknown_0A)", unknown_0A)
        elseif i == 6 then
            note = string.format(" <-- Expect offset=%d (entry1_offset_lo)", entry1_offset_lo)
        end
        print(string.format("  TOC[%d]: offset=%5d, count=%5d%s", 
            i, entry.offset, entry.count, note))
    end
    
    -- Verify claims
    print("\n--- Verification Results ---")
    
    -- Claim 1: entry1_offset_lo == TOC[6].offset
    local toc6_offset = toc[6].offset
    local claim1_pass = (entry1_offset_lo == toc6_offset)
    print(string.format("Field 0x08 (entry1_offset_lo): %d", entry1_offset_lo))
    print(string.format("TOC[6].offset:                 %d", toc6_offset))
    print(string.format("MATCH: %s", claim1_pass and "YES ✓" or "NO ✗"))
    
    -- Claim 2: unknown_0A == TOC[2].count
    local toc2_count = toc[2].count
    local claim2_pass = (unknown_0A == toc2_count)
    print(string.format("\nField 0x0A (unknown_0A): %d", unknown_0A))
    print(string.format("TOC[2].count:            %d", toc2_count))
    print(string.format("MATCH: %s", claim2_pass and "YES ✓" or "NO ✗"))
    
    if claim1_pass and claim2_pass then
        print("\n*** BOTH CLAIMS VERIFIED ***")
    else
        print("\n*** VERIFICATION FAILED ***")
    end
    
    return claim1_pass and claim2_pass
end

-- ============================================
-- Breakpoint Setup
-- ============================================

local verificationPending = false
local pendingLevelIndex = 0

local function setupVerificationBreakpoint()
    -- Set breakpoint at the end of func_8007A62C (after TOC is parsed)
    -- The function returns at approximately 0x8007A880 (need to verify)
    -- For now, let's set it after the return
    
    -- Alternative: Hook after the loadCallback call returns
    -- The function structure from LevelDataParser.c:
    --   if (!(ctx->loadCallback(...) & 0xFF)) return 0;
    --   ctx->tocPtr = buffer;
    --   ...parse TOC...
    --   return 1;
    
    -- We'll set a breakpoint just after tocPtr is stored
    -- Approximately at 0x8007A750 (after ctx->tocPtr = buffer)
    
    local hookAddr = 0x8007A760  -- Adjust based on actual disassembly
    
    print(string.format("\n[SETUP] Setting verification breakpoint at 0x%08X", hookAddr))
    print("        Will verify TOC fields after level data is loaded")
    
    PCSX.addBreakpoint(hookAddr, "Exec", 4, function()
        -- Read the buffer pointer from context
        local ctxAddr = ADDRESSES.LEVEL_DATA_CONTEXT
        local bufferAddr = read_u32(ctxAddr + 0x68)  -- tocPtr offset in context
        
        if bufferAddr == 0 then
            print("[VERIFY] tocPtr is NULL, skipping")
            return false
        end
        
        -- Get current level index
        local headerBase = ADDRESSES.BLB_HEADER_BASE
        local headerOffset = read_u8(ctxAddr + 0x60)
        local levelIndex = read_u8(headerBase + 0xF92 + headerOffset)
        
        print(string.format("\n[VERIFY] Level data loaded for index %d", levelIndex))
        verifyFieldsAfterLoad(levelIndex, bufferAddr)
        
        return false  -- Continue execution
    end)
end

-- Manual verification function (call after level is loaded)
local function manualVerify(levelIndex)
    -- Use the known buffer address
    local bufferAddr = ADDRESSES.PRIMARY_DATA_BUFFER
    
    -- Check if data looks valid (first word should be small count like 3)
    local firstWord = read_u16(bufferAddr)
    if firstWord == 0 or firstWord > 100 then
        print(string.format("Buffer at 0x%08X doesn't look like valid TOC (first word = %d)",
            bufferAddr, firstWord))
        print("Try loading a level first, then call manualVerify(levelIndex)")
        return false
    end
    
    return verifyFieldsAfterLoad(levelIndex, bufferAddr)
end

-- ============================================
-- Main Entry Point
-- ============================================

print("\n" .. string.rep("=", 70))
print("TOC Fields Verification Script")
print(string.rep("=", 70))
print("\nThis script verifies that level metadata fields match TOC entries:")
print("  - Field 0x08 should equal TOC[6].offset")
print("  - Field 0x0A should equal TOC[2].count")
print("\nUsage:")
print("  1. Load the game and wait for menu")
print("  2. Call: setupVerificationBreakpoint()")
print("  3. Start a level in-game")
print("  4. Verification runs automatically when level loads")
print("")
print("Or for manual verification after a level is loaded:")
print("  manualVerify(0)  -- Verify MENU level")
print("  manualVerify(1)  -- Verify PHRO level")
print(string.rep("=", 70))

-- Export functions as globals so they can be called from the console
_G.manualVerify = manualVerify
_G.setupVerificationBreakpoint = setupVerificationBreakpoint
_G.verifyFieldsAfterLoad = verifyFieldsAfterLoad
_G.parsePrimaryTOC = parsePrimaryTOC

-- Also return a table for require() usage
return {
    verify = verifyFieldsAfterLoad,
    manualVerify = manualVerify,
    setupBreakpoint = setupVerificationBreakpoint,
    parseTOC = parsePrimaryTOC,
}
