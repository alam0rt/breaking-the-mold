-- decode_hidden_bs.lua
-- Decode and display hidden BS/MDEC images from GAME.BLB
-- 
-- Usage in PCSX-Redux:
-- 1. Load the game normally
-- 2. Open Lua console (Debug > Lua Console)
-- 3. Run: dofile("scripts/decode_hidden_bs.lua")
-- 4. Call: decode_bs_image(2, 10)  -- for sectors 2-11
--
-- Based on PlayStation MDEC documentation (mdecnote.pdf)

local ffi = require("ffi")

-- Constants
local SECTOR_SIZE = 2048
local DISPLAY_WIDTH = 320
local DISPLAY_HEIGHT = 256

-- BLB file location (loaded in emulator)
local g_BLBSector = 0x800A59F0  -- g_GameBLBSector

-- Function to read BS frame from BLB at given sector
function read_bs_from_blb(start_sector, sector_count)
    print(string.format("Reading BS frame from sector %d (%d sectors)", start_sector, sector_count))
    
    -- Get the buffer address (we'll use the game's own buffer)
    -- The game uses param_1[0xf] as the decompression buffer
    local buffer_base = 0x801FC000  -- High RAM area
    
    -- Read the sectors using CD functions
    -- This mimics what CdBLB_ReadSectors does
    local blb_start = PCSX.getMemPtr(g_BLBSector)
    if blb_start then
        local blb_sector = ffi.cast("uint32_t*", blb_start)[0]
        print(string.format("BLB starts at CD sector %d", blb_sector))
    end
    
    return buffer_base
end

-- Decode a BS frame using MDEC
-- This calls the real PSX MDEC functions
function decode_bs_frame(bs_data_addr, output_addr)
    print("Decoding BS frame using MDEC...")
    
    -- The game's decode function is at 0x800399a8 (FUN_800399a8)
    -- It does:
    -- 1. CdBLB_ReadSectors(sector_offset, sector_count, buffer + 0x33800)
    -- 2. DecDCTReset(0)
    -- 3. DecDCTvlcBuild(buffer)
    -- 4. DecDCTvlc2(buffer + 0x33800, runlevel_buffer, buffer)
    -- 5. DecDCTin(runlevel_buffer, 2)
    -- 6. DecDCTout(vram_buffer, width*height/2)
    
    -- We can't easily call these directly, but we can trigger a load
    print("To decode, we would need to:")
    print("1. Write BS data to memory")
    print("2. Call DecDCTReset(0)")
    print("3. Call DecDCTvlcBuild(buffer)")
    print("4. Call DecDCTvlc2(bs_data, runlevel, buffer)")
    print("5. Call DecDCTin(runlevel, 0)")
    print("6. Call DecDCTout(output, 320*256/2)")
    print("7. Call DecDCToutSync(0)")
end

-- Main function to decode hidden BS images
function decode_hidden_image(image_name)
    local images = {
        mystery = { sector = 2, count = 10, desc = "Mystery unused image" },
        pira = { sector = 12, count = 10, desc = "PIRA (Pirates intro)" },
        legl = { sector = 22, count = 4, desc = "LEGL (Legal screen)" },
        container_0 = { sector = 26, count = 13, desc = "Container frame 0" },
    }
    
    local img = images[image_name]
    if not img then
        print("Unknown image. Available: mystery, pira, legl, container_0")
        return
    end
    
    print(string.format("\n=== Decoding: %s ===", img.desc))
    print(string.format("Sector: %d, Count: %d", img.sector, img.count))
    
    -- For now, just show info about what we would decode
    -- Full decoding requires more complex MDEC emulation
end

-- Quick analysis function
function analyze_hidden_data()
    print("\n=== Hidden BS Images in GAME.BLB ===\n")
    print("1. MYSTERY (sectors 2-11)")
    print("   - Valid BS/MDEC v2 frame")
    print("   - Quant=7 (lower quality than others)")
    print("   - NOT referenced by any game code")
    print("   - Likely: Unused loading screen or test image")
    print("")
    print("2. Container at sectors 26-196")
    print("   - Contains 10 BS frames")
    print("   - Each is a 320x256 image")
    print("   - NOT referenced by any game code")
    print("   - Likely: Unused cutscene or transition frames")
    print("")
    print("3. Duplicate at sectors 36929-37099")
    print("   - Exact copy of #2")
    print("   - Build artifact")
    print("")
    print("To actually view these images, you need to:")
    print("1. Patch the sector table to point to the hidden data")
    print("2. Trigger a loading screen display")
    print("3. Capture the VRAM output")
end

-- Patch sector table to show hidden image
function patch_sector_entry(entry_index, new_sector, new_count)
    -- Sector table is at BLB offset 0xCD0
    -- Each entry is 16 bytes
    -- sector_offset at +0x0C (u16), sector_count at +0x0E (u16)
    
    local header_base = 0x800AE3E0  -- BLB header in RAM
    local entry_offset = 0xCD0 + (entry_index * 0x10)
    local sector_addr = header_base + entry_offset + 0x0C
    
    print(string.format("Patching sector entry %d at 0x%08X", entry_index, sector_addr))
    print(string.format("  New sector: %d, count: %d", new_sector, new_count))
    
    -- Write the new values
    local ptr = PCSX.getMemPtr(sector_addr)
    if ptr then
        local data = ffi.cast("uint16_t*", ptr)
        data[0] = new_sector  -- sector_offset
        data[1] = new_count   -- sector_count
        print("Patched! Now trigger a level load to see the hidden image.")
    else
        print("ERROR: Could not get memory pointer")
    end
end

-- Show help
function help()
    print([[
Hidden BS Image Decoder for Skullmonkeys
=========================================

Functions:
  analyze_hidden_data()     - Show info about hidden images
  patch_sector_entry(0, 2, 10)  - Patch PIRA entry to show mystery image
  
To view the mystery image:
  1. patch_sector_entry(0, 2, 10)  -- Point PIRA to mystery
  2. Start a new game or trigger level load
  3. The "PIRA" screen will now show the mystery image

Hidden images found:
  - Sectors 2-11: Unknown BS image (20KB)
  - Sectors 26-196: 10 BS frames (342KB)
  - Sectors 36929-37099: Duplicate of above
]])
end

print("Hidden BS Image Decoder loaded!")
print("Run help() for usage info, or analyze_hidden_data() for details")
