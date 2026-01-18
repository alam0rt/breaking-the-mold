#@category Skullmonkeys
#@menupath Analysis.Fix GameState Struct
#@description Recreates GameState struct with proper field offsets from decompilation analysis

"""
Ghidra Python script to fix/recreate the GameState struct.
Run this in Ghidra's Script Manager or via analyzeHeadless.

Based on analysis of:
- InitGameState @ 0x8007cd34
- SetupAndStartLevel @ 0x8007d8a0
- SpawnPlayerAndEntities @ 0x8007df38
- SaveCheckpointState @ 0x8007eaac
- EntityTickLoop @ 0x80020e1c
"""

from ghidra.program.model.data import (
    StructureDataType, CategoryPath, PointerDataType,
    SignedWordDataType, UnsignedCharDataType, IntegerDataType,
    UnsignedShortDataType, ShortDataType, ByteDataType,
    ArrayDataType, Undefined1DataType
)
from ghidra.program.model.symbol import SourceType

# Constants
GAMESTATE_ADDR = 0x8009DC40
GAMESTATE_SIZE = 0x1A0  # 416 bytes
CATEGORY = "/Skullmonkeys"

def get_or_create_category(dtm, path):
    """Get or create a category path in the data type manager."""
    cat = dtm.getCategory(CategoryPath(path))
    if cat is None:
        cat = dtm.createCategory(CategoryPath(path))
    return cat

def get_existing_struct(dtm, name):
    """Find an existing struct by name."""
    for dt in dtm.getAllDataTypes():
        if dt.getName() == name and isinstance(dt, StructureDataType):
            return dt
    return None

def create_gamestate_struct(dtm):
    """Create the GameState structure with all known fields."""
    
    # Get LevelDataContext if it exists (to embed)
    level_ctx = get_existing_struct(dtm, "LevelDataContext")
    
    # Delete existing GameState if present
    existing = get_existing_struct(dtm, "GameState")
    if existing is not None:
        dtm.remove(existing, monitor)
        print("Removed existing GameState struct")
    
    # Create new struct
    struct = StructureDataType(CategoryPath(CATEGORY), "GameState", 0)
    struct.setDescription(
        "Main game state structure at 0x8009DC40. Size 0x1A0 (416 bytes). "
        "LevelDataContext is embedded at offset 0x84."
    )
    
    # Helper types
    ptr_type = PointerDataType()
    int_type = IntegerDataType()
    short_type = ShortDataType()
    ushort_type = UnsignedShortDataType()
    byte_type = ByteDataType()
    ubyte_type = UnsignedCharDataType()
    
    # ============================================================
    # FIELD DEFINITIONS - offset, size, name, type, comment
    # Discovered from decompilation of:
    #   - InitGameState @ 0x8007cd34
    #   - SetupAndStartLevel @ 0x8007d8a0
    #   - SpawnPlayerAndEntities @ 0x8007df38
    #   - SaveCheckpointState @ 0x8007eaac
    #   - RestoreCheckpointEntities @ 0x8007eaec
    #   - EntityTickLoopWithCamera @ 0x80020b34
    #   - UpdateCameraPositionSmooth @ 0x800233c0
    #   - InitLayersAndTileState @ 0x80024778
    #   - InitTileAttributeState @ 0x80024cf4
    #   - AddLayerToRenderList_* @ 0x80021590/778/960
    #   - ClearTickAndRenderLists @ 0x80020c54
    #   - MarkEntityForDeferredRemoval @ 0x80020d74
    #   - LoadTileDataToVRAM @ 0x80025240
    #   - AddPreInitEntitiesToList @ 0x800250c8
    # ============================================================
    fields = [
        # Core mode/callback (0x00-0x07)
        (0x00, 4, "mode_base_offset", int_type, "Base offset for current mode (0xFFFF0000)"),
        (0x04, 4, "mode_callback_ptr", ptr_type, "Current mode callback function (GameModeCallback)"),
        
        # Layer list heads (0x08-0x18) - from ClearTickAndRenderLists @ 0x80020c54
        # Each layer type has its own linked list for z-sorting with entities
        (0x08, 4, "layer_list_static", ptr_type, "Static layer list head (AddLayerToRenderList_Static)"),
        (0x0C, 4, "layer_list_scrolling", ptr_type, "Scrolling layer list head (AddLayerToRenderList_Scrolling)"),
        (0x10, 4, "layer_list_parallax", ptr_type, "Parallax layer list head (AddLayerToRenderList_Parallax)"),
        (0x14, 4, "layer_list_standard", ptr_type, "Standard layer list head (AddLayerToRenderList_Standard)"),
        (0x18, 4, "layer_render_context_ptr", ptr_type, "Layer render context pointer"),
        
        # Entity lists (0x1C-0x33)
        (0x1C, 4, "tick_list_head", ptr_type, "Head of z-sorted entity tick list"),
        (0x20, 4, "render_list_head", ptr_type, "Head of z-sorted entity render list"),
        (0x24, 4, "collision_list_head", ptr_type, "Entity collision/update queue head"),
        (0x28, 4, "entity_spawn_list", ptr_type, "Raw 24-byte entity defs from Asset 501"),
        (0x2C, 4, "player_entity_alt", ptr_type, "Player entity (alternate reference for collision)"),
        (0x30, 4, "player_entity_ptr", ptr_type, "Main player entity pointer"),
        
        # Deferred entity removal (0x34-0x3B) - from MarkEntityForDeferredRemoval @ 0x80020d74
        (0x34, 4, "pending_removal_entity", ptr_type, "Entity marked for deferred removal"),
        (0x38, 4, "removal_mode", int_type, "Removal mode: 0=all lists, 1=keep render"),
        
        # Previous spawn list (0x3C-0x43) - from SetupAndStartLevel @ 0x8007d8a0
        (0x3C, 4, "previous_spawn_list", ptr_type, "Previous spawn list (copied from 0x13C during setup)"),
        (0x40, 4, "field_40", int_type, "Unknown - cleared to 0 in InitGameState"),
        
        # Camera position (0x44-0x4B) - from UpdateCameraPositionSmooth @ 0x800233c0
        (0x44, 2, "camera_x", short_type, "Camera X position (pixels)"),
        (0x46, 2, "camera_y", short_type, "Camera Y position (pixels)"),
        (0x48, 2, "camera_limit_x", short_type, "Level width for camera X clamping"),
        (0x4A, 2, "camera_limit_y", short_type, "Level height for camera Y clamping"),
        
        # Camera velocity (0x4C-0x53) - 16.16 fixed-point
        (0x4C, 4, "camera_velocity_x", int_type, "Camera X velocity (16.16 fixed-point)"),
        (0x50, 4, "camera_velocity_y", int_type, "Camera Y velocity (16.16 fixed-point)"),
        
        # Player render offset (0x54-0x57)
        (0x54, 2, "player_render_offset_x", short_type, "Player X offset for rendering"),
        (0x56, 2, "player_render_offset_y", short_type, "Player Y offset for rendering"),
        
        # Scroll limit flags (0x58-0x5B)
        (0x58, 1, "scroll_limit_left", ubyte_type, "Camera left scroll limit flag"),
        (0x59, 1, "scroll_limit_top", ubyte_type, "Camera top scroll limit flag"),
        (0x5A, 1, "scroll_limit_right", ubyte_type, "Camera right scroll limit flag"),
        (0x5B, 1, "scroll_limit_bottom", ubyte_type, "Camera bottom scroll limit flag"),
        
        # Camera sub-pixel (0x5C-0x5F)
        (0x5C, 2, "camera_subpixel_x", ushort_type, "Camera X sub-pixel accumulator"),
        (0x5E, 2, "camera_subpixel_y", ushort_type, "Camera Y sub-pixel accumulator"),
        
        # Camera mode flags (0x60-0x63)
        (0x60, 1, "camera_y_lock", ubyte_type, "Camera vertical lock flag"),
        (0x61, 1, "camera_mode_flags", ubyte_type, "Camera mode flags"),
        (0x62, 1, "camera_invert_x", ubyte_type, "Camera invert flag"),
        (0x63, 1, "pause_freeze_flag", ubyte_type, "Pause/freeze flag (also set by checkpoint)"),
        
        # Player hitbox (0x64-0x67)
        (0x64, 2, "player_hitbox_width", short_type, "Player hitbox width (0x28 normal, 0 boss/glide)"),
        (0x66, 2, "player_hitbox_y_offset", short_type, "Player hitbox Y offset (0xFFD0=-48 normal)"),
        
        # Tile collision state (0x68-0x73) - from InitTileAttributeState @ 0x80024cf4
        (0x68, 4, "tile_collision_data_ptr", ptr_type, "Tile collision attribute array (from Asset 500)"),
        (0x6C, 2, "tile_collision_offset_x", short_type, "Tile collision offset X (tiles)"),
        (0x6E, 2, "tile_collision_offset_y", short_type, "Tile collision offset Y (tiles)"),
        (0x70, 2, "tile_collision_width", short_type, "Tile collision map width (tiles)"),
        (0x72, 2, "tile_collision_height", short_type, "Tile collision map height (tiles)"),
        
        # Level/layer state (0x74-0x7B) - from InitLayersAndTileState @ 0x80024778
        (0x74, 4, "level_context_field_3C", ptr_type, "LevelDataContext+0x3C ptr"),
        (0x78, 2, "tile_header_field_1C", ushort_type, "TileHeader field 0x1C"),
        (0x7A, 2, "field_7A", ushort_type, "Unknown field"),
        
        # Entity callback system (0x7C-0x83)
        (0x7C, 4, "entity_callback_table", ptr_type, "Entity type callback table (121 entries x 8 bytes)"),
        (0x80, 2, "entity_type_count", ushort_type, "Number of entity types (0x79 = 121)"),
        (0x82, 2, "field_82", ushort_type, "Unknown field"),
        
        # LevelDataContext embedded at 0x84 (128 bytes) - added separately
        
        # Post-LevelDataContext fields (0x104-0x10F)
        (0x104, 2, "tile_render_state_count", ushort_type, "Tile render state count"),
        (0x106, 2, "field_106", ushort_type, "Unknown field"),
        (0x108, 4, "tile_render_state_ptr", ptr_type, "Tile render state buffer (allocated, passed to layer init)"),
        (0x10C, 4, "frame_counter", int_type, "Frame counter (incremented in EntityTickLoopWithCamera)"),
        
        # Palette/spawn info (0x110-0x11F) - from LoadTileDataToVRAM @ 0x80025240
        (0x110, 4, "palette_group_ptrs", ptr_type, "Palette render context pointer array"),
        (0x114, 1, "palette_group_count", ubyte_type, "Count of palette groups (byte)"),
        (0x115, 1, "field_115", ubyte_type, "Padding"),
        (0x116, 2, "spawn_x", short_type, "Player spawn X position"),
        (0x118, 2, "spawn_y", short_type, "Player spawn Y position"),
        (0x11A, 2, "screen_shake_index", ushort_type, "Index into shake lookup table"),
        (0x11C, 4, "camera_scroll_speed", int_type, "Camera scroll speed (0x8000/0xC000/0x10000)"),
        
        # Special mode state (0x120-0x12F) - glide/boss/FINN
        (0x120, 2, "glide_boss_state_x", short_type, "Glide/boss/FINN state X (zeroed in SpawnPlayerAndEntities)"),
        (0x122, 2, "glide_boss_state_y", short_type, "Glide/boss/FINN state Y (zeroed in SpawnPlayerAndEntities)"),
        (0x124, 1, "layer0_tint_r", ubyte_type, "Layer 0 R color tint (from LayerEntry+0x2C)"),
        (0x125, 1, "layer0_tint_g", ubyte_type, "Layer 0 G color tint (from LayerEntry+0x2D)"),
        (0x126, 1, "layer0_tint_b", ubyte_type, "Layer 0 B color tint (from LayerEntry+0x2E)"),
        (0x127, 1, "field_127", ubyte_type, "Unknown field"),
        (0x128, 2, "field_128", short_type, "Unknown field"),
        (0x12A, 1, "glide_boss_flag", ubyte_type, "Glide/boss/FINN flag (zeroed in SpawnPlayerAndEntities)"),
        (0x12B, 1, "field_12B", ubyte_type, "Unknown field"),
        (0x12C, 4, "field_12C", int_type, "Unknown field"),
        
        # Background color update (0x130-0x133) - from RenderEntities
        (0x130, 1, "bg_color_change_flag", ubyte_type, "Triggers BG color update in RenderEntities"),
        (0x131, 1, "bg_color_r_pending", ubyte_type, "Pending BG R component"),
        (0x132, 1, "bg_color_g_pending", ubyte_type, "Pending BG G component"),
        (0x133, 1, "bg_color_b_pending", ubyte_type, "Pending BG B component"),
        
        # Checkpoint system (0x134-0x14F)
        (0x134, 4, "checkpoint_entity_list", ptr_type, "Saved entity list for checkpoint respawn"),
        (0x138, 4, "checkpoint_saved_score", int_type, "Saved frame_counter at checkpoint"),
        (0x13C, 4, "entity_defs_backup", ptr_type, "Backed up entity defs ptr in ClearEntitiesAndFadeToBlack"),
        (0x140, 4, "input_state_ptr", ptr_type, "Input state pointer"),
        (0x144, 1, "level_clear_pending", ubyte_type, "Level clear pending (triggers ClearEntitiesAndFadeToBlack)"),
        (0x145, 1, "level_transition_complete", ubyte_type, "Set after ClearEntitiesAndFadeToBlack runs"),
        (0x146, 1, "advance_level_flag", ubyte_type, "Advance to next level (calls AdvanceLevelSequence)"),
        (0x147, 1, "next_level_flag", ubyte_type, "Next level flag (set by SetNextLevelFlagWithFade)"),
        (0x148, 1, "direct_level_load", ubyte_type, "Direct level load flag"),
        (0x149, 1, "checkpoint_restore_pending", ubyte_type, "Checkpoint restore pending flag"),
        (0x14A, 1, "checkpoint_active", ubyte_type, "Checkpoint active flag"),
        (0x14B, 1, "checkpoint_powerup_state", ubyte_type, "Saved powerup state, restored to PlayerState+0x18"),
        
        # HUD/menu entity (0x14C-0x14F)
        (0x14C, 4, "hud_entity_ptr", ptr_type, "HUD entity pointer (CreateMenuEntities)"),
        
        # Pause system (0x150-0x163) - from PauseGameAndShowMenu
        (0x150, 1, "menu_active", ubyte_type, "Pause menu active flag"),
        (0x151, 1, "fade_out_active", ubyte_type, "Fade-out in progress flag"),
        (0x152, 1, "demo_return_flag", ubyte_type, "Return from demo to menu flag"),
        (0x153, 1, "field_153", ubyte_type, "Unknown field"),
        (0x154, 4, "saved_frame_counter", int_type, "Saved frame counter for pause restore"),
        (0x158, 1, "saved_freeze_flag", ubyte_type, "Saved pause state byte"),
        (0x159, 1, "field_159", ubyte_type, "Unknown field"),
        (0x15A, 2, "field_15A", short_type, "Unknown field"),
        (0x15C, 4, "saved_tick_list", ptr_type, "Saved tick list for pause restore"),
        (0x160, 1, "pause_countdown", ubyte_type, "Pause fade countdown (0x16 = 22 frames)"),
        (0x161, 1, "spawn_freeze_flag", ubyte_type, "Spawn freezing flag (set by checkpoint)"),
        (0x162, 2, "field_162", short_type, "Unknown field"),
        
        # Alternate entity spawn system (0x164-0x16F) - from SpawnEntitiesAlternateSystem @ 0x80081d0c
        (0x164, 4, "alternate_entity_data", ptr_type, "128-byte stride entity array (from GetVehicleDataPtr)"),
        (0x168, 2, "alternate_entity_count", ushort_type, "Count of alternate entities (TileHeader field 16)"),
        (0x16A, 2, "field_16A", short_type, "Unknown field"),
        (0x16C, 4, "entity_pool_ptr", ptr_type, "Entity pool pointer (256-entry array)"),
        
        # Level active + password (0x170-0x17B)
        (0x170, 1, "level_active", ubyte_type, "Level active flag"),
        (0x171, 1, "password_levels_0", ubyte_type, "Password level list entry 0"),
        (0x172, 1, "password_levels_1", ubyte_type, "Password level list entry 1"),
        (0x173, 1, "password_levels_2", ubyte_type, "Password level list entry 2"),
        (0x174, 1, "password_levels_3", ubyte_type, "Password level list entry 3"),
        (0x175, 1, "password_levels_4", ubyte_type, "Password level list entry 4"),
        (0x176, 1, "password_levels_5", ubyte_type, "Password level list entry 5"),
        (0x177, 1, "password_levels_6", ubyte_type, "Password level list entry 6"),
        (0x178, 1, "password_levels_7", ubyte_type, "Password level list entry 7"),
        (0x179, 1, "password_levels_8", ubyte_type, "Password level list entry 8"),
        (0x17A, 1, "password_levels_9", ubyte_type, "Password level list entry 9"),
        (0x17B, 1, "password_level_count", ubyte_type, "Password level count (max 10)"),
        
        # Score display array (0x17C-0x18B) - 8 entries x 2 bytes
        (0x17C, 2, "score_display_0", ushort_type, "Score display digit 0"),
        (0x17E, 2, "score_display_1", ushort_type, "Score display digit 1"),
        (0x180, 2, "score_display_2", ushort_type, "Score display digit 2"),
        (0x182, 2, "score_display_3", ushort_type, "Score display digit 3"),
        (0x184, 2, "score_display_4", ushort_type, "Score display digit 4"),
        (0x186, 2, "score_display_5", ushort_type, "Score display digit 5"),
        (0x188, 2, "score_display_6", ushort_type, "Score display digit 6"),
        (0x18A, 2, "score_display_7", ushort_type, "Score display digit 7"),
        
        # More flags (0x18C-0x198)
        (0x18C, 1, "field_18C", ubyte_type, "Unknown field"),
        (0x18D, 1, "player_readd_flag", ubyte_type, "Player re-add to render list flag (UnpauseGame)"),
        (0x18E, 1, "boss_player_type", ubyte_type, "Boss player type index (0=KLOGG,1=BIRDHEAD,2=JOE_HEAD)"),
        (0x18F, 1, "field_18F", ubyte_type, "Unknown field"),
        (0x190, 4, "field_190", int_type, "Unknown field"),
        (0x194, 4, "field_194", int_type, "Unknown field"),
        (0x198, 1, "field_198", ubyte_type, "Unknown field"),
        
        # Background colors (0x199-0x19B)
        (0x199, 1, "bg_color_r", ubyte_type, "Background R component (default 0x40)"),
        (0x19A, 1, "bg_color_g", ubyte_type, "Background G component (default 0x40)"),
        (0x19B, 1, "bg_color_b", ubyte_type, "Background B component (default 0x40)"),
        
        # Scrolling layer (0x19C-0x19F)
        (0x19C, 1, "scrolling_layer_active", ubyte_type, "Scrolling layer active (level flags bit 1)"),
        (0x19D, 1, "direct_stage_index", ubyte_type, "Direct stage index for SetupAndStartLevel"),
        (0x19E, 2, "field_19E", short_type, "Unknown field (padding to 0x1A0)"),
    ]
    
    # Grow struct to full size first
    struct.growStructure(GAMESTATE_SIZE)
    
    # Add all fields
    for offset, size, name, dtype, comment in fields:
        try:
            struct.replaceAtOffset(offset, dtype, size, name, comment)
            print("Added field: 0x%02X %s" % (offset, name))
        except Exception as e:
            print("Warning: Could not add %s at 0x%02X: %s" % (name, offset, str(e)))
    
    # Try to embed LevelDataContext at 0x84
    if level_ctx is not None:
        try:
            struct.replaceAtOffset(0x84, level_ctx, 128, "level_context", 
                                   "Embedded LevelDataContext (128 bytes)")
            print("Embedded LevelDataContext at offset 0x84")
        except Exception as e:
            print("Warning: Could not embed LevelDataContext: %s" % str(e))
    else:
        print("Note: LevelDataContext not found, leaving 0x84-0x103 as undefined")
    
    # Add to data type manager
    dtm.addDataType(struct, None)
    print("GameState struct created successfully (%d bytes)" % struct.getLength())
    
    return struct

def apply_type_to_address(program, addr, dtype):
    """Apply a data type to an address."""
    listing = program.getListing()
    try:
        listing.clearCodeUnits(addr, addr.add(dtype.getLength() - 1), False)
        listing.createData(addr, dtype)
        print("Applied %s to address %s" % (dtype.getName(), addr))
        return True
    except Exception as e:
        print("Warning: Could not apply type at %s: %s" % (addr, str(e)))
        return False

def create_label(program, addr, name):
    """Create or update a label at an address."""
    sym_table = program.getSymbolTable()
    existing = sym_table.getPrimarySymbol(addr)
    if existing is not None:
        if existing.getName() != name:
            existing.setName(name, SourceType.USER_DEFINED)
            print("Renamed symbol at %s to %s" % (addr, name))
    else:
        sym_table.createLabel(addr, name, SourceType.USER_DEFINED)
        print("Created label %s at %s" % (name, addr))

def run():
    """Main script entry point."""
    print("=" * 60)
    print("GameState Struct Fixer for Skullmonkeys")
    print("=" * 60)
    
    dtm = currentProgram.getDataTypeManager()
    
    # Create the struct
    struct = create_gamestate_struct(dtm)
    
    # Apply to g_GameState address
    addr = toAddr(GAMESTATE_ADDR)
    
    # Create/update label
    create_label(currentProgram, addr, "g_GameState")
    
    # Apply the struct type
    apply_type_to_address(currentProgram, addr, struct)
    
    print("=" * 60)
    print("Done! GameState struct applied to 0x%08X" % GAMESTATE_ADDR)
    print("=" * 60)
    print("")
    print("Next steps:")
    print("1. Run Auto-Analyze to propagate types")
    print("2. Check decompiled functions for improved readability")
    print("3. If LevelDataContext wasn't embedded, do it manually at offset 0x84")

# Run the script
if __name__ == "__main__":
    run()
