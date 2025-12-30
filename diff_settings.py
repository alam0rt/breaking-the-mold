#!/usr/bin/env python3
# =============================================================================
# asm-differ Configuration
# =============================================================================
# This file configures the asm-differ tool for comparing compiled output
# to the original binary.
#
# Usage: python3 tools/asm-differ/diff.py -mwo FunctionName
#
# See: https://github.com/simonlindholm/asm-differ
# =============================================================================


def apply(config, args):
    """Configure asm-differ for PSX MIPS comparison."""
    
    # -------------------------------------------------------------------------
    # Basic settings
    # -------------------------------------------------------------------------
    
    # Architecture: MIPS (PSX uses MIPS R3000)
    config["arch"] = "mips"
    
    # Base directory for source files
    config["baseimg"] = "disks/us/SLUS_000.00"  # Original binary
    config["myimg"] = "build/us/game.bin"        # Your build
    
    # Map file from linker (shows symbol addresses)
    config["mapfile"] = "build/us/game.map"
    
    # Build command to run before diffing
    config["make_command"] = ["make"]
    
    # Source directories to search for symbols
    config["source_directories"] = ["src", "include"]
    
    # -------------------------------------------------------------------------
    # MIPS-specific settings
    # -------------------------------------------------------------------------
    
    # Instruction�comparison settings
    config["objdump_executable"] = "mipsel-linux-gnu-objdump"
    
    # Show instruction�bytes in diff
    config["show_line_numbers"] = True
    
    # -------------------------------------------------------------------------
    # Symbol handling
    # -------------------------------------------------------------------------
    
    # Symbol files for address lookup
    config["symbol_addrs_path"] = "config/symbols.us.txt"
    
    # -------------------------------------------------------------------------
    # Diff display options
    # -------------------------------------------------------------------------
    
    # Number of context lines to show around differences
    config["context"] = 3
    
    # Show register allocation differences
    config["show_rodata_refs"] = True
    
    # -------------------------------------------------------------------------
    # Project-specific adjustments
    # -------------------------------------------------------------------------
    
    # If your project uses different paths, adjust here:
    # config["baseimg"] = "expected/us/SLUS_000.00"
    # config["myimg"] = "build/us/main.bin"
    
    # For overlay support (multiple binaries):
    # if args.overlay:
    #     config["baseimg"] = f"disks/us/{args.overlay}.bin"
    #     config["myimg"] = f"build/us/{args.overlay}.bin"
    #     config["mapfile"] = f"build/us/{args.overlay}.map"
