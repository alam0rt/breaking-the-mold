#!/usr/bin/env python3
"""
Parse GameState raw hex dumps and analyze field changes.

Usage:
    python3 scripts/analyze_gamestate.py game_watcher/logs/gamestate_raw_*.txt
"""

import sys
from pathlib import Path

# GameState field definitions from game_state.h
# Format: (offset, size, name)
FIELDS = [
    (0x00, 4, "mode_base_offset"),
    (0x04, 4, "mode_callback_ptr"),
    (0x08, 4, "layer_list_static"),
    (0x0C, 4, "layer_list_scrolling"),
    (0x10, 4, "layer_list_parallax"),
    (0x14, 4, "layer_list_standard"),
    (0x18, 4, "layer_render_context_ptr"),
    (0x1C, 4, "tick_list_head"),
    (0x20, 4, "render_list_head"),
    (0x24, 4, "collision_list_head"),
    (0x28, 4, "entity_spawn_list"),
    (0x2C, 4, "player_entity_alt"),
    (0x30, 4, "player_entity_ptr"),
    (0x34, 4, "pending_removal_entity"),
    (0x38, 4, "removal_mode"),
    (0x3C, 4, "previous_spawn_list"),
    (0x40, 4, "blb_header_ptr"),  # Was field_40, now identified
    (0x44, 2, "camera_x"),
    (0x46, 2, "camera_y"),
    (0x48, 2, "camera_limit_x"),
    (0x4A, 2, "camera_limit_y"),
    (0x4C, 4, "camera_velocity_x"),
    (0x50, 4, "camera_velocity_y"),
    (0x54, 2, "player_render_offset_x"),
    (0x56, 2, "player_render_offset_y"),
    (0x58, 1, "scroll_limit_left"),
    (0x59, 1, "scroll_limit_top"),
    (0x5A, 1, "scroll_limit_right"),
    (0x5B, 1, "scroll_limit_bottom"),
    (0x5C, 2, "camera_subpixel_x"),
    (0x5E, 2, "camera_subpixel_y"),
    (0x60, 1, "camera_y_lock"),
    (0x61, 1, "camera_mode_flags"),
    (0x62, 1, "camera_invert_x"),
    (0x63, 1, "pause_freeze_flag"),
    (0x64, 2, "player_hitbox_width"),
    (0x66, 2, "player_hitbox_y_offset"),
    (0x68, 4, "tile_collision_data_ptr"),
    (0x6C, 2, "tile_collision_offset_x"),
    (0x6E, 2, "tile_collision_offset_y"),
    (0x70, 2, "tile_collision_width"),
    (0x72, 2, "tile_collision_height"),
    (0x74, 4, "level_context_field_3C"),
    (0x78, 2, "tile_header_field_1C"),
    (0x7A, 2, "_pad_7A"),  # Verified padding (always 0)
    (0x7C, 4, "entity_callback_table"),
    (0x80, 2, "entity_type_count"),
    (0x82, 2, "_pad_82"),  # Verified padding (always 0)
    # LevelDataContext at 0x84-0x103 (128 bytes) - skip individual fields
    (0x84, 128, "level_data_context"),
    (0x104, 2, "tile_render_state_count"),
    (0x106, 2, "_pad_106"),  # Verified padding (always 0)
    (0x108, 4, "tile_render_state_ptr"),
    (0x10C, 4, "frame_counter"),
    (0x110, 4, "palette_group_ptrs"),
    (0x114, 1, "palette_group_count"),
    (0x115, 1, "_pad_115"),  # Verified padding (always 0)
    (0x116, 2, "spawn_x"),
    (0x118, 2, "spawn_y"),
    (0x11A, 2, "screen_shake_index"),
    (0x11C, 4, "camera_scroll_speed"),
    (0x120, 2, "glide_boss_state_x"),
    (0x122, 2, "glide_boss_state_y"),
    (0x124, 1, "layer0_tint_r"),
    (0x125, 1, "layer0_tint_g"),
    (0x126, 1, "layer0_tint_b"),
    (0x127, 1, "layer1_tint_r"),  # Observed same as layer0 in dumps
    (0x128, 1, "layer1_tint_g"),  # Observed same as layer0 in dumps
    (0x129, 1, "layer1_tint_b"),  # Observed same as layer0 in dumps
    (0x12A, 1, "glide_boss_flag"),
    (0x12B, 1, "_pad_12B"),  # Verified padding (always 0)
    (0x12C, 4, "field_12C"),  # Changes during init, purpose unknown
    (0x130, 1, "bg_color_change_flag"),
    (0x131, 1, "bg_color_r_pending"),
    (0x132, 1, "bg_color_g_pending"),
    (0x133, 1, "bg_color_b_pending"),
    (0x134, 4, "checkpoint_entity_list"),
    (0x138, 4, "checkpoint_saved_score"),
    (0x13C, 4, "entity_defs_backup"),
    (0x140, 4, "input_state_ptr"),
    (0x144, 1, "level_clear_pending"),
    (0x145, 1, "level_transition_complete"),
    (0x146, 1, "advance_level_flag"),
    (0x147, 1, "next_level_flag"),
    (0x148, 1, "direct_level_load"),
    (0x149, 1, "checkpoint_restore_pending"),
    (0x14A, 1, "checkpoint_active"),
    (0x14B, 1, "checkpoint_powerup_state"),
    (0x14C, 4, "hud_entity_ptr"),
    (0x150, 1, "menu_active"),
    (0x151, 1, "fade_out_active"),
    (0x152, 1, "demo_return_flag"),
    (0x153, 1, "_pad_153"),  # Verified padding (always 0)
    (0x154, 4, "saved_frame_counter"),
    (0x158, 1, "saved_freeze_flag"),
    (0x159, 3, "_pad_159"),  # Verified padding (always 0)
    (0x15C, 4, "saved_tick_list"),
    (0x160, 1, "pause_countdown"),
    (0x161, 1, "spawn_freeze_flag"),
    (0x162, 2, "_pad_162"),  # Verified padding (always 0)
    (0x164, 4, "alternate_entity_data"),
    (0x168, 2, "alternate_entity_count"),
    (0x16A, 2, "_pad_16A"),  # Verified padding (always 0)
    (0x16C, 4, "entity_pool_ptr"),
    (0x170, 1, "level_active"),
    (0x171, 10, "password_levels"),
    (0x17B, 1, "password_level_count"),
    (0x17C, 16, "cheat_input_buffer"),
    (0x18C, 1, "cheat_input_index"),
    (0x18D, 1, "player_readd_flag"),
    (0x18E, 1, "boss_player_type"),
    (0x18F, 1, "debug_pause_enable"),
    (0x190, 1, "debug_pause_active"),
    (0x191, 3, "_pad_191"),  # Padding
    (0x194, 4, "field_194"),  # Cleared but purpose unknown
    (0x198, 1, "field_198"),  # Cleared but purpose unknown
    (0x199, 1, "bg_color_r"),
    (0x19A, 1, "bg_color_g"),
    (0x19B, 1, "bg_color_b"),
    (0x19C, 1, "scrolling_layer_active"),
    (0x19D, 1, "direct_stage_index"),
    (0x19E, 2, "_pad_19E"),  # Verified padding (always 0)
]

def parse_hex_line(line: str) -> tuple[int, bytes]:
    """Parse a line: 'frame_number hex_bytes'"""
    parts = line.strip().split(' ', 1)
    frame = int(parts[0])
    data = bytes.fromhex(parts[1])
    return frame, data

def extract_field(data: bytes, offset: int, size: int) -> int:
    """Extract a field value (little-endian)."""
    value = 0
    for i in range(size):
        if offset + i < len(data):
            value |= data[offset + i] << (i * 8)
    return value

def format_value(value: int, size: int) -> str:
    """Format value based on size."""
    if size == 1:
        return f"{value:02X}"
    elif size == 2:
        return f"{value:04X}"
    elif size == 4:
        return f"{value:08X}"
    else:
        return f"{value:X}"

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 scripts/analyze_gamestate.py <dump_file>")
        print("       python3 scripts/analyze_gamestate.py --changes <dump_file>")
        print("       python3 scripts/analyze_gamestate.py --unknown <dump_file>")
        sys.exit(1)
    
    show_changes = "--changes" in sys.argv
    show_unknown = "--unknown" in sys.argv
    
    # Find the dump file argument
    dump_file = None
    for arg in sys.argv[1:]:
        if not arg.startswith("--"):
            dump_file = arg
            break
    
    if not dump_file:
        print("Error: No dump file specified")
        sys.exit(1)
    
    # Read all dumps
    dumps = []
    with open(dump_file) as f:
        for line in f:
            if line.startswith("#"):
                continue
            if line.strip():
                frame, data = parse_hex_line(line)
                dumps.append((frame, data))
    
    print(f"Loaded {len(dumps)} dumps from {dump_file}")
    
    if show_unknown:
        # Show only unknown fields
        print("\n=== Unknown Fields ===")
        unknown_fields = [(o, s, n) for o, s, n in FIELDS if "field_" in n.lower()]
        
        for offset, size, name in unknown_fields:
            values = set()
            for frame, data in dumps:
                val = extract_field(data, offset, size)
                values.add(val)
            
            if len(values) == 1:
                val = list(values)[0]
                print(f"  0x{offset:03X} {name:30s} = {format_value(val, size)} (constant)")
            else:
                print(f"  0x{offset:03X} {name:30s} = {len(values)} unique values: {sorted(values)[:10]}")
    
    elif show_changes:
        # Show fields that changed between dumps
        print("\n=== Field Changes ===")
        
        prev_data = None
        for frame, data in dumps:
            if prev_data is not None:
                changes = []
                for offset, size, name in FIELDS:
                    old_val = extract_field(prev_data, offset, size)
                    new_val = extract_field(data, offset, size)
                    if old_val != new_val:
                        changes.append((name, offset, old_val, new_val, size))
                
                if changes:
                    print(f"\nFrame {frame}:")
                    for name, offset, old_val, new_val, size in changes:
                        marker = " <-- UNKNOWN" if "field_" in name.lower() else ""
                        print(f"  0x{offset:03X} {name:30s}: {format_value(old_val, size)} -> {format_value(new_val, size)}{marker}")
            
            prev_data = data
    
    else:
        # Default: show latest dump with all fields
        if dumps:
            frame, data = dumps[-1]
            print(f"\n=== Latest Dump (frame {frame}) ===")
            for offset, size, name in FIELDS:
                if size > 16:
                    continue  # Skip large blocks
                val = extract_field(data, offset, size)
                marker = " <-- UNKNOWN" if "field_" in name.lower() else ""
                print(f"  0x{offset:03X} {name:30s} = {format_value(val, size)}{marker}")

if __name__ == "__main__":
    main()
