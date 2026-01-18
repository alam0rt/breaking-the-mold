#@category Skullmonkeys
#@menupath Analysis.Fix LayerEntry Struct
#@description Recreates LayerEntry struct (92 bytes) for Asset 201 layer data

"""
Ghidra Python script to fix/recreate the LayerEntry struct.
Run this in Ghidra's Script Manager or via analyzeHeadless.

Updated: 2026-01-18
Based on:
- blb.hexpat pattern file (source of truth)
- GetLayerEntry @ 0x8007B700 (returns ctx[4] + index * 0x5C)
- InitLayersAndTileState @ 0x80024778
"""

from ghidra.program.model.data import (
    StructureDataType, CategoryPath, PointerDataType,
    UnsignedCharDataType, UnsignedShortDataType,
    UnsignedIntegerDataType, ArrayDataType
)

# Constants
LAYER_ENTRY_SIZE = 0x5C  # 92 bytes
CATEGORY = "/Skullmonkeys"

def get_existing_struct(dtm, name):
    """Find an existing struct by name."""
    for dt in dtm.getAllDataTypes():
        if dt.getName() == name and isinstance(dt, StructureDataType):
            return dt
    return None

def create_color_tint_struct(dtm):
    """Create ColorTintEntry struct (3 bytes)."""
    existing = get_existing_struct(dtm, "ColorTintEntry")
    if existing is not None:
        dtm.remove(existing, monitor)
        print("Removed existing ColorTintEntry struct")
    
    struct = StructureDataType(CategoryPath(CATEGORY), "ColorTintEntry", 0)
    struct.setDescription("RGB color tint (3 bytes). Used in LayerEntry color_tints[16] table.")
    
    ubyte = UnsignedCharDataType()
    struct.add(ubyte, 1, "r", "Red component")
    struct.add(ubyte, 1, "g", "Green component")
    struct.add(ubyte, 1, "b", "Blue component")
    
    dtm.addDataType(struct, None)
    print("ColorTintEntry struct created (3 bytes)")
    return struct

def create_layer_entry_struct(dtm):
    """Create the LayerEntry structure with all known fields."""
    
    # Create ColorTintEntry first
    color_tint = create_color_tint_struct(dtm)
    
    # Delete existing LayerEntry if present
    existing = get_existing_struct(dtm, "LayerEntry")
    if existing is not None:
        dtm.remove(existing, monitor)
        print("Removed existing LayerEntry struct")
    
    # Create new struct
    struct = StructureDataType(CategoryPath(CATEGORY), "LayerEntry", 0)
    struct.setDescription(
        "Layer metadata from Asset 201 (92 bytes per layer). "
        "Accessed via GetLayerEntry @ 0x8007B700 (ctx[4] + index * 0x5C). "
        "Contains position, scroll factors, render mode, and 16 color tints."
    )
    
    # Helper types
    ushort = UnsignedShortDataType()
    uint = UnsignedIntegerDataType()
    ubyte = UnsignedCharDataType()
    
    # Grow struct to full size first
    struct.growStructure(LAYER_ENTRY_SIZE)
    
    # Position and dimensions (0x00-0x0B)
    struct.replaceAtOffset(0x00, ushort, 2, "x_offset", "X position in tiles")
    struct.replaceAtOffset(0x02, ushort, 2, "y_offset", "Y position in tiles")
    struct.replaceAtOffset(0x04, ushort, 2, "width", "Layer width in tiles")
    struct.replaceAtOffset(0x06, ushort, 2, "height", "Layer height in tiles")
    struct.replaceAtOffset(0x08, ushort, 2, "level_width", "Level width in tiles (from Asset 100)")
    struct.replaceAtOffset(0x0A, ushort, 2, "level_height", "Level height in tiles")
    
    # Render parameter (0x0C-0x0F)
    struct.replaceAtOffset(0x0C, uint, 4, "render_param", "Low 16 bits = priority (lower = further back)")
    
    # Scroll factors (0x10-0x17) - 16.16 fixed-point
    struct.replaceAtOffset(0x10, uint, 4, "scroll_x", "X parallax (0x10000=1.0, 0x8000=0.5)")
    struct.replaceAtOffset(0x14, uint, 4, "scroll_y", "Y parallax factor")
    
    # Render context fields (0x18-0x1D)
    struct.replaceAtOffset(0x18, ushort, 2, "render_field_30", "Stored at renderCtx+0x30")
    struct.replaceAtOffset(0x1A, ushort, 2, "render_field_32", "Stored at renderCtx+0x32")
    struct.replaceAtOffset(0x1C, ubyte, 1, "render_field_3a", "Stored at renderCtx+0x3A")
    struct.replaceAtOffset(0x1D, ubyte, 1, "render_field_3b", "Stored at renderCtx+0x3B")
    
    # Scroll enable flags (0x1E-0x21)
    struct.replaceAtOffset(0x1E, ubyte, 1, "scroll_left_enable", "If !=0, set GameState+0x59")
    struct.replaceAtOffset(0x1F, ubyte, 1, "scroll_right_enable", "If !=0, set GameState+0x5B")
    struct.replaceAtOffset(0x20, ubyte, 1, "scroll_up_enable", "If !=0, set GameState+0x58")
    struct.replaceAtOffset(0x21, ubyte, 1, "scroll_down_enable", "If !=0, set GameState+0x5A")
    
    # Render mode selection (0x22-0x25)
    struct.replaceAtOffset(0x22, ushort, 2, "render_mode_h", "If !=0, affects render path")
    struct.replaceAtOffset(0x24, ushort, 2, "render_mode_v", "If !=0, affects render path")
    
    # Layer type and flags (0x26-0x2B)
    struct.replaceAtOffset(0x26, ubyte, 1, "layer_type", "Layer type (0=normal, 3=skip render)")
    struct.replaceAtOffset(0x27, ubyte, 1, "_pad_27", "Padding")
    struct.replaceAtOffset(0x28, ushort, 2, "skip_render", "If !=0 (and type!=3), skip this layer")
    struct.replaceAtOffset(0x2A, ushort, 2, "_pad_2a", "Padding (always 0)")
    
    # Color Tint Table (0x2C-0x5B) - 16 RGB entries, 48 bytes
    # Create array of ColorTintEntry[16]
    color_tint_array = ArrayDataType(color_tint, 16, 3)
    try:
        struct.replaceAtOffset(0x2C, color_tint_array, 48, "color_tints",
                               "16 RGB tints, selected by tilemap bits 12-15")
        print("Embedded ColorTintEntry[16] at offset 0x2C")
    except Exception as e:
        print("Warning: Could not embed color_tints array: %s" % str(e))
    
    # Add to data type manager
    dtm.addDataType(struct, None)
    print("LayerEntry struct created successfully (%d bytes)" % struct.getLength())
    
    return struct

def run():
    """Main script entry point."""
    print("=" * 60)
    print("LayerEntry Struct Fixer for Skullmonkeys (PAL)")
    print("Updated: 2026-01-18")
    print("=" * 60)
    
    dtm = currentProgram.getDataTypeManager()
    
    # Create the struct
    struct = create_layer_entry_struct(dtm)
    
    print("=" * 60)
    print("Done! LayerEntry struct created.")
    print("=" * 60)
    print("")
    print("Key offsets:")
    print("  0x00: x_offset, y_offset, width, height")
    print("  0x0C: render_param (low 16 = priority)")
    print("  0x10: scroll_x, scroll_y (16.16 fixed-point)")
    print("  0x1E: scroll enable flags (L/R/U/D)")
    print("  0x22: render_mode_h/v")
    print("  0x26: layer_type, skip_render")
    print("  0x2C: color_tints[16] (48 bytes of RGB)")
    print("")
    print("Next steps:")
    print("1. Run Auto-Analyze to propagate types")
    print("2. Use at GetLayerEntry @ 0x8007B700 return type")

# Run the script
if __name__ == "__main__":
    run()
