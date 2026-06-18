-- -----------------------------------------------------------------------------
-- GameState
-- Main game state structure at 0x8009DC40. Size 0x1A0 (416 bytes). Contains embedded LevelDataContext at offset 0x84.
-- Size: 72 bytes (0x48)
-- Exported from Ghidra via: python3 scripts/decompile.py --export-lua GameState
-- -----------------------------------------------------------------------------

local GameState_offsets = {
    _size = 0x48,  -- Total struct size
    mode_base_offset = 0x00, -- int, Always 0xFFFF0000, sets entity callback mode
    mode_callback_ptr = 0x04, -- ptr, GameModeCallback function pointer
    layer_list_static = 0x08, -- ptr, Static layer list head (AddLayerToRenderList_Static)
    layer_list_scrolling = 0x0C, -- ptr, Scrolling layer list head (AddLayerToRenderList_Scrolling)
    layer_list_parallax = 0x10, -- ptr, Parallax layer list head (AddLayerToRenderList_Parallax)
    layer_list_standard = 0x14, -- ptr, Standard layer list head (AddLayerToRenderList_Standard)
    layer_render_context_ptr = 0x18, -- ptr, Layer render context pointer
    tick_list_head = 0x1C, -- ptr, Z-sorted entity tick list (iterated by EntityTickLoop)
    render_list_head = 0x20, -- ptr, Z-sorted entity render list
    collision_list_head = 0x24, -- ptr, Entity collision/update queue head
    entity_spawn_list = 0x28, -- ptr, Raw 24-byte entity defs from Asset 501
    player_entity_alt = 0x2C, -- ptr, Player entity alternate reference
    player_entity_ptr = 0x30, -- ptr, Main player entity pointer
    pending_removal_entity = 0x34, -- ptr, Entity marked for deferred removal
    removal_mode = 0x38, -- int, Removal mode: 0=all lists, 1=keep render
    previous_spawn_list = 0x3C, -- ptr, Previous spawn list (copied from 0x13C during setup)
    field_40 = 0x40, -- int, Unknown - cleared to 0 in InitGameState
    camera_x = 0x44, -- short, Camera X position (pixels)
    camera_y = 0x46, -- short, Camera Y position (pixels)
}

-- Usage in game_watcher.lua:
-- local value = read_u8(struct_addr + GameState_offsets.field_name)

return GameState_offsets

