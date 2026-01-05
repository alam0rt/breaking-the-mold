-- Decode mystery BS frames from unreferenced BLB sectors
-- Run in PCSX-Redux: Configuration > Emulation > Enable Lua and load this script

-- The mystery image is at BLB offset 0x1000 (sector 2)
-- It's a valid BS/MDEC v2 frame: 13216 words, quant=7

local function read_blb_sectors(sector_start, sector_count)
    -- Read sectors from the loaded BLB/ISO
    local SECTOR_SIZE = 2048
    local offset = sector_start * SECTOR_SIZE
    local size = sector_count * SECTOR_SIZE
    
    -- This would need access to the disc image
    -- For now, we'll use the memory-based approach
    print(string.format("Would read %d sectors starting at sector %d (offset 0x%X)", 
        sector_count, sector_start, offset))
end

-- Alternative: Load the pre-extracted BS frame from file
local function decode_bs_from_file(filename, output_name)
    print("=== Decoding BS Frame ===")
    print("File: " .. filename)
    
    -- Read the file
    local f = io.open(filename, "rb")
    if not f then
        print("Error: Cannot open file " .. filename)
        return
    end
    
    local data = f:read("*all")
    f:close()
    
    print("Read " .. #data .. " bytes")
    
    -- Parse BS header
    local frame_size = string.unpack("<H", data, 1)
    local magic = string.unpack("<H", data, 3)
    local quant = string.unpack("<H", data, 5)
    local version = string.unpack("<H", data, 7)
    
    print(string.format("Frame size: %d words", frame_size))
    print(string.format("Magic: 0x%04X %s", magic, magic == 0x3800 and "✓" or "✗"))
    print(string.format("Quant scale: %d", quant))
    print(string.format("Version: %d", version))
    
    if magic ~= 0x3800 then
        print("Error: Not a valid BS frame")
        return
    end
    
    -- To decode, we need to:
    -- 1. Call DecDCTvlcBuild() to build VLC table
    -- 2. Call DecDCTvlc2() to decompress VLC to DCT blocks
    -- 3. Call DecDCTin() to send blocks to MDEC
    -- 4. Call DecDCTout() to receive decoded pixels
    
    -- This requires the emulator to be running with the game loaded
    -- The game's MDEC routines can be called directly
    
    print("\nTo decode this image:")
    print("1. Start the game in PCSX-Redux")
    print("2. Pause at a point where MDEC is initialized")
    print("3. Upload this data to RAM")
    print("4. Call the game's display function (FUN_800399a8)")
    print("5. Capture VRAM")
end

-- Print instructions
print([[
=== Mystery BS Frame Decoder ===

The mystery image at BLB sectors 2-11 is a valid BS/MDEC frame.
It cannot be decoded with jpsxdec because it's not in STR container format.

To view this image, you need to:

Option 1: Use the game's own decoder
  - Load the game in PCSX-Redux
  - Patch the sector table to point to sector 2 instead of PIRA (sector 12)
  - The game will display the mystery image as a loading screen

Option 2: Manual MDEC decode
  - Run the game until MDEC is initialized
  - Upload the BS data to a free RAM location
  - Call DecDCTvlc2() on the data
  - Capture the VRAM output

The mystery image has:
  - Quant=7 (lower quality than PIRA's quant=4)
  - Same VLC start pattern as PIRA/LEGL (dark/black start)
  - 26,432 bytes of data (vs PIRA's 23,040)

This is likely an unused or cut loading screen from development.
]])

-- Try to decode if file exists
local mystery_file = "/tmp/unreferenced_bs/mystery_sector2.bs"
decode_bs_from_file(mystery_file, "mystery")
