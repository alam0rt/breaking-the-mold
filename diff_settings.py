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
    
    # Architecture: MIPS (PSX uses MIPS R3000, little-endian)
    config["arch"] = "mipsel"
    
    # Base directory for source files
    config["baseimg"] = "bin/SLES_010.90"  # Original binary
    config["myimg"] = "build/SLES_010.90.bin"        # Your build
    
    # Map file from linker (shows symbol addresses)
    config["mapfile"] = "build/SLES_010.90.map"
    
    # PSX load address: code starts at file offset 0x800, loaded to VRAM 0x80010000
    # This tells asm-differ how to translate VRAM addresses to file offsets
    config["base_shift"] = 0x80010000 - 0x800
    
    # Build command to run before diffing
    config["make_command"] = ["make"]
    
    # Source directories to search for symbols
    config["source_directories"] = ["src", "include"]
    
    # -------------------------------------------------------------------------
    # MIPS-specific settings
    # -------------------------------------------------------------------------
    
    # Instruction comparison settings
    config["objdump_executable"] = "mipsel-unknown-linux-gnu-objdump"
    
    # Show instruction�bytes in diff
    config["show_line_numbers"] = True
    
    # -------------------------------------------------------------------------
    # Symbol handling
    # -------------------------------------------------------------------------
    
    # Symbol files for address lookup
    config["symbol_addrs_path"] = "config/symbol_addrs.txt"
    
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
