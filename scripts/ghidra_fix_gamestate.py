#@category Skullmonkeys
#@menupath Analysis.Fix GameState Struct
#@description Recreates GameState struct with proper field offsets from decompilation analysis

"""
Ghidra Python script to fix/recreate the GameState struct.
Run this in Ghidra's Script Manager or via analyzeHeadless.

Updated: 2026-01-18
Based on verified runtime dumps and decompilation analysis of:
- InitGameState @ 0x8007CD34
- SetupAndStartLevel @ 0x8007D8A0
- SpawnPlayerAndEntities @ 0x8007DF38
- SaveCheckpointState @ 0x8007EAAC
- EntityTickLoop @ 0x80020E1C
- CheckCheatCodeInput @ 0x800820B4
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
        "Main game state structure at 0x8009DC40 (PAL). Size 0x1A0 (416 bytes). "
        "Initialized by InitGameState @ 0x8007CD34. "
        "LevelDataContext is embedded at offset 0x84 (128 bytes)."
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
    # 
    # Verified from:
    #   - Runtime memory dumps via game_watcher.lua
    #   - InitGameState @ 0x8007CD34
    #   - SetupAndStartLevel @ 0x8007D8A0
    #   - SpawnPlayerAndEntities @ 0x8007DF38
    #   - SaveCheckpointState @ 0x8007EAAC
    #   - RestoreCheckpointEntities @ 0x8007EAEC
    #   - EntityTickLoopWithCamera @ 0x80020B34
    #   - UpdateCameraPositionSmooth @ 0x800233C0
    #   - InitLayersAndTileState @ 0x80024778
    #   - InitTileAttributeState @ 0x80024CF4
    #   - ClearTickAndRenderLists @ 0x80020C54
    #   - MarkEntityForDeferredRemoval @ 0x80020D74
    #   - LoadTileDataToVRAM @ 0x80025240
    #   - CheckCheatCodeInput @ 0x800820B4
    #   - PauseGameAndShowMenu @ 0x80082310
    # ============================================================
    fields = [
        # Core mode/callback (0x00-0x07)
        (0x00, 4, "mode_base_offset", int_type, "Always 0xFFFF0000, sets entity callback mode"),
        (0x04, 4, "mode_callback_ptr", ptr_type, "GameModeCallback function pointer"),
        
        # Layer list heads (0x08-0x18) - from ClearTickAndRenderLists @ 0x80020C54
        # Each layer type has its own linked list for z-sorting with entities
        (0x08, 4, "layer_list_static", ptr_type, "Static layer list head (AddLayerToRenderList_Static)"),
        (0x0C, 4, "layer_list_scrolling", ptr_type, "Scrolling layer list head (AddLayerToRenderList_Scrolling)"),
        (0x10, 4, "layer_list_parallax", ptr_type, "Parallax layer list head (AddLayerToRenderList_Parallax)"),
        (0x14, 4, "layer_list_standard", ptr_type, "Standard layer list head (AddLayerToRenderList_Standard)"),
        (0x18, 4, "layer_render_context_ptr", ptr_type, "Layer render context pointer"),
        
        # Entity lists (0x1C-0x33)
        (0x1C, 4, "tick_list_head", ptr_type, "Z-sorted entity tick list (iterated by EntityTickLoop)"),
        (0x20, 4, "render_list_head", ptr_type, "Z-sorted entity render list"),
        (0x24, 4, "collision_list_head", ptr_type, "Entity collision/update queue head"),
        (0x28, 4, "entity_spawn_list", ptr_type, "Raw 24-byte entity defs from Asset 501"),
        (0x2C, 4, "player_entity_alt", ptr_type, "Player entity (alternate reference for collision)"),
        (0x30, 4, "player_entity_ptr", ptr_type, "Main player entity pointer"),
        
        # Deferred entity removal (0x34-0x3B) - from MarkEntityForDeferredRemoval @ 0x80020D74
        (0x34, 4, "pending_removal_entity", ptr_type, "Entity marked for deferred removal"),
        (0x38, 4, "removal_mode", int_type, "Removal mode: 0=all lists, 1=keep render, 2=normal"),
        
        # Previous spawn list (0x3C-0x43) - from SetupAndStartLevel @ 0x8007D8A0
        (0x3C, 4, "previous_spawn_list", ptr_type, "Previous spawn list (copied from 0x13C during setup)"),
        (0x40, 4, "blb_header_ptr", ptr_type, "Pointer to BLB header buffer (0x800AE3E0 after boot)"),
        
        # Camera position (0x44-0x4B) - from UpdateCameraPositionSmooth @ 0x800233C0
        (0x44, 2, "camera_x", short_type, "Camera X position (pixels)"),
        (0x46, 2, "camera_y", short_type, "Camera Y position (pixels)"),
        (0x48, 2, "camera_limit_x", short_type, "Level width for camera X clamping"),
        (0x4A, 2, "camera_limit_y", short_type, "Level height for camera Y clamping"),
        
        # Camera velocity (0x4C-0x53) - 16.16 fixed-point
        (0x4C, 4, "camera_velocity_x", int_type, "Camera X velocity (16.16 fixed-point)"),
        (0x50, 4, "camera_velocity_y", int_type, "Camera Y velocity (16.16 fixed-point)"),
        
        # Player render offset (0x54-0x57)
        (0x54, 2, "player_render_offset_x", short_type, "Player render offset X"),
        (0x56, 2, "player_render_offset_y", short_type, "Player render offset Y"),
        
        # Scroll limit flags (0x58-0x5B)
        (0x58, 1, "scroll_limit_left", ubyte_type, "Scroll limit left flag"),
        (0x59, 1, "scroll_limit_top", ubyte_type, "Scroll limit top flag"),
        (0x5A, 1, "scroll_limit_right", ubyte_type, "Scroll limit right flag"),
        (0x5B, 1, "scroll_limit_bottom", ubyte_type, "Scroll limit bottom flag"),
        
        # Camera sub-pixel (0x5C-0x5F)
        (0x5C, 2, "camera_subpixel_x", ushort_type, "Camera sub-pixel X"),
        (0x5E, 2, "camera_subpixel_y", ushort_type, "Camera sub-pixel Y"),
        
        # Camera mode flags (0x60-0x63)
        (0x60, 1, "camera_y_lock", ubyte_type, "Camera Y lock flag"),
        (0x61, 1, "camera_mode_flags", ubyte_type, "Camera mode flags"),
        (0x62, 1, "camera_invert_x", ubyte_type, "Camera invert X flag"),
        (0x63, 1, "pause_freeze_flag", ubyte_type, "Pause freeze flag (also set by SaveCheckpointState)"),
        
        # Player hitbox (0x64-0x67)
        (0x64, 2, "player_hitbox_width", short_type, "Player hitbox width (0x28 normal, 0 boss/glide)"),
        (0x66, 2, "player_hitbox_y_offset", short_type, "Player hitbox Y offset (-48 normal, -80 FINN, 0 glide)"),
        
        # Tile collision state (0x68-0x73) - from InitTileAttributeState @ 0x80024CF4
        (0x68, 4, "tile_collision_data_ptr", ptr_type, "Tile collision attribute array (Asset 500)"),
        (0x6C, 2, "tile_collision_offset_x", short_type, "Tile collision offset X (tiles)"),
        (0x6E, 2, "tile_collision_offset_y", short_type, "Tile collision offset Y (tiles)"),
        (0x70, 2, "tile_collision_width", short_type, "Tile collision map width (tiles)"),
        (0x72, 2, "tile_collision_height", short_type, "Tile collision map height (tiles)"),
        
        # Level/layer state (0x74-0x7B) - from InitLayersAndTileState @ 0x80024778
        (0x74, 4, "animated_tile_data_ptr", ptr_type, "LevelDataContext+0x3C (AnimOffsets for animated tiles)"),
        (0x78, 2, "scroll_limit_height", ushort_type, "TileHeader+0x1C (vertical scroll limit, 0 for bosses)"),
        (0x7A, 2, "_pad_7A", ushort_type, "Padding to 4-byte alignment"),
        
        # Entity callback system (0x7C-0x83)
        (0x7C, 4, "entity_callback_table", ptr_type, "EntityTypeCallback[121] at 0x8009D5F8"),
        (0x80, 2, "entity_type_count", ushort_type, "Always 0x79 (121 entity types)"),
        (0x82, 2, "_pad_82", ushort_type, "Padding to 4-byte alignment"),
        
        # LevelDataContext embedded at 0x84 (128 bytes) - added separately below
        
        # Post-LevelDataContext fields (0x104-0x10F)
        (0x104, 2, "tile_render_state_count", ushort_type, "Total tile count + 1 (from GetTotalTileCount)"),
        (0x106, 2, "_pad_106", ushort_type, "Padding to 4-byte alignment"),
        (0x108, 4, "tile_render_state_ptr", ptr_type, "Tile lookup table (8 bytes per tile: TPage, CLUT, UV)"),
        (0x10C, 4, "frame_counter", int_type, "Frame counter (incremented in EntityTickLoopWithCamera)"),
        
        # Palette/spawn info (0x110-0x11F) - from LoadTileDataToVRAM @ 0x80025240
        (0x110, 4, "palette_group_ptrs", ptr_type, "Palette render context pointer array"),
        (0x114, 1, "palette_group_count", ubyte_type, "Count of palette groups"),
        (0x115, 1, "_pad_115", ubyte_type, "Padding to 2-byte alignment"),
        (0x116, 2, "spawn_x", short_type, "Player spawn X position"),
        (0x118, 2, "spawn_y", short_type, "Player spawn Y position"),
        (0x11A, 2, "screen_shake_index", ushort_type, "Index into shake lookup table"),
        (0x11C, 4, "camera_scroll_speed", int_type, "Camera scroll speed (0x8000, 0xC000, or 0x10000)"),
        
        # Special mode state (0x120-0x12F) - glide/boss/FINN
        (0x120, 2, "glide_boss_state_x", short_type, "Glide/boss/FINN state X (zeroed in SpawnPlayerAndEntities)"),
        (0x122, 2, "glide_boss_state_y", short_type, "Glide/boss/FINN state Y (zeroed in SpawnPlayerAndEntities)"),
        (0x124, 1, "layer0_tint_r", ubyte_type, "Layer 0 R color tint (from LayerEntry+0x2C)"),
        (0x125, 1, "layer0_tint_g", ubyte_type, "Layer 0 G color tint (from LayerEntry+0x2D)"),
        (0x126, 1, "layer0_tint_b", ubyte_type, "Layer 0 B color tint (from LayerEntry+0x2E)"),
        (0x127, 1, "layer1_tint_r", ubyte_type, "Layer 1 R color tint"),
        (0x128, 1, "layer1_tint_g", ubyte_type, "Layer 1 G color tint"),
        (0x129, 1, "layer1_tint_b", ubyte_type, "Layer 1 B color tint"),
        (0x12A, 1, "glide_boss_flag", ubyte_type, "Glide/boss/FINN flag (zeroed in SpawnPlayerAndEntities)"),
        (0x12B, 1, "_pad_12B", ubyte_type, "Padding to 4-byte alignment"),
        (0x12C, 4, "heap_debug_marker", int_type, "Debug sentinel (set to 0x01234567 during init)"),
        
        # Background color update (0x130-0x133) - from RenderEntities
        (0x130, 1, "bg_color_change_flag", ubyte_type, "Triggers BG color update in RenderEntities"),
        (0x131, 1, "bg_color_r_pending", ubyte_type, "Pending BG R component"),
        (0x132, 1, "bg_color_g_pending", ubyte_type, "Pending BG G component"),
        (0x133, 1, "bg_color_b_pending", ubyte_type, "Pending BG B component"),
        
        # Checkpoint system (0x134-0x14F)
        (0x134, 4, "checkpoint_entity_list", ptr_type, "Saved entity tick list for checkpoint respawn"),
        (0x138, 4, "checkpoint_saved_score", int_type, "Saves frame_counter at checkpoint"),
        (0x13C, 4, "entity_defs_backup", ptr_type, "Backed up entity defs ptr in ClearEntitiesAndFadeToBlack"),
        (0x140, 4, "input_state_ptr", ptr_type, "Input state pointer, also stored in player entity[0x40]"),
        (0x144, 1, "level_clear_pending", ubyte_type, "Level clear pending flag (triggers ClearEntitiesAndFadeToBlack)"),
        (0x145, 1, "level_transition_complete", ubyte_type, "Set after ClearEntitiesAndFadeToBlack runs"),
        (0x146, 1, "advance_level_flag", ubyte_type, "Advance to next level (calls AdvanceLevelSequence)"),
        (0x147, 1, "next_level_flag", ubyte_type, "Next level flag (set by SetNextLevelFlagWithFade)"),
        (0x148, 1, "direct_level_load", ubyte_type, "Direct level load flag"),
        (0x149, 1, "checkpoint_restore_pending", ubyte_type, "Checkpoint restore pending, zeroed after restore"),
        (0x14A, 1, "checkpoint_active", ubyte_type, "Checkpoint active flag, set by SaveCheckpointState"),
        (0x14B, 1, "checkpoint_powerup_state", ubyte_type, "Saved powerup state, restored to PlayerState+0x18 on respawn"),
        
        # HUD/menu entity (0x14C-0x14F)
        (0x14C, 4, "hud_entity_ptr", ptr_type, "HUD entity created by CreateMenuEntities"),
        
        # Pause system (0x150-0x163) - from PauseGameAndShowMenu
        (0x150, 1, "menu_active", ubyte_type, "Pause menu active flag"),
        (0x151, 1, "fade_out_active", ubyte_type, "Fade-out in progress flag"),
        (0x152, 1, "demo_return_flag", ubyte_type, "Return from demo to menu flag"),
        (0x153, 1, "_pad_153", ubyte_type, "Padding"),
        (0x154, 4, "saved_frame_counter", int_type, "Saved frame counter for pause restore"),
        (0x158, 1, "saved_freeze_flag", ubyte_type, "Saved pause state byte"),
        (0x159, 1, "_pad_159_0", ubyte_type, "Padding"),
        (0x15A, 1, "_pad_159_1", ubyte_type, "Padding"),
        (0x15B, 1, "_pad_159_2", ubyte_type, "Padding"),
        (0x15C, 4, "saved_tick_list", ptr_type, "Saved tick list for pause restore"),
        (0x160, 1, "pause_countdown", ubyte_type, "Pause fade countdown (0x16 = 22 frames)"),
        (0x161, 1, "spawn_freeze_flag", ubyte_type, "Spawn freezing flag (set by checkpoint)"),
        (0x162, 1, "_pad_162_0", ubyte_type, "Padding"),
        (0x163, 1, "_pad_162_1", ubyte_type, "Padding"),
        
        # Alternate entity spawn system (0x164-0x16F) - from SpawnEntitiesAlternateSystem @ 0x80081D0C
        (0x164, 4, "alternate_entity_data", ptr_type, "64-byte stride entity array (from GetVehicleDataPtr)"),
        (0x168, 2, "alternate_entity_count", ushort_type, "Count of alternate entities (TileHeader field 16)"),
        (0x16A, 2, "_pad_16A", ushort_type, "Padding to 4-byte alignment"),
        (0x16C, 4, "entity_pool_ptr", ptr_type, "Entity pool (256-entry array, allocated for special modes)"),
        
        # Level active + password (0x170-0x17B)
        (0x170, 1, "level_active", ubyte_type, "Set to 1 if level has valid asset index, 0 otherwise"),
        # password_levels[10] at 0x171-0x17A
        (0x171, 1, "password_levels_0", ubyte_type, "Password-selectable level list entry 0"),
        (0x172, 1, "password_levels_1", ubyte_type, "Password-selectable level list entry 1"),
        (0x173, 1, "password_levels_2", ubyte_type, "Password-selectable level list entry 2"),
        (0x174, 1, "password_levels_3", ubyte_type, "Password-selectable level list entry 3"),
        (0x175, 1, "password_levels_4", ubyte_type, "Password-selectable level list entry 4"),
        (0x176, 1, "password_levels_5", ubyte_type, "Password-selectable level list entry 5"),
        (0x177, 1, "password_levels_6", ubyte_type, "Password-selectable level list entry 6"),
        (0x178, 1, "password_levels_7", ubyte_type, "Password-selectable level list entry 7"),
        (0x179, 1, "password_levels_8", ubyte_type, "Password-selectable level list entry 8"),
        (0x17A, 1, "password_levels_9", ubyte_type, "Password-selectable level list entry 9"),
        (0x17B, 1, "password_level_count", ubyte_type, "Count of password-selectable levels"),
        
        # Cheat code input buffer (0x17C-0x18B) - 8 entries x 2 bytes
        # Used by CheckCheatCodeInput @ 0x800820B4 to store last 8 button presses.
        # NOT score display - it's a circular cheat input buffer.
        (0x17C, 2, "cheat_input_buffer_0", ushort_type, "Cheat input buffer entry 0 (button press)"),
        (0x17E, 2, "cheat_input_buffer_1", ushort_type, "Cheat input buffer entry 1"),
        (0x180, 2, "cheat_input_buffer_2", ushort_type, "Cheat input buffer entry 2"),
        (0x182, 2, "cheat_input_buffer_3", ushort_type, "Cheat input buffer entry 3"),
        (0x184, 2, "cheat_input_buffer_4", ushort_type, "Cheat input buffer entry 4"),
        (0x186, 2, "cheat_input_buffer_5", ushort_type, "Cheat input buffer entry 5"),
        (0x188, 2, "cheat_input_buffer_6", ushort_type, "Cheat input buffer entry 6"),
        (0x18A, 2, "cheat_input_buffer_7", ushort_type, "Cheat input buffer entry 7"),
        
        # Cheat/debug flags (0x18C-0x198) - from CheckCheatCodeInput @ 0x800820B4
        (0x18C, 1, "cheat_input_index", ubyte_type, "Circular index into cheat_input_buffer (0-7)"),
        (0x18D, 1, "player_readd_flag", ubyte_type, "Player re-add to render list flag (cheat 0x13, UnpauseGame)"),
        (0x18E, 1, "boss_player_type", ubyte_type, "Boss player type index (0=KLOGG,1=BIRDHEAD,2=JOE_HEAD)"),
        (0x18F, 1, "debug_pause_enable", ubyte_type, "Debug frame-step enable (cheat 0x0F toggles)"),
        (0x190, 1, "debug_pause_active", ubyte_type, "Debug frame-step state (toggled when debug_pause_enable set)"),
        (0x191, 1, "_pad_191_0", ubyte_type, "Padding"),
        (0x192, 1, "_pad_191_1", ubyte_type, "Padding"),
        (0x193, 1, "_pad_191_2", ubyte_type, "Padding"),
        (0x194, 4, "_reserved_194", int_type, "Reserved/unused - cleared in InitGameState, always 0"),
        (0x198, 1, "_reserved_198", ubyte_type, "Reserved/unused - cleared in InitGameState, always 0"),
        
        # Background colors (0x199-0x19B)
        (0x199, 1, "bg_color_r", ubyte_type, "Background R component (default 0x40)"),
        (0x19A, 1, "bg_color_g", ubyte_type, "Background G component (default 0x40)"),
        (0x19B, 1, "bg_color_b", ubyte_type, "Background B component (default 0x40)"),
        
        # Scrolling layer (0x19C-0x19F)
        (0x19C, 1, "scrolling_layer_active", ubyte_type, "Scrolling layer active (level flags bit 1)"),
        (0x19D, 1, "direct_stage_index", ubyte_type, "Direct stage index for SetupAndStartLevel"),
        (0x19E, 1, "_pad_19E", ubyte_type, "Padding to 0x1A0 total size"),
        (0x19F, 1, "_pad_19F", ubyte_type, "Padding to 0x1A0 total size"),
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
        print("      Run ghidra_fix_level_data_context.py first to create it")
    
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
    print("GameState Struct Fixer for Skullmonkeys (PAL)")
    print("Updated: 2026-01-18")
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
    print("Key offsets:")
    print("  0x00: mode_base_offset/mode_callback_ptr")
    print("  0x1C: tick_list_head (entity tick list)")
    print("  0x30: player_entity_ptr")
    print("  0x40: blb_header_ptr")
    print("  0x44: camera_x/camera_y")
    print("  0x7C: entity_callback_table")
    print("  0x84: level_context (embedded LevelDataContext, 128 bytes)")
    print("  0x10C: frame_counter")
    print("  0x116: spawn_x/spawn_y")
    print("  0x14C: hud_entity_ptr")
    print("  0x150: menu_active")
    print("  0x170: level_active")
    print("  0x17C: cheat_input_buffer[8]")
    print("  0x199: bg_color_r/g/b")
    print("")
    print("Next steps:")
    print("1. Run Auto-Analyze to propagate types")
    print("2. Check decompiled functions for improved readability")
    print("3. If LevelDataContext wasn't embedded, run ghidra_fix_level_data_context.py first")

# Run the script
if __name__ == "__main__":
    run()
