-- Web server handlers for memory inspection
-- Load this in PCSX-Redux: Configuration > Lua > Run Script
-- Then call endpoints via: http://localhost:8080/api/v1/lua/<endpoint>

local ffi = require("ffi")

-- Helper to read memory
local function read_u16(addr)
    return PCSX.getMemPtr():cast("uint16_t*")[(addr - 0x80000000) / 2]
end

local function read_u32(addr)
    return PCSX.getMemPtr():cast("uint32_t*")[(addr - 0x80000000) / 4]
end

-- Read Asset 100 fields
PCSX.WebServer.Handlers["asset100"] = function(req)
    local GAME_STATE = 0x8009DC40
    local LEVEL_CTX = 0x8009DCC4
    
    -- Read LevelDataContext pointers
    local ctx_base = PCSX.getMemPtr():cast("uint32_t*")
    local asset100_ptr = ctx_base[(LEVEL_CTX + 4 - 0x80000000) / 4]  -- ctx[1]
    
    if asset100_ptr == 0 then
        return '{"error": "No level loaded (Asset100 pointer is null)"}'
    end
    
    -- Read Asset 100 header (36 bytes)
    local mem = PCSX.getMemPtr():cast("uint8_t*")
    local base = asset100_ptr - 0x80000000
    
    local result = string.format([[{
  "asset100_ptr": "0x%08X",
  "field_08_width": %d,
  "field_0a_height": %d,
  "field_10_tile16": %d,
  "field_12_tile8": %d,
  "field_14_extra": %d,
  "field_1c": %d,
  "field_1e": %d,
  "field_20": %d,
  "gamestate_78": %d
}]],
        asset100_ptr,
        read_u16(asset100_ptr + 0x08),
        read_u16(asset100_ptr + 0x0A),
        read_u16(asset100_ptr + 0x10),
        read_u16(asset100_ptr + 0x12),
        read_u16(asset100_ptr + 0x14),
        read_u16(asset100_ptr + 0x1C),
        read_u16(asset100_ptr + 0x1E),
        read_u16(asset100_ptr + 0x20),
        read_u16(GAME_STATE + 0x78)
    )
    
    return result
end

-- Read spawn coordinates
PCSX.WebServer.Handlers["spawn"] = function(req)
    local GAME_STATE = 0x8009DC40
    
    local spawn_x = read_u16(GAME_STATE + 0x116)
    local spawn_y = read_u16(GAME_STATE + 0x118)
    local field_78 = read_u16(GAME_STATE + 0x78)
    
    return string.format([[{
  "spawn_x": %d,
  "spawn_y": %d,
  "gamestate_78": %d
}]], spawn_x, spawn_y, field_78)
end

-- Read full GameState dump
PCSX.WebServer.Handlers["gamestate"] = function(req)
    local GAME_STATE = 0x8009DC40
    local result = "GameState dump (0x8009DC40):\n"
    
    local mem = PCSX.getMemPtr():cast("uint8_t*")
    for offset = 0, 0x150, 0x10 do
        result = result .. string.format("%04X: ", offset)
        for i = 0, 15 do
            result = result .. string.format("%02X ", mem[(GAME_STATE - 0x80000000) + offset + i])
        end
        result = result .. "\n"
    end
    
    return result
end

print("=== Web handlers registered ===")
print("Endpoints available:")
print("  /api/v1/lua/asset100  - Read Asset 100 fields")
print("  /api/v1/lua/spawn     - Read spawn coordinates")
print("  /api/v1/lua/gamestate - Full GameState hex dump")
print("")
print("Call with: curl http://localhost:8080/api/v1/lua/<endpoint>")
