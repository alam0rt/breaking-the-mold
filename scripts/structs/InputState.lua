-- -----------------------------------------------------------------------------
-- InputState
-- Input state structure for controller handling, processed by UpdateInputState @ 0x800259d4. Used at g_pPlayer1Input and g_pPlayer2Input.
-- Size: 20 bytes (0x14)
-- Exported from Ghidra via: python3 scripts/decompile.py --export-lua InputState
-- -----------------------------------------------------------------------------

local InputState_offsets = {
    _size = 0x14,  -- Total struct size
    buttons_held = 0x00, -- u16, Currently held buttons (PSX controller bitfield)
    buttons_pressed = 0x02, -- u16, Newly pressed this frame (edge detection)
    playback_data_ptr = 0x04, -- ptr, Pointer to playback buffer (demo mode)
    playback_active = 0x08, -- u8, Non-zero if replaying recorded input
    padding09 = 0x09, -- u8, Padding bytes
    padding0A = 0x0A, -- u8, Padding bytes
    padding0B = 0x0B, -- u8, Padding bytes
    recording_buffer = 0x0C, -- ptr, Pointer to recording buffer
    playback_index = 0x10, -- u16, Current playback position
    playback_timer = 0x12, -- u16, Frames until next input event
}

-- Usage in game_watcher.lua:
-- local value = read_u8(struct_addr + InputState_offsets.field_name)

return InputState_offsets

