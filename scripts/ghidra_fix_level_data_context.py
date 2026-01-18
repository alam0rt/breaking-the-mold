#@category Skullmonkeys
#@menupath Analysis.Fix LevelDataContext Struct
#@description Recreates LevelDataContext struct (128 bytes) for asset loading context

"""
Ghidra Python script to fix/recreate the LevelDataContext struct.
Run this in Ghidra's Script Manager or via analyzeHeadless.

Updated: 2026-01-18
Based on:
- include/Game/level_data_context.h (source of truth)
- LoadAssetContainer @ 0x8007B074 (populates asset pointers)
- InitLevelDataContext @ 0x8007A1BC
- ClearLevelDataContext @ 0x8007A234
"""

from ghidra.program.model.data import (
    StructureDataType, CategoryPath, PointerDataType,
    UnsignedCharDataType, UnsignedIntegerDataType
)

# Constants
LEVEL_DATA_CONTEXT_SIZE = 0x80  # 128 bytes
LEVEL_DATA_CONTEXT_ADDR = 0x8009DCC4  # GameState + 0x84
CATEGORY = "/Skullmonkeys"

def get_existing_struct(dtm, name):
    """Find an existing struct by name."""
    for dt in dtm.getAllDataTypes():
        if dt.getName() == name and isinstance(dt, StructureDataType):
            return dt
    return None

def create_level_data_context_struct(dtm):
    """Create the LevelDataContext structure with all known fields."""
    
    # Delete existing if present
    existing = get_existing_struct(dtm, "LevelDataContext")
    if existing is not None:
        dtm.remove(existing, monitor)
        print("Removed existing LevelDataContext struct")
    
    # Create new struct
    struct = StructureDataType(CategoryPath(CATEGORY), "LevelDataContext", 0)
    struct.setDescription(
        "Level data context at GameState+0x84 (PAL: 0x8009DCC4). Size 0x80 (128 bytes). "
        "Stores pointers to loaded BLB assets. Initialized by InitLevelDataContext @ 0x8007A1BC, "
        "populated by LoadAssetContainer @ 0x8007B074."
    )
    
    # Helper types
    ptr_type = PointerDataType()
    uint_type = UnsignedIntegerDataType()
    ubyte_type = UnsignedCharDataType()
    
    # Grow struct to full size first
    struct.growStructure(LEVEL_DATA_CONTEXT_SIZE)
    
    # ============================================================
    # FIELD DEFINITIONS - based on level_data_context.h
    # 
    # Asset types → context indices (from LoadAssetContainer):
    #   100 (0x64)  → ctx[1]  = TileHeader
    #   101 (0x65)  → ctx[2]  = VRAMSlotConfig  
    #   200 (0xC8)  → ctx[3]  = TilemapContainer
    #   201 (0xC9)  → ctx[4]  = LayerEntries
    #   300 (0x12C) → ctx[5]  = TilePixels
    #   301 (0x12D) → ctx[6]  = PaletteIndices
    #   302 (0x12E) → ctx[7]  = TileFlags
    #   303 (0x12F) → ctx[10] = AnimatedTiles
    #   400 (0x190) → ctx[8]  = PaletteContainer
    #   401 (0x191) → ctx[9]  = PaletteAnim
    #   500 (0x1F4) → ctx[11] = TileAttributes
    #   501 (0x1F5) → ctx[14] = Entities
    #   502 (0x1F6) → ctx[15] = VRAMRects
    #   503 (0x1F7) → ctx[12] = AnimOffsets
    #   504 (0x1F8) → ctx[13] = VehicleData
    #   600 (0x258) → ctx[16,17] = Sprites + size
    #   601 (0x259) → ctx[18,19] = Audio + size
    #   602 (0x25A) → ctx[20] = Palette
    #   700 (0x2BC) → ctx[21,22] = SPUSamples + size
    # ============================================================
    
    fields = [
        # Asset Pointers (indices 0-22, cleared by ClearLevelDataContext)
        (0x00, "current_sub_block", uint_type, "Sub-block index (0=none, 1=primary, 2=secondary)"),
        (0x04, "tile_header", ptr_type, "Asset 100: TileHeader (36 bytes)"),
        (0x08, "vram_slot_config", ptr_type, "Asset 101: VRAMSlotConfig (12 bytes)"),
        (0x0C, "tilemap_container", ptr_type, "Asset 200: TilemapContainer sub-TOC"),
        (0x10, "layer_entries", ptr_type, "Asset 201: LayerEntries (92 bytes each)"),
        (0x14, "tile_pixels", ptr_type, "Asset 300: TilePixels (8bpp indexed)"),
        (0x18, "palette_indices", ptr_type, "Asset 301: PaletteIndices (1 byte/tile)"),
        (0x1C, "tile_flags", ptr_type, "Asset 302: TileFlags (semi-trans, size, skip)"),
        (0x20, "palette_container", ptr_type, "Asset 400: PaletteContainer sub-TOC"),
        (0x24, "palette_anim", ptr_type, "Asset 401: PaletteAnim data"),
        (0x28, "animated_tiles", ptr_type, "Asset 303: AnimatedTiles lookup"),
        (0x2C, "tile_attributes", ptr_type, "Asset 500: TileAttributes (collision, 1 byte/tile)"),
        (0x30, "anim_offsets", ptr_type, "Asset 503: AnimOffsets (ToolX sequences)"),
        (0x34, "vehicle_data", ptr_type, "Asset 504: VehicleData (FINN/RUNN levels)"),
        (0x38, "entities", ptr_type, "Asset 501: Entities (24-byte structs)"),
        (0x3C, "vram_rects", ptr_type, "Asset 502: VRAMRects (texture pages)"),
        (0x40, "sprites", ptr_type, "Asset 600: Sprites/Geometry data"),
        (0x44, "sprites_size", uint_type, "Asset 600: Size in bytes"),
        (0x48, "audio", ptr_type, "Asset 601: Audio (SPU ADPCM)"),
        (0x4C, "audio_size", uint_type, "Asset 601: Size in bytes"),
        (0x50, "palette", ptr_type, "Asset 602: Palette (15-bit PSX colors)"),
        (0x54, "spu_samples", ptr_type, "Asset 700: SPUSamples (additional audio)"),
        (0x58, "spu_samples_size", uint_type, "Asset 700: Size in bytes"),
        
        # BLB Navigation (indices 23-25, set by InitLevelDataContext)
        (0x5C, "blb_header", ptr_type, "Pointer to loaded BLB header"),
        (0x60, "current_sequence_index", ubyte_type, "Current playback sequence index (0xFF = none)"),
        (0x61, "_pad_61", ubyte_type, "Padding"),
        (0x62, "_pad_62", ubyte_type, "Padding"),
        (0x63, "_pad_63", ubyte_type, "Padding"),
        (0x64, "loader_callback", ptr_type, "Sector loader callback function"),
        
        # Loaded Data Pointers (indices 26-31, set by LevelDataParser)
        (0x68, "primary_data", ptr_type, "Primary asset container base"),
        (0x6C, "secondary_data", ptr_type, "Secondary/tertiary container base"),
        (0x70, "container_600", ptr_type, "Asset 600 container (from mode 6)"),
        (0x74, "container_601", ptr_type, "Asset 601 container"),
        (0x78, "container_601_size", uint_type, "Asset 601 size"),
        (0x7C, "container_602", ptr_type, "Asset 602 container"),
    ]
    
    # Add all fields
    for offset, name, dtype, comment in fields:
        try:
            struct.replaceAtOffset(offset, dtype, dtype.getLength(), name, comment)
            print("Added field: 0x%02X %s" % (offset, name))
        except Exception as e:
            print("Warning: Could not add %s at 0x%02X: %s" % (name, offset, str(e)))
    
    # Add to data type manager
    dtm.addDataType(struct, None)
    print("LevelDataContext struct created successfully (%d bytes)" % struct.getLength())
    
    return struct

def run():
    """Main script entry point."""
    print("=" * 60)
    print("LevelDataContext Struct Fixer for Skullmonkeys (PAL)")
    print("Updated: 2026-01-18")
    print("=" * 60)
    
    dtm = currentProgram.getDataTypeManager()
    
    # Create the struct
    struct = create_level_data_context_struct(dtm)
    
    print("=" * 60)
    print("Done! LevelDataContext struct created.")
    print("=" * 60)
    print("")
    print("Key offsets (as ctx[N] indices):")
    print("  ctx[0]  0x00: current_sub_block")
    print("  ctx[1]  0x04: tile_header (Asset 100)")
    print("  ctx[4]  0x10: layer_entries (Asset 201)")
    print("  ctx[11] 0x2C: tile_attributes (Asset 500)")
    print("  ctx[14] 0x38: entities (Asset 501)")
    print("  ctx[16] 0x40: sprites (Asset 600)")
    print("  ctx[23] 0x5C: blb_header")
    print("")
    print("Next steps:")
    print("1. Run ghidra_fix_gamestate.py to embed this in GameState")
    print("2. Run Auto-Analyze to propagate types")

# Run the script
if __name__ == "__main__":
    run()
