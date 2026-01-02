-- trace_level_data.lua - Trace level data loading and asset parsing
-- For PCSX-Redux Lua scripting API
--
-- Purpose: Confirm the primary.bin TOC structure and asset parsing
--
-- Key structures to verify:
--   1. LevelDataContext at 0x8009DCC4 (GameState + 0x84)
--      - ctx+0x5C: headerBuffer pointer (0x800AE3E0)
--      - ctx+0x60: stateOffset (sliding window index)
--      - ctx+0x64: loadCallback function pointer
--      - ctx+0x68: tocPtr (pointer to loaded TOC)
--      - ctx+0x6C: dataOffset (offset to data after TOC)
--      - ctx+0x70: asset258 (world graphics pointer)
--      - ctx+0x74: asset259 (collision data pointer)
--      - ctx+0x78: asset259Size
--      - ctx+0x7C: asset25A (palette pointer)
--
--   2. TOC format (at start of primary.bin data):
--      - u16 count at offset 0
--      - Entries at offset 4, each 12 bytes: (type u32, size u32, offset u32)
--
--   3. Asset types: 0x258 (world), 0x259 (collision), 0x25A (palette)

print("===========================================")
print("Level Data Trace Script")
print("Focus: LevelDataParser and asset pointers")
print("===========================================")

-- ============================================
-- CONSTANTS
-- ============================================

-- Known addresses (PAL version)
local LEVEL_DATA_CONTEXT = 0x8009DCC4
local BLB_HEADER_ADDR = 0x800AE3E0

-- LevelDataContext offsets
local CTX_OFFSETS = {
    headerBuffer = 0x5C,
    stateOffset = 0x60,
    loadCallback = 0x64,
    tocPtr = 0x68,
    dataOffset = 0x6C,
    asset258 = 0x70,
    asset259 = 0x74,
    asset259Size = 0x78,
    asset25A = 0x7C,
}

-- Header offsets for level table lookups
local HEADER_OFFSETS = {
    levelTable = 0x000,      -- Level metadata table (0x70 per entry)
    levelEntrySize = 0x70,
    gameModeBase = 0xF36,
    assetIndexBase = 0xF92,
    levelCount = 0xF31,
}

-- Asset types
local ASSET_TYPES = {
    [0x258] = "World/Graphics",
    [0x259] = "Collision/Physics",
    [0x25A] = "Palette/Colors",
}

-- ============================================
-- HELPER FUNCTIONS
-- ============================================
local breakpoints = {}
local logCount = 0
local callCounts = {}

local function hex(val)
    if val then
        return string.format("0x%08X", val)
    end
    return "nil"
end

local function readU8(addr)
    local mem = PCSX.getMemPtr()
    local physAddr = bit.band(addr, 0x1FFFFF)
    return mem[physAddr]
end

local function readU16(addr)
    local mem = PCSX.getMemPtr()
    local physAddr = bit.band(addr, 0x1FFFFF)
    return mem[physAddr] + mem[physAddr + 1] * 0x100
end

local function readU32(addr)
    local mem = PCSX.getMemPtr()
    local physAddr = bit.band(addr, 0x1FFFFF)
    return mem[physAddr] + mem[physAddr + 1] * 0x100 + 
           mem[physAddr + 2] * 0x10000 + mem[physAddr + 3] * 0x1000000
end

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

-- Read and display LevelDataContext structure
local function dumpLevelDataContext(ctxAddr)
    print("  LevelDataContext dump:")
    print(string.format("    headerBuffer (0x5C): %s", hex(readU32(ctxAddr + CTX_OFFSETS.headerBuffer))))
    print(string.format("    stateOffset  (0x60): %d", readU8(ctxAddr + CTX_OFFSETS.stateOffset)))
    print(string.format("    loadCallback (0x64): %s", hex(readU32(ctxAddr + CTX_OFFSETS.loadCallback))))
    print(string.format("    tocPtr       (0x68): %s", hex(readU32(ctxAddr + CTX_OFFSETS.tocPtr))))
    print(string.format("    dataOffset   (0x6C): %s", hex(readU32(ctxAddr + CTX_OFFSETS.dataOffset))))
    print(string.format("    asset258     (0x70): %s [World/Graphics]", hex(readU32(ctxAddr + CTX_OFFSETS.asset258))))
    print(string.format("    asset259     (0x74): %s [Collision]", hex(readU32(ctxAddr + CTX_OFFSETS.asset259))))
    print(string.format("    asset259Size (0x78): %d bytes", readU32(ctxAddr + CTX_OFFSETS.asset259Size)))
    print(string.format("    asset25A     (0x7C): %s [Palette]", hex(readU32(ctxAddr + CTX_OFFSETS.asset25A))))
end

-- Parse and display TOC structure
local function dumpTOC(tocAddr)
    local count = readU16(tocAddr)
    print(string.format("  TOC at %s:", hex(tocAddr)))
    print(string.format("    Entry count: %d", count))
    
    for i = 0, math.min(count - 1, 9) do  -- Max 10 entries
        local entryBase = tocAddr + 4 + (i * 12)
        local assetType = readU32(entryBase)
        local assetSize = readU32(entryBase + 4)
        local assetOffset = readU32(entryBase + 8)
        local typeName = ASSET_TYPES[assetType] or "Unknown"
        
        print(string.format("    [%d] Type=0x%03X (%s), Size=%d, Offset=0x%X",
            i, assetType, typeName, assetSize, assetOffset))
        
        -- For known types, show the actual pointer
        if assetType == 0x258 or assetType == 0x259 or assetType == 0x25A then
            local dataPtr = tocAddr + assetOffset
            print(string.format("        -> Data at %s", hex(dataPtr)))
            
            -- Peek at first 16 bytes of data
            local peek = ""
            for j = 0, 15 do
                peek = peek .. string.format("%02X ", readU8(dataPtr + j))
            end
            print(string.format("        -> First 16 bytes: %s", peek))
        end
    end
end

-- Read level metadata from header
local function dumpLevelMetadata(headerAddr, levelIndex)
    local entryBase = headerAddr + (levelIndex * HEADER_OFFSETS.levelEntrySize)
    
    local sectorOffset = readU16(entryBase + 0x00)
    local sectorCount = readU16(entryBase + 0x02)
    local tocOffset = readU32(entryBase + 0x08)
    local levelId = readString(entryBase + 0x56, 5)
    local levelName = readString(entryBase + 0x5B, 21)
    
    print(string.format("  Level[%d] Metadata:", levelIndex))
    print(string.format("    ID: '%s', Name: '%s'", levelId, levelName))
    print(string.format("    Sector: offset=%d, count=%d", sectorOffset, sectorCount))
    print(string.format("    TOC offset within data: 0x%X", tocOffset))
end

-- ============================================
-- TRACE FUNCTIONS
-- ============================================

local TRACE_FUNCTIONS = {
    -- Main level data parser
    {
        name = "LevelDataParser",
        addr = 0x8007A62C,
        onEntry = function(regs)
            local ctxAddr = regs.GPR.n.a0
            local bufferAddr = regs.GPR.n.a1
            
            print("\n>>> LevelDataParser ENTRY")
            print(string.format("  ctx=%s, buffer=%s", hex(ctxAddr), hex(bufferAddr)))
            
            -- Read game mode and level index from header
            local headerPtr = readU32(ctxAddr + CTX_OFFSETS.headerBuffer)
            local stateOffset = readU8(ctxAddr + CTX_OFFSETS.stateOffset)
            local gameMode = readU8(headerPtr + HEADER_OFFSETS.gameModeBase + stateOffset)
            local levelIndex = readU8(headerPtr + HEADER_OFFSETS.assetIndexBase + stateOffset)
            
            print(string.format("  headerPtr=%s, stateOffset=%d", hex(headerPtr), stateOffset))
            print(string.format("  gameMode=%d, levelIndex=%d", gameMode, levelIndex))
            
            if gameMode == 3 then
                dumpLevelMetadata(headerPtr, levelIndex)
            end
        end,
        onExit = function(regs)
            local ctxAddr = LEVEL_DATA_CONTEXT  -- Use known address
            local result = regs.GPR.n.v0
            
            print(string.format("\n<<< LevelDataParser EXIT (result=%d)", result))
            
            if result ~= 0 then
                dumpLevelDataContext(ctxAddr)
                
                -- Dump TOC if loaded
                local tocPtr = readU32(ctxAddr + CTX_OFFSETS.tocPtr)
                if tocPtr ~= 0 then
                    dumpTOC(tocPtr)
                end
            end
        end
    },
    
    -- LoadBLBHeader
    {
        name = "LoadBLBHeader",
        addr = 0x800208B0,
        onEntry = function(regs)
            print("\n>>> LoadBLBHeader")
            print(string.format("  gameState=%s", hex(regs.GPR.n.a0)))
        end
    },
    
    -- CD sector read (to see what data is loaded)
    {
        name = "CdBLB_ReadSectors",
        addr = 0x80038BA0,
        onEntry = function(regs)
            local sectorOffset = regs.GPR.n.a0
            local sectorCount = regs.GPR.n.a1
            local destBuffer = regs.GPR.n.a2
            
            print(string.format("\n>>> CdBLB_ReadSectors: offset=%d, count=%d, dest=%s",
                sectorOffset, sectorCount, hex(destBuffer)))
        end
    },
    
    -- Level count accessor
    {
        name = "GetLevelCount",
        addr = 0x8007A9B0,
        onEntry = function(regs)
            print(string.format("\n>>> GetLevelCount(ctx=%s)", hex(regs.GPR.n.a0)))
        end,
        onExit = function(regs)
            print(string.format("    result=%d", regs.GPR.n.v0))
        end
    },
    
    -- Level name accessor
    {
        name = "getLevelName",
        addr = 0x8007AA08,
        onEntry = function(regs)
            print(string.format("\n>>> getLevelName(ctx=%s, idx=%d)", 
                hex(regs.GPR.n.a0), regs.GPR.n.a1))
        end,
        onExit = function(regs)
            local namePtr = regs.GPR.n.v0
            local name = readString(namePtr, 24)
            print(string.format("    result=%s -> \"%s\"", hex(namePtr), name))
        end
    },
}

-- ============================================
-- BREAKPOINT SETUP
-- ============================================

-- Create entry breakpoint callback
local function createEntryCallback(funcDef)
    return function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        callCounts[funcDef.name] = (callCounts[funcDef.name] or 0) + 1
        
        if funcDef.onEntry then
            funcDef.onEntry(regs)
        else
            print(string.format("[%04d] %s() RA=%s", 
                logCount, funcDef.name, hex(regs.GPR.n.ra)))
        end
        
        -- If there's an exit callback, set up return breakpoint
        if funcDef.onExit then
            local raAddr = regs.GPR.n.ra
            local exitBp
            exitBp = PCSX.addBreakpoint(raAddr, 'Exec', 4, funcDef.name .. "_exit", function()
                local exitRegs = PCSX.getRegisters()
                funcDef.onExit(exitRegs)
                exitBp:disable()
            end)
        end
    end
end

-- Set up breakpoints
print("\nSetting up function breakpoints...")
for _, funcDef in ipairs(TRACE_FUNCTIONS) do
    local bp = PCSX.addBreakpoint(funcDef.addr, 'Exec', 4, funcDef.name, createEntryCallback(funcDef))
    table.insert(breakpoints, bp)
    print(string.format("  %s @ %s", funcDef.name, hex(funcDef.addr)))
end

-- ============================================
-- INTERACTIVE COMMANDS
-- ============================================

function printSummary()
    print("\n===========================================")
    print("CALL SUMMARY")
    print("===========================================")
    
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

function dumpContext()
    print("\n===========================================")
    print("Current LevelDataContext State")
    print("===========================================")
    dumpLevelDataContext(LEVEL_DATA_CONTEXT)
    
    local tocPtr = readU32(LEVEL_DATA_CONTEXT + CTX_OFFSETS.tocPtr)
    if tocPtr ~= 0 then
        dumpTOC(tocPtr)
    else
        print("  TOC not loaded yet")
    end
end

function dumpHeader()
    print("\n===========================================")
    print("BLB Header Summary")
    print("===========================================")
    
    local levelCount = readU8(BLB_HEADER_ADDR + HEADER_OFFSETS.levelCount)
    print(string.format("  Level count: %d", levelCount))
    
    for i = 0, math.min(levelCount - 1, 5) do
        dumpLevelMetadata(BLB_HEADER_ADDR, i)
    end
end

function clearLog()
    logCount = 0
    callCounts = {}
    print("Log cleared.")
end

print("\n===========================================")
print("Commands:")
print("  printSummary() - Show call counts")
print("  dumpContext()  - Dump current LevelDataContext")
print("  dumpHeader()   - Dump BLB header level info")
print("  clearLog()     - Reset counters")
print("===========================================\n")
