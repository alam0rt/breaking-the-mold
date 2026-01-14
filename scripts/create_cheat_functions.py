#!/usr/bin/env python3
"""
Ghidra Python Script: Create Functions for Cheat Code Callbacks

This script automatically creates and names functions for all 22 cheat code handlers
in CheckCheatCodeInput @ 0x800820B4.

Usage in Ghidra:
1. Window > Script Manager
2. Run this script
3. Or from command line: analyzeHeadless <project_path> <project_name> -process <program> -postScript create_cheat_functions.py
"""

from ghidra.program.model.symbol import SourceType
from ghidra.program.model.listing import CodeUnit

# Cheat code information based on online cheat list and reverse engineering
CHEAT_CODES = [
    {"index": 0x00, "name": "Cheat_RemovePauseTextAndInventory", "addr": None, "desc": "Remove Pause Text & Inventory Screen"},
    {"index": 0x01, "name": "Cheat_Unknown01", "addr": None, "desc": "Unknown cheat (no handler)"},
    {"index": 0x02, "name": "Cheat_MaxItemsMostTypes", "addr": None, "desc": "Max Items (most types included)"},
    {"index": 0x03, "name": "Cheat_GetAllSwirlyQs", "addr": None, "desc": "Get all Swirly Q's immediately"},
    {"index": 0x04, "name": "Cheat_ExtraHalo", "addr": None, "desc": "Extra Halo (invincibility flag)"},
    {"index": 0x05, "name": "Cheat_MaxLives", "addr": None, "desc": "Max Lives (99)"},
    {"index": 0x06, "name": "Cheat_MaxUniverseEnemas", "addr": None, "desc": "Max Universe Enemas (powerup slot 3)"},
    {"index": 0x07, "name": "Cheat_MaxPhoenixHands", "addr": None, "desc": "Max Phoenix Hands (powerup slot 1)"},
    {"index": 0x08, "name": "Cheat_MaxSuperWillies", "addr": None, "desc": "Max Super Willies (powerup slot 4)"},
    {"index": 0x09, "name": "Cheat_MaxPhartHeads", "addr": None, "desc": "Max Phart Heads (powerup slot 2)"},
    {"index": 0x0A, "name": "Cheat_MaxGreenBullets", "addr": None, "desc": "Max Green Bullets (3 green orbs)"},
    {"index": 0x0B, "name": "Cheat_Max1970sItems", "addr": None, "desc": "Max 1970s Items (3 '1970' icons)"},
    {"index": 0x0C, "name": "Cheat_LevelSkip", "addr": None, "desc": "Level Skip (advance to next stage)"},
    {"index": 0x0D, "name": "Cheat_WarpToMenu", "addr": None, "desc": "Warp to stage 99 (menu)"},
    {"index": 0x0E, "name": "Cheat_ToggleInvincibility", "addr": None, "desc": "Toggle invincibility (g_GameFlags ^= 1)"},
    {"index": 0x0F, "name": "Cheat_TogglePauseMenuVisibility", "addr": None, "desc": "Toggle pause menu visibility"},
    {"index": 0x10, "name": "Cheat_TintedKlaymen", "addr": None, "desc": "Tinted Klaymen (rainbow RGB effect)"},
    {"index": 0x11, "name": "Cheat_HazyKlaymen", "addr": None, "desc": "Hazy Klaymen (entity+0x1AF flag)"},
    {"index": 0x12, "name": "Cheat_SlowFastGameplay", "addr": None, "desc": "Slow/Fast Gameplay (toggle frame skip)"},
    {"index": 0x13, "name": "Cheat_PlayerRespawn", "addr": None, "desc": "Player respawn (re-add flag)"},
    {"index": 0x14, "name": "Cheat_MiniKlaymen", "addr": None, "desc": "Mini Klaymen (entity+0x1B0 flag)"},
    {"index": 0x15, "name": "Cheat_FireKlaymensHeads", "addr": None, "desc": "Fire Klaymen's Heads (entity+0x1B1 flag)"},
]

def get_address(addr_str):
    """Convert hex string to Ghidra Address"""
    return currentProgram.getAddressFactory().getAddress(addr_str)

def read_switch_table():
    """Read the switch jump table at 0x80012700"""
    table_addr = get_address("0x80012700")
    listing = currentProgram.getListing()
    
    print("[*] Reading switch table at 0x80012700...")
    
    for i in range(0x16):  # 22 entries (0x00 to 0x15)
        entry_addr = table_addr.add(i * 4)
        data = listing.getDataAt(entry_addr)
        
        if data is not None:
            # Get the pointer value
            target_addr = data.getValue()
            if target_addr is not None:
                CHEAT_CODES[i]["addr"] = target_addr
                print("  [+] Case 0x{:02X}: {} -> 0x{}".format(
                    i, CHEAT_CODES[i]["name"], target_addr))
        else:
            print("  [-] Case 0x{:02X}: No data at table entry".format(i))

def create_cheat_functions():
    """Create labels and comments for cheat handler addresses"""
    function_manager = currentProgram.getFunctionManager()
    listing = currentProgram.getListing()
    symbol_table = currentProgram.getSymbolTable()
    
    print("\n[*] Creating cheat handler labels and comments...")
    
    labeled_count = 0
    
    for cheat in CHEAT_CODES:
        if cheat["addr"] is None:
            continue
            
        addr = cheat["addr"]
        name = cheat["name"]
        desc = cheat["desc"]
        
        # Check if this address is inside an existing function
        containing_func = function_manager.getFunctionContaining(addr)
        
        if containing_func is not None:
            # This is an inline case within CheckCheatCodeInput
            # Create a label instead of a function
            try:
                # Remove any existing default labels
                existing_symbols = symbol_table.getSymbols(addr)
                for sym in existing_symbols:
                    if sym.getName().startswith("case_") or sym.getName().startswith("switchD_"):
                        symbol_table.removeSymbolSpecial(sym)
                
                # Create new label
                symbol_table.createLabel(addr, name, SourceType.USER_DEFINED)
                print("  [+] Created label: {} @ 0x{} (inline case)".format(name, addr))
                labeled_count += 1
                
                # Set EOL comment at the address
                code_unit = listing.getCodeUnitAt(addr)
                if code_unit is not None:
                    comment = "Cheat 0x{:02X}: {}".format(cheat["index"], desc)
                    code_unit.setComment(CodeUnit.EOL_COMMENT, comment)
                    
            except Exception as e:
                print("  [-] Error creating label at 0x{}: {}".format(addr, str(e)))
        else:
            # This address is not in a function - try to create a function
            try:
                new_func = function_manager.createFunction(
                    name,
                    addr,
                    None,  # Let Ghidra determine body
                    SourceType.USER_DEFINED
                )
                
                if new_func is not None:
                    print("  [+] Created function: {} @ 0x{}".format(name, addr))
                    labeled_count += 1
                    
                    # Set plate comment
                    comment = """Cheat Code Handler: {}

Index: 0x{:02X}
Called from: CheckCheatCodeInput @ 0x800820B4 (switch case 0x{:02X})

This handler is activated when the player enters the correct 8-button
sequence for this cheat code during the pause menu.

Cheat Effect: {}

Verified via reverse engineering 2025-01-14.""".format(
                        desc,
                        cheat["index"],
                        cheat["index"],
                        desc
                    )
                    
                    new_func.setComment(comment)
                else:
                    print("  [-] Failed to create function: {} @ 0x{}".format(name, addr))
                    
            except Exception as e:
                # If function creation fails, create a label instead
                try:
                    symbol_table.createLabel(addr, name, SourceType.USER_DEFINED)
                    print("  [+] Created label (fallback): {} @ 0x{}".format(name, addr))
                    labeled_count += 1
                    
                    code_unit = listing.getCodeUnitAt(addr)
                    if code_unit is not None:
                        comment = "Cheat 0x{:02X}: {}".format(cheat["index"], desc)
                        code_unit.setComment(CodeUnit.EOL_COMMENT, comment)
                except Exception as e2:
                    print("  [-] Error at 0x{}: {}".format(addr, str(e2)))
    
    return labeled_count

def main():
    """Main script entry point"""
    print("=" * 70)
    print("Cheat Code Function Creator")
    print("=" * 70)
    print()
    
    # Read the switch table
    read_switch_table()
    
    # Create labels/functions for cheat handlers
    labeled_count = create_cheat_functions()
    
    print()
    print("=" * 70)
    print("Summary:")
    print("  - Total cheat codes: 22 (0x00 to 0x15)")
    print("  - Labels/functions created: {}".format(labeled_count))
    print("=" * 70)

if __name__ == "__main__":
    main()
