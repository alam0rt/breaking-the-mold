# Ghidra Script: Convert Jump Trampolines to Labels
# @category Analysis  
# @description Removes function definitions from 8-byte jump trampolines, keeping them as code
#
# IMPORTANT DISTINCTION:
# - Jump TABLES are DATA (arrays of addresses, used by switch statements) → rodata in splat
# - Jump TRAMPOLINES are CODE (executable j+delay slot) → asm in splat, but not functions
#
# This script identifies functions that are just jump trampolines:
#   j target_address
#   ori a1, zero, parameter  (delay slot)
#
# And removes the function definition while keeping them as disassembled code.
# They remain in the `asm` section for splat - they are NOT converted to data.
#
# Why do this?
# - These inflate the function count (28 "functions" that are really one dispatcher)
# - They're not semantically functions - they're dispatch stubs
# - The real function is the target they jump to
#
# Usage:
# 1. Open the script manager in Ghidra (Window > Script Manager)
# 2. Create a new script with this content
# 3. Run it - it will show a preview first, then ask for confirmation

from ghidra.program.model.symbol import SymbolType, SourceType
from ghidra.program.model.listing import CodeUnit
from ghidra.program.model.data import DWordDataType, ArrayDataType

# Known trampoline patterns (addresses to convert)
# Format: (address, name, target)
TRAMPOLINES = [
    # S_SCA_OBJ group -> 0x8008fafc
    (0x8008FABC, "S_SCA_OBJ_5C", 0x8008fafc),
    (0x8008FAC4, "S_SCA_OBJ_64", 0x8008fafc),
    (0x8008FACC, "S_SCA_OBJ_6C", 0x8008fafc),
    (0x8008FAD4, "S_SCA_OBJ_74", 0x8008fafc),
    (0x8008FADC, "S_SCA_OBJ_7C", 0x8008fafc),
    (0x8008FAE4, "S_SCA_OBJ_84", 0x8008fafc),
    (0x8008FAEC, "S_SCA_OBJ_8C", 0x8008fafc),
    
    # S_SCA_OBJ group -> 0x8008fbc4
    (0x8008FB84, "S_SCA_OBJ_124", 0x8008fbc4),
    (0x8008FB8C, "S_SCA_OBJ_12C", 0x8008fbc4),
    (0x8008FB94, "S_SCA_OBJ_134", 0x8008fbc4),
    (0x8008FB9C, "S_SCA_OBJ_13C", 0x8008fbc4),
    (0x8008FBA4, "S_SCA_OBJ_144", 0x8008fbc4),
    (0x8008FBAC, "S_SCA_OBJ_14C", 0x8008fbc4),
    (0x8008FBB4, "S_SCA_OBJ_154", 0x8008fbc4),
    
    # SR_SV_OBJ group -> 0x80090020
    (0x8008FFE0, "SR_SV_OBJ_1EC", 0x80090020),
    (0x8008FFE8, "SR_SV_OBJ_1F4", 0x80090020),
    (0x8008FFF0, "SR_SV_OBJ_1FC", 0x80090020),
    (0x8008FFF8, "SR_SV_OBJ_204", 0x80090020),
    (0x80090000, "SR_SV_OBJ_20C", 0x80090020),
    (0x80090008, "SR_SV_OBJ_214", 0x80090020),
    (0x80090010, "SR_SV_OBJ_21C", 0x80090020),
    
    # SR_SV_OBJ group -> 0x800900f0
    (0x800900B0, "SR_SV_OBJ_2BC", 0x800900f0),
    (0x800900B8, "SR_SV_OBJ_2C4", 0x800900f0),
    (0x800900C0, "SR_SV_OBJ_2CC", 0x800900f0),
    (0x800900C8, "SR_SV_OBJ_2D4", 0x800900f0),
    (0x800900D0, "SR_SV_OBJ_2DC", 0x800900f0),
    (0x800900D8, "SR_SV_OBJ_2E4", 0x800900f0),
    (0x800900E0, "SR_SV_OBJ_2EC", 0x800900f0),
]

def run():
    """Main script entry point"""
    from ghidra.util.exception import CancelledException
    
    func_mgr = currentProgram.getFunctionManager()
    listing = currentProgram.getListing()
    
    # Preview mode first
    print("=" * 60)
    print("TRAMPOLINE CONVERSION PREVIEW")
    print("=" * 60)
    print()
    print("The following functions will be converted to data:")
    print()
    
    valid_trampolines = []
    
    for addr_int, name, target in TRAMPOLINES:
        addr = toAddr(addr_int)
        func = func_mgr.getFunctionAt(addr)
        
        if func is None:
            print("  SKIP: {} @ {} - No function found".format(name, addr))
            continue
        
        if func.getName() != name:
            print("  WARN: {} @ {} - Name mismatch (found: {})".format(name, addr, func.getName()))
        
        # Verify it's 8 bytes
        body = func.getBody()
        if body.getNumAddresses() != 8:
            print("  SKIP: {} @ {} - Size is {} (expected 8)".format(name, addr, body.getNumAddresses()))
            continue
        
        valid_trampolines.append((addr, func, name, target))
        print("  OK:   {} @ {} -> target {}".format(name, addr, hex(target)))
    
    print()
    print("Total to convert: {}".format(len(valid_trampolines)))
    print()
    
    if len(valid_trampolines) == 0:
        print("Nothing to convert. Exiting.")
        return
    
    # Ask for confirmation
    from ghidra.util import Msg
    choice = askYesNo("Convert Trampolines", 
        "Remove function definitions from {} trampolines?\n\n"
        "This will:\n"
        "1. Remove the function definition\n"
        "2. Keep the code as disassembly (NOT data)\n"
        "3. Add a plate comment explaining the trampoline\n"
        "4. Create a label for reference\n\n"
        "Note: These remain as CODE (asm), not data.\n"
        "This can be undone with Edit > Undo.".format(len(valid_trampolines)))
    
    if not choice:
        print("Cancelled by user.")
        return
    
    # Perform conversion
    print("Converting trampolines...")
    
    converted = 0
    for addr, func, name, target in valid_trampolines:
        try:
            # Add a plate comment explaining this is a trampoline
            comment = "TRAMPOLINE: {} -> {}".format(name, hex(target))
            listing.setComment(addr, CodeUnit.PLATE_COMMENT, comment)
            
            # Remove the function (keeps the disassembly)
            func_mgr.removeFunction(addr)
            
            # Create a label at the address
            symbol_table = currentProgram.getSymbolTable()
            symbol_table.createLabel(addr, "trampoline_" + name, SourceType.USER_DEFINED)
            
            converted += 1
            print("  Converted: {} @ {}".format(name, addr))
            
        except Exception as e:
            print("  ERROR: {} @ {} - {}".format(name, addr, str(e)))
    
    print()
    print("=" * 60)
    print("Conversion complete: {}/{} trampolines".format(converted, len(valid_trampolines)))
    print("=" * 60)

# Entry point for Ghidra script
run()
