-- trace_blb.lua - Trace all BLB asset loads from GAME.BLB
-- For Skullmonkeys PAL (SLES_010.90)

local ffi = require('ffi')

-- Version-specific addresses (PAL SLES_010.90)
local BLB_HEADER_BASE = 0x800AE3E0  -- Where BLB header is loaded
local SECTOR_SIZE = 2048            -- PSX CD sector size
local CD_READ_FILE = 0x800863B0     -- CdReadFile function address

-- Storage for breakpoints (prevent garbage collection)
local breakpoints = {}

-- Format address as hex
local function hex(val)
    return string.format("0x%08X", val)
end

-- Format sector as BLB byte offset
local function sectorToOffset(sector)
    return sector * SECTOR_SIZE
end

print("===========================================")
print("BLB Trace Script for Skullmonkeys")
print("===========================================")
print("BLB Header Base: " .. hex(BLB_HEADER_BASE))
print("CdReadFile addr: " .. hex(CD_READ_FILE))
print("")

-- Log counter
local logCount = 0

-- Hook CdReadFile(offset, sectors, buffer)
-- MIPS calling convention: a0=offset, a1=sectors, a2=buffer
local cdReadBP = PCSX.addBreakpoint(CD_READ_FILE, 'Exec', 4, "CdReadFile", function()
    local regs = PCSX.getRegisters()
    local offset = regs.GPR.n.a0       -- Sector offset in BLB
    local sectors = regs.GPR.n.a1      -- Number of sectors to read
    local buffer = regs.GPR.n.a2       -- Destination buffer address
    local caller = regs.GPR.n.ra       -- Return address (caller)
    
    local byteOffset = sectorToOffset(offset)
    local byteSize = sectorToOffset(sectors)
    
    logCount = logCount + 1
    print(string.format("[%04d] CdReadFile: sector=%d (0x%X), count=%d, size=0x%X, dest=%s, caller=%s",
        logCount, offset, byteOffset, sectors, byteSize, hex(buffer), hex(caller)))
end)
table.insert(breakpoints, cdReadBP)
print("Hooked CdReadFile @ " .. hex(CD_READ_FILE))

-- Also monitor the state variable at 0x8009DD80
local stateAddr = 0x8009DD80
local stateBP = PCSX.addBreakpoint(stateAddr, 'Write', 1, "GameState write", function(address)
    local mem = PCSX.getMemPtr()
    local offset = bit.band(address, 0x1FFFFF)
    local newValue = mem[offset]
    print(string.format("[STATE] Game state changed to: 0x%02X @ PC=%s", 
        newValue, hex(PCSX.getRegisters().pc)))
end)
table.insert(breakpoints, stateBP)
print("Monitoring GameState @ " .. hex(stateAddr))

print("")
print("Tracing active! Play the game to see BLB reads.")
print("===========================================")
