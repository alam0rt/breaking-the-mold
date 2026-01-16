-- -----------------------------------------------------------------------------
-- PlayerState
-- Player state structure initialized by initPlayerState @ 0x8001F3B4
-- Size: 30 bytes (0x1E)
-- Exported from Ghidra via: python3 scripts/decompile.py --export-lua PlayerState
-- -----------------------------------------------------------------------------

local PlayerState_offsets = {
    _size = 0x1E,  -- Total struct size
    initialized = 0x00, -- u8, State initialized (1)
    active = 0x01, -- u8, Player is active (1)
    unknown02 = 0x02, -- u16, Unknown 16-bit value (cleared)
    unknown04 = 0x04, -- u8, Unknown (cleared by init)
    total_1ups = 0x05, -- u8, Total 1-ups collected (returned on reset)
    clayball_flag_0 = 0x06, -- u8, Clayball collection flag for stage 0
    clayball_flag_1 = 0x07, -- u8, Clayball collection flag for stage 1
    clayball_flag_2 = 0x08, -- u8, Clayball collection flag for stage 2
    clayball_flag_3 = 0x09, -- u8, Clayball collection flag for stage 3
    clayball_flag_4 = 0x0A, -- u8, Clayball collection flag for stage 4
    clayball_flag_5 = 0x0B, -- u8, Clayball collection flag for stage 5
    clayball_flag_6 = 0x0C, -- u8, Clayball collection flag for stage 6
    clayball_flag_7 = 0x0D, -- u8, Clayball collection flag for stage 7
    clayball_flag_8 = 0x0E, -- u8, Clayball collection flag for stage 8
    clayball_flag_9 = 0x0F, -- u8, Clayball collection flag for stage 9
    level_complete = 0x10, -- u8, Level complete flag
    lives = 0x11, -- u8, Current lives (default: 5)
    orb_count = 0x12, -- u8, Clay/orb count (100 → 1up)
    swirly_q_count = 0x13, -- u8, Swirly Q count (3 opens bonus portal, max 20 via cheat). NOT Green Bullets!
    phoenix_hands = 0x14, -- u8, Phoenix Hand count (max 7, L1) - homing bird attack
    phart_heads = 0x15, -- u8, Phart Head count (max 7, L2) - ghostly scout clone
    universe_enemas = 0x16, -- u8, Universe Enema count (max 7, R1) - screen-clear weapon
    powerup_flags = 0x17, -- u8, Powerup flags: 0x01=Halo (1-hit shield), 0x02=Yellow Bird (glide)
    shrink_mode = 0x18, -- u8, Player is shrunk (mini mode)
    icon_1970_count = 0x19, -- u8, 1970 icon count (max 3)
    hamster_count = 0x1A, -- u8, Hamster shields orbiting player (max 3). Game mislabels as "Green Bullets" in cheat!
    total_swirly_qs = 0x1B, -- u8, Total Swirly Qs collected across all stages (48=0x30 for secret ending)
    super_willies = 0x1C, -- u8, Super Willie count (max 7, R2) - auto-collect all items on screen
    unknown_1d = 0x1D, -- u8, Cleared on death
}

-- Usage in game_watcher.lua:
-- local value = read_u8(struct_addr + PlayerState_offsets.field_name)

return PlayerState_offsets

