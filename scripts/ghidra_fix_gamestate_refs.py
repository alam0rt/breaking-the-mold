#@category Skullmonkeys
#@menupath Analysis.Fix GameState References
#@description Improves decompiler output by creating data references for GameState fields

"""
This script helps Ghidra recognize GameState field accesses by:
1. Finding instructions that reference the GameState base address region
2. Creating explicit data references to field addresses
3. Adding equates for common offset values

Run AFTER ghidra_fix_gamestate.py has created the struct.
"""

from ghidra.program.model.symbol import SourceType, RefType
from ghidra.program.model.scalar import Scalar

# GameState location
GAMESTATE_BASE = 0x8009DC40
GAMESTATE_SIZE = 0x1A0

# Field name lookup for creating better references
FIELD_NAMES = {
    0x00: "mode_base_offset",
    0x04: "mode_callback_ptr",
    0x1C: "tick_list_head",
    0x20: "render_list_head",
    0x24: "collision_list_head",
    0x28: "entity_spawn_list",
    0x2C: "player_entity_alt",
    0x30: "player_entity_ptr",
    0x34: "entity_pending_removal",
    0x38: "removal_mode",
    0x44: "camera_x",
    0x46: "camera_y",
    0x48: "level_width_limit",
    0x4A: "level_height_limit",
    0x4C: "camera_velocity_x",
    0x50: "camera_velocity_y",
    0x54: "player_render_offset_x",
    0x56: "player_render_offset_y",
    0x58: "scroll_limit_left",
    0x59: "scroll_limit_top",
    0x5A: "scroll_limit_right",
    0x5B: "scroll_limit_bottom",
    0x5C: "camera_subpixel_x",
    0x5E: "camera_subpixel_y",
    0x60: "camera_vertical_lock",
    0x61: "camera_mode_flags",
    0x62: "camera_invert_flag",
    0x63: "pause_freeze_flag",
    0x64: "player_hitbox_width",
    0x66: "player_hitbox_y_offset",
    0x7C: "entity_callback_table",
    0x80: "entity_type_count",
    0x84: "level_context",
    0x104: "field_104",
    0x108: "field_108",
    0x10C: "frame_counter",
    0x116: "spawn_x",
    0x118: "spawn_y",
    0x11A: "screen_shake_index",
    0x11C: "camera_scroll_speed",
    0x120: "special_state_1",
    0x122: "special_state_2",
    0x12A: "special_mode_flag",
    0x130: "bg_color_update_flag",
    0x134: "checkpoint_entity_list",
    0x138: "checkpoint_saved_score",
    0x13C: "pending_player_entity",
    0x140: "input_state_ptr",
    0x149: "checkpoint_restore_pending",
    0x14A: "checkpoint_active",
    0x14C: "hud_entity_ptr",
    0x150: "pause_menu_active",
    0x152: "demo_playback_mode",
    0x154: "saved_frame_counter",
    0x158: "saved_pause_state",
    0x15C: "saved_tick_list",
    0x160: "pause_countdown",
    0x161: "spawn_freeze_flag",
    0x164: "vehicle_data_ptr",
    0x168: "tile_header_field_16",
    0x16C: "entity_pool_ptr",
    0x170: "level_active",
    0x171: "password_level_list",
    0x17B: "password_level_count",
    0x17C: "score_display",
    0x18D: "player_readd_flag",
    0x18E: "boss_player_type",
    0x199: "bg_color_r",
    0x19A: "bg_color_g",
    0x19B: "bg_color_b",
    0x19C: "boss_defeated",
    0x19D: "tile_header_field_1a",
}

def create_equates_for_offsets():
    """Create equates for GameState field offsets."""
    equate_table = currentProgram.getEquateTable()
    created = 0
    
    for offset, name in FIELD_NAMES.items():
        equate_name = "GS_%s" % name
        try:
            existing = equate_table.getEquate(equate_name)
            if existing is None:
                equate_table.createEquate(equate_name, offset)
                created += 1
        except:
            pass
    
    print("Created %d equates for GameState offsets" % created)

def find_gamestate_field_accesses():
    """
    Find instructions that access GameState fields by looking for
    the common MIPS pattern:
        lui   reg, 0x800A
        addiu/lw/sw reg, reg, offset  (where offset makes it land in GameState)
    """
    listing = currentProgram.getListing()
    ref_mgr = currentProgram.getReferenceManager()
    
    # The high part loaded for GameState region
    # 0x8009DC40 = 0x800A0000 - 0x23C0
    # So lui loads 0x800A, then offset is negative (-0x23C0 + field_offset)
    
    accesses = {}  # offset -> list of instruction addresses
    
    # Search all instructions
    inst_iter = listing.getInstructions(True)
    count = 0
    
    while inst_iter.hasNext() and count < 500000:
        inst = inst_iter.next()
        count += 1
        
        # Look for load/store instructions with offsets
        mnemonic = inst.getMnemonicString()
        if mnemonic in ['lw', 'lh', 'lhu', 'lb', 'lbu', 'sw', 'sh', 'sb']:
            # Check if any operand references GameState range
            for i in range(inst.getNumOperands()):
                refs = inst.getReferencesFrom()
                for ref in refs:
                    to_addr = ref.getToAddress().getOffset()
                    if GAMESTATE_BASE <= to_addr < GAMESTATE_BASE + GAMESTATE_SIZE:
                        offset = to_addr - GAMESTATE_BASE
                        if offset not in accesses:
                            accesses[offset] = []
                        accesses[offset].append(inst.getAddress())
    
    return accesses

def analyze_and_report():
    """Analyze GameState field usage and print a report."""
    print("=" * 70)
    print("GameState Field Access Analysis")
    print("=" * 70)
    
    accesses = find_gamestate_field_accesses()
    
    # Sort by offset
    sorted_offsets = sorted(accesses.keys())
    
    print("\nFields with direct references found:\n")
    for offset in sorted_offsets:
        addrs = accesses[offset]
        field_name = FIELD_NAMES.get(offset, "field_%02X" % offset)
        print("  0x%02X %-30s: %d references" % (offset, field_name, len(addrs)))
        # Show first few addresses
        for addr in addrs[:3]:
            func = getFunctionContaining(addr)
            func_name = func.getName() if func else "unknown"
            print("       -> %s in %s" % (addr, func_name))
        if len(addrs) > 3:
            print("       ... and %d more" % (len(addrs) - 3))
    
    # Find unknown fields that have references
    print("\n" + "=" * 70)
    print("Unknown fields with references (need investigation):")
    print("=" * 70)
    
    for offset in sorted_offsets:
        if offset not in FIELD_NAMES:
            addrs = accesses[offset]
            print("\n  0x%02X (unknown): %d references" % (offset, len(addrs)))
            for addr in addrs[:5]:
                func = getFunctionContaining(addr)
                func_name = func.getName() if func else "unknown"
                print("       -> %s in %s" % (addr, func_name))

def create_field_labels():
    """Create labels for each GameState field address."""
    sym_table = currentProgram.getSymbolTable()
    created = 0
    
    for offset, name in FIELD_NAMES.items():
        addr = toAddr(GAMESTATE_BASE + offset)
        label_name = "g_GameState_%s" % name
        
        # Check if label already exists
        existing = sym_table.getPrimarySymbol(addr)
        if existing is None or existing.getName().startswith("DAT_"):
            try:
                sym_table.createLabel(addr, label_name, SourceType.USER_DEFINED)
                created += 1
            except:
                pass
    
    print("Created %d field labels" % created)

def run():
    print("=" * 70)
    print("GameState Reference Fixer for Skullmonkeys")
    print("Base address: 0x%08X" % GAMESTATE_BASE)
    print("=" * 70)
    
    # Create equates for offsets (helps when viewing assembly)
    create_equates_for_offsets()
    
    # Analyze current references
    analyze_and_report()
    
    # Note: Creating individual field labels can fragment the struct view
    # Uncomment if you want labels at each field:
    # create_field_labels()
    
    print("\n" + "=" * 70)
    print("Done! Run Auto-Analyze to propagate changes.")
    print("=" * 70)

if __name__ == "__main__":
    run()
