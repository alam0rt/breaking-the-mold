-- trace_blb.lua - Trace CD reads and BLB asset loads
-- Works by hooking BIOS syscalls and scanning for PSY-Q patterns

local ffi = require('ffi')

-- Storage for breakpoints (prevent garbage collection)
local breakpoints = {}
local logCount = 0

-- Format helpers
local function hex(val)
    if val < 0 then val = val + 0x100000000 end
    return string.format("0x%08X", val)
end

local function hex16(val)
    return string.format("0x%04X", bit.band(val, 0xFFFF))
end

print("===========================================")
print("PSX CD/BLB Trace Script")
print("===========================================")

-- Read memory helper
local function readU32(addr)
    local mem = PCSX.getMemPtr()
    local offset = bit.band(addr, 0x1FFFFF)
    return mem[offset] + 
           mem[offset + 1] * 0x100 + 
           mem[offset + 2] * 0x10000 + 
           mem[offset + 3] * 0x1000000
end

-- ============================================
-- Method 1: Hook BIOS A0/B0/C0 syscall tables
-- ============================================
-- PSX BIOS uses jump tables at fixed addresses
-- A0 table:ings like file I/O
-- B0 table: includes CD functions
-- C0 table: more system functions

-- BIOS entry points (called via syscall instructions)
local BIOS_A0_ENTRY = 0x000000A0  -- A0 function dispatcher
local BIOS_B0_ENTRY = 0x000000B0  -- B0 function dispatcher  
local BIOS_C0_ENTRY = 0x000000C0  -- C0 function dispatcher

-- Hook A0 syscalls (file operations)
local a0BP = PCSX.addBreakpoint(BIOS_A0_ENTRY, 'Exec', 4, "BIOS A0", function()
    local regs = PCSX.getRegisters()
    local func = bit.band(regs.GPR.n.t1, 0xFF)  -- Function number in t1
    
    -- Interesting A0 functions
    local a0_names = {
        [0x00] = "FileOpen",
        [0x01] = "FileSeek", 
        [0x02] = "FileRead",
        [0x03] = "FileWrite",
        [0x04] = "FileClose",
        [0x42] = "firstfile",
        [0x43] = "nextfile",
        [0x44] = "rename",
        [0x45] = "delete",
    }
    
    if a0_names[func] then
        logCount = logCount + 1
        print(string.format("[%04d] A0(%02X) %s: a0=%s a1=%s a2=%s ra=%s",
            logCount, func, a0_names[func],
            hex(regs.GPR.n.a0), hex(regs.GPR.n.a1), 
            hex(regs.GPR.n.a2), hex(regs.GPR.n.ra)))
    end
end)
table.insert(breakpoints, a0BP)
print("Hooked BIOS A0 @ " .. hex(BIOS_A0_ENTRY))

-- Hook B0 syscalls (includes some CD functions)
local b0BP = PCSX.addBreakpoint(BIOS_B0_ENTRY, 'Exec', 4, "BIOS B0", function()
    local regs = PCSX.getRegisters()
    local func = bit.band(regs.GPR.n.t1, 0xFF)
    
    local b0_names = {
        [0x32] = "FileOpen",
        [0x33] = "FileSeek",
        [0x34] = "FileRead", 
        [0x35] = "FileWrite",
        [0x36] = "FileClose",
        [0x40] = "cd_open",
        [0x42] = "firstfile",
        [0x43] = "nextfile",
        [0x44] = "cd_rename",
        [0x45] = "cd_delete",
        [0x4A] = "GetSystemInfo",
        [0x54] = "_96_init",
        [0x56] = "_96_remove",
    }
    
    if b0_names[func] then
        logCount = logCount + 1
        print(string.format("[%04d] B0(%02X) %s: a0=%s a1=%s a2=%s ra=%s",
            logCount, func, b0_names[func],
            hex(regs.GPR.n.a0), hex(regs.GPR.n.a1),
            hex(regs.GPR.n.a2), hex(regs.GPR.n.ra)))
    end
end)
table.insert(breakpoints, b0BP)
print("Hooked BIOS B0 @ " .. hex(BIOS_B0_ENTRY))

-- ============================================
-- Method 2: Hook CD-ROM hardware registers
-- ============================================
-- The CD controller is accessed via memory-mapped I/O
-- Watching writes to CD registers catches ALL CD access

local CDROM_REG0 = 0x1F801800  -- CD status/index register
local CDROM_REG1 = 0x1F801801  -- CD command register
local CDROM_REG2 = 0x1F801802  -- CD parameter/data
local CDROM_REG3 = 0x1F801803  -- CD request/interrupt

-- Hook writes to CD command register
local cdCmdBP = PCSX.addBreakpoint(CDROM_REG1, 'Write', 1, "CD Command", function(address, width)
    local regs = PCSX.getRegisters()
    local mem = PCSX.getMemPtr()
    
    -- Read the command being written (it's in the register being stored)
    -- For SW instruction, value is in rt register
    local cmd = bit.band(regs.GPR.n.v0, 0xFF)  -- Often stored from v0
    
    local cd_cmds = {
        [0x01] = "Nop",
        [0x02] = "Setloc",
        [0x03] = "Play",
        [0x04] = "Forward",
        [0x05] = "Backward", 
        [0x06] = "ReadN",
        [0x07] = "MotorOn",
        [0x08] = "Stop",
        [0x09] = "Pause",
        [0x0A] = "Init",
        [0x0B] = "Mute",
        [0x0C] = "Demute",
        [0x0D] = "Setfilter",
        [0x0E] = "Setmode",
        [0x0F] = "Getparam",
        [0x10] = "GetlocL",
        [0x11] = "GetlocP",
        [0x12] = "SetSession",
        [0x13] = "GetTN",
        [0x14] = "GetTD",
        [0x15] = "SeekL",
        [0x16] = "SeekP",
        [0x1A] = "GetID",
        [0x1B] = "ReadS",
        [0x1E] = "ReadTOC",
    }
    
    logCount = logCount + 1
    local cmdName = cd_cmds[cmd] or string.format("Cmd_%02X", cmd)
    print(string.format("[%04d] CD_CMD: %s (0x%02X) from PC=%s",
        logCount, cmdName, cmd, hex(regs.pc)))
end)
table.insert(breakpoints, cdCmdBP)
print("Hooked CD Command Register @ " .. hex(CDROM_REG1))

-- ============================================  
-- Method 3: Specific known addresses (if provided)
-- ============================================
-- These are common PSY-Q library function offsets
-- Adjust for your specific binary

local KNOWN_FUNCS = {
    -- Add addresses you find in Ghidra here:
    -- {addr = 0x800863B0, name = "CdReadFile"},
    -- {addr = 0x80086XXX, name = "CdRead"},
}

for _, func in ipairs(KNOWN_FUNCS) do
    local bp = PCSX.addBreakpoint(func.addr, 'Exec', 4, func.name, function()
        local regs = PCSX.getRegisters()
        logCount = logCount + 1
        print(string.format("[%04d] %s: a0=%s a1=%s a2=%s a3=%s ra=%s",
            logCount, func.name,
            hex(regs.GPR.n.a0), hex(regs.GPR.n.a1),
            hex(regs.GPR.n.a2), hex(regs.GPR.n.a3),
            hex(regs.GPR.n.ra)))
    end)
    table.insert(breakpoints, bp)
    print("Hooked " .. func.name .. " @ " .. hex(func.addr))
end

-- ============================================
-- Monitor game state
-- ============================================
local stateAddr = 0x8009DD80
local stateBP = PCSX.addBreakpoint(stateAddr, 'Write', 1, "GameState", function(address)
    local mem = PCSX.getMemPtr()
    local offset = bit.band(address, 0x1FFFFF)
    local newValue = mem[offset]
    print(string.format("[STATE] Game state: 0x%02X @ PC=%s", 
        newValue, hex(PCSX.getRegisters().pc)))
end)
table.insert(breakpoints, stateBP)
print("Monitoring GameState @ " .. hex(stateAddr))

print("")
print("Tracing: BIOS syscalls + CD hardware + GameState")
print("===========================================")
