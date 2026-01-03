--[[
    PCSX-Redux MCP Extension Script
    
    This script adds custom HTTP endpoints to PCSX-Redux's web server
    that extend the capabilities available to the MCP server.
    
    Load this script in PCSX-Redux to enable advanced debugging features.
    
    Usage:
        make lua SCRIPT=scripts/mcp_endpoints.lua
    
    Or in PCSX-Redux Lua console:
        dofile('scripts/mcp_endpoints.lua')
    
    Endpoints added:
        GET  /api/v1/lua/registers     - Read all CPU registers
        POST /api/v1/lua/registers     - Write specific register
        GET  /api/v1/lua/breakpoints   - List active breakpoints
        POST /api/v1/lua/breakpoint    - Add/remove breakpoint
        GET  /api/v1/lua/disasm        - Disassemble memory range
        GET  /api/v1/lua/struct        - Read structured data
]]

local ffi = require('ffi')
local json = require('json') -- PCSX-Redux includes json library

-- Store breakpoints to prevent garbage collection
local breakpoints = {}
local breakpointId = 0

-- Helper: Convert register struct to Lua table
local function registersToTable()
    local regs = PCSX.getRegisters()
    return {
        -- General purpose registers
        gpr = {
            r0 = regs.GPR.n.r0,
            at = regs.GPR.n.at,
            v0 = regs.GPR.n.v0, v1 = regs.GPR.n.v1,
            a0 = regs.GPR.n.a0, a1 = regs.GPR.n.a1, a2 = regs.GPR.n.a2, a3 = regs.GPR.n.a3,
            t0 = regs.GPR.n.t0, t1 = regs.GPR.n.t1, t2 = regs.GPR.n.t2, t3 = regs.GPR.n.t3,
            t4 = regs.GPR.n.t4, t5 = regs.GPR.n.t5, t6 = regs.GPR.n.t6, t7 = regs.GPR.n.t7,
            s0 = regs.GPR.n.s0, s1 = regs.GPR.n.s1, s2 = regs.GPR.n.s2, s3 = regs.GPR.n.s3,
            s4 = regs.GPR.n.s4, s5 = regs.GPR.n.s5, s6 = regs.GPR.n.s6, s7 = regs.GPR.n.s7,
            t8 = regs.GPR.n.t8, t9 = regs.GPR.n.t9,
            k0 = regs.GPR.n.k0, k1 = regs.GPR.n.k1,
            gp = regs.GPR.n.gp, sp = regs.GPR.n.sp, s8 = regs.GPR.n.s8, ra = regs.GPR.n.ra,
            lo = regs.GPR.n.lo, hi = regs.GPR.n.hi,
        },
        pc = regs.pc,
        -- CP0 registers (subset of commonly used ones)
        cp0 = {
            status = regs.CP0.r[12],
            cause = regs.CP0.r[13],
            epc = regs.CP0.r[14],
            badvaddr = regs.CP0.r[8],
        },
    }
end

-- Helper: Read memory as hex string
local function readMemoryHex(address, size)
    local mem = PCSX.getMemPtr()
    local offset = bit.band(address, 0x1FFFFF)  -- Mask to 2MB
    local result = {}
    for i = 0, size - 1 do
        table.insert(result, string.format('%02x', mem[offset + i]))
    end
    return table.concat(result)
end

-- Helper: Read u32 from memory
local function readU32(address)
    local mem = PCSX.getMemPtr()
    local offset = bit.band(address, 0x1FFFFF)
    return mem[offset] + 
           mem[offset + 1] * 0x100 + 
           mem[offset + 2] * 0x10000 + 
           mem[offset + 3] * 0x1000000
end

-- Helper: Read u16 from memory
local function readU16(address)
    local mem = PCSX.getMemPtr()
    local offset = bit.band(address, 0x1FFFFF)
    return mem[offset] + mem[offset + 1] * 0x100
end

-- ============================================================================
-- HTTP Endpoint Handlers
-- ============================================================================

-- GET /api/v1/lua/registers - Read all CPU registers
PCSX.WebServer.Handlers = PCSX.WebServer.Handlers or {}

PCSX.WebServer.Handlers["registers"] = function(request)
    if request.method == "GET" then
        local regs = registersToTable()
        return json.encode(regs)
    elseif request.method == "POST" then
        -- Write register(s) from form data
        local regs = PCSX.getRegisters()
        for name, value in pairs(request.form or {}) do
            local val = tonumber(value)
            if val then
                if name == "pc" then
                    regs.pc = val
                elseif regs.GPR.n[name] then
                    regs.GPR.n[name] = val
                end
            end
        end
        return json.encode({ status = "ok" })
    end
end

-- GET /api/v1/lua/breakpoints - List breakpoints
-- POST /api/v1/lua/breakpoints - Add/remove breakpoint
PCSX.WebServer.Handlers["breakpoints"] = function(request)
    if request.method == "GET" then
        -- List all breakpoints
        local list = {}
        for id, bp in pairs(breakpoints) do
            table.insert(list, {
                id = id,
                address = bp.address,
                type = bp.bpType,
                width = bp.width,
                cause = bp.cause,
                enabled = bp.handle:isEnabled(),
                hitCount = bp.hitCount or 0,
            })
        end
        return json.encode({ breakpoints = list })
    elseif request.method == "POST" then
        local form = request.form or {}
        local action = form.action or "add"
        
        if action == "add" then
            local address = tonumber(form.address) or 0
            local bpType = form.type or "Exec"
            local width = tonumber(form.width) or 4
            local cause = form.cause or "MCP breakpoint"
            local pauseOnHit = form.pause ~= "false"
            
            breakpointId = breakpointId + 1
            local id = breakpointId
            
            local handle = PCSX.addBreakpoint(address, bpType, width, cause, function(addr, w, c)
                breakpoints[id].hitCount = (breakpoints[id].hitCount or 0) + 1
                if pauseOnHit then
                    PCSX.pauseEmulator()
                end
            end)
            
            breakpoints[id] = {
                handle = handle,
                address = address,
                bpType = bpType,
                width = width,
                cause = cause,
                hitCount = 0,
            }
            
            return json.encode({ status = "ok", id = id })
            
        elseif action == "remove" then
            local id = tonumber(form.id)
            if id and breakpoints[id] then
                breakpoints[id].handle:remove()
                breakpoints[id] = nil
                return json.encode({ status = "ok" })
            else
                return json.encode({ error = "Breakpoint not found" })
            end
            
        elseif action == "enable" then
            local id = tonumber(form.id)
            if id and breakpoints[id] then
                breakpoints[id].handle:enable()
                return json.encode({ status = "ok" })
            end
            
        elseif action == "disable" then
            local id = tonumber(form.id)
            if id and breakpoints[id] then
                breakpoints[id].handle:disable()
                return json.encode({ status = "ok" })
            end
            
        elseif action == "clear" then
            for id, bp in pairs(breakpoints) do
                bp.handle:remove()
            end
            breakpoints = {}
            return json.encode({ status = "ok" })
        end
        
        return json.encode({ error = "Unknown action" })
    end
end

-- GET /api/v1/lua/struct?address=X&format=FORMAT - Read structured data
-- Formats: blb_header, level_entry, sector_entry
PCSX.WebServer.Handlers["struct"] = function(request)
    local query = request.urlData.query or ""
    
    -- Parse query string manually (simple version)
    local params = {}
    for key, value in query:gmatch("([^&=]+)=([^&=]+)") do
        params[key] = tonumber(value) or value
    end
    
    local address = params.address or 0x800AE3E0  -- Default: BLB header
    local format = params.format or "raw"
    local size = params.size or 64
    
    if format == "blb_header" then
        -- Read BLB header summary
        local header_addr = address
        local level_count = PCSX.getMemPtr()[bit.band(header_addr + 0xF31, 0x1FFFFF)]
        local movie_count = PCSX.getMemPtr()[bit.band(header_addr + 0xF32, 0x1FFFFF)]
        local sector_count = PCSX.getMemPtr()[bit.band(header_addr + 0xF33, 0x1FFFFF)]
        
        return json.encode({
            address = string.format("0x%08X", header_addr),
            level_count = level_count,
            movie_count = movie_count,
            sector_count = sector_count,
        })
        
    elseif format == "level_entry" then
        local index = params.index or 0
        local entry_addr = address + (index * 0x70)
        
        local sector_offset = readU16(entry_addr + 0x00)
        local sector_count = readU16(entry_addr + 0x02)
        local unk_04 = readU16(entry_addr + 0x04)
        local unk_06 = readU16(entry_addr + 0x06)
        local unk_08 = readU16(entry_addr + 0x08)
        local unk_0A = readU16(entry_addr + 0x0A)
        
        local mem = PCSX.getMemPtr()
        local base = bit.band(entry_addr, 0x1FFFFF)
        local level_index = mem[base + 0x0C]
        local level_flag = mem[base + 0x0D]
        local tert_count = mem[base + 0x0E]
        
        -- Read level code at 0x56
        local code = ""
        for i = 0, 3 do
            local c = mem[base + 0x56 + i]
            if c > 0 then code = code .. string.char(c) end
        end
        
        -- Read level name at 0x5B
        local name = ""
        for i = 0, 24 do
            local c = mem[base + 0x5B + i]
            if c == 0 then break end
            name = name .. string.char(c)
        end
        
        return json.encode({
            index = index,
            address = string.format("0x%08X", entry_addr),
            code = code,
            name = name,
            sector_offset = sector_offset,
            sector_count = sector_count,
            unknown_04 = unk_04,
            unknown_06 = unk_06,
            unknown_08 = unk_08,
            unknown_0A = unk_0A,
            level_index = level_index,
            level_flag = level_flag,
            tert_block_count = tert_count,
        })
        
    else
        -- Raw hex dump
        return json.encode({
            address = string.format("0x%08X", address),
            size = size,
            hex = readMemoryHex(address, size),
        })
    end
end

-- GET /api/v1/lua/call?func=ADDR&a0=X&a1=Y... - Call function with args
-- This is DANGEROUS but useful for testing
PCSX.WebServer.Handlers["call"] = function(request)
    local query = request.urlData.query or ""
    local params = {}
    for key, value in query:gmatch("([^&=]+)=([^&=]+)") do
        params[key] = tonumber(value) or value
    end
    
    local func_addr = tonumber(params.func)
    if not func_addr then
        return json.encode({ error = "Missing func parameter" })
    end
    
    -- Set up arguments
    local regs = PCSX.getRegisters()
    if params.a0 then regs.GPR.n.a0 = tonumber(params.a0) end
    if params.a1 then regs.GPR.n.a1 = tonumber(params.a1) end
    if params.a2 then regs.GPR.n.a2 = tonumber(params.a2) end
    if params.a3 then regs.GPR.n.a3 = tonumber(params.a3) end
    
    -- Save current PC and set to function
    local old_pc = regs.pc
    local old_ra = regs.GPR.n.ra
    regs.pc = func_addr
    -- Set ra to a known address we can breakpoint on
    regs.GPR.n.ra = 0x80000000 + 0x1000  -- Will trap on return
    
    return json.encode({
        status = "prepared",
        func = string.format("0x%08X", func_addr),
        note = "Function call prepared. Resume emulation to execute.",
        old_pc = string.format("0x%08X", old_pc),
    })
end

-- GET /api/v1/lua/iso - Get info about loaded ISO
PCSX.WebServer.Handlers["iso"] = function(request)
    local iso = PCSX.getCurrentIso()
    if not iso then
        return json.encode({ error = "No ISO loaded" })
    end
    
    -- Try to get some basic info
    return json.encode({
        status = "ok",
        -- ISO info methods may vary by version
    })
end

print("MCP endpoints loaded successfully!")
print("Available endpoints:")
print("  GET  /api/v1/lua/registers")
print("  POST /api/v1/lua/registers")
print("  GET  /api/v1/lua/breakpoints")
print("  POST /api/v1/lua/breakpoints")
print("  GET  /api/v1/lua/struct")
print("  GET  /api/v1/lua/call")
print("  GET  /api/v1/lua/iso")
