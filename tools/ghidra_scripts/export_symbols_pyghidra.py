#!/usr/bin/env python3
"""
export_symbols_pyghidra.py - Export all Ghidra symbols using pyghidra

This script uses pyghidra to directly access a Ghidra project database
and export all functions and data symbols with their sizes.

Usage:
    # Set GHIDRA_INSTALL_DIR first (for NixOS, find via: dirname $(dirname $(readlink -f $(which ghidra))))
    export GHIDRA_INSTALL_DIR=/nix/store/.../ghidra
    
    # Run with an existing Ghidra project:
    python3 tools/ghidra_scripts/export_symbols_pyghidra.py --project ~/ghidra_projects/btm --program SLES_010.90
    
    # Or let pyghidra create a temporary project from binary:
    python3 tools/ghidra_scripts/export_symbols_pyghidra.py --binary disks/pal/SLES_010.90

Output is CSV format:
    address,rom_offset,type,name,size,end_address
"""

import argparse
import sys
import os

# PSX memory layout constants
RAM_BASE = 0x80010000
ROM_BASE = 0x800

def vram_to_rom(vram: int) -> int:
    """Convert VRAM address to ROM offset."""
    if vram >= RAM_BASE:
        return (vram - RAM_BASE) + ROM_BASE
    return -1


def export_from_program(program, output_file=None):
    """Export all symbols from a Ghidra program object."""
    import pyghidra
    
    symbols = []
    
    # Get all functions
    func_mgr = program.getFunctionManager()
    print(f"Exporting functions...", file=sys.stderr)
    
    for func in func_mgr.getFunctions(True):  # True = forward iteration
        addr = func.getEntryPoint().getOffset()
        name = func.getName()
        body = func.getBody()
        size = body.getNumAddresses() if body else 0
        end_addr = addr + size if size > 0 else addr
        
        symbols.append({
            'address': addr,
            'name': name,
            'type': 'function',
            'size': size,
            'end_address': end_addr
        })
    
    print(f"  Found {len(symbols)} functions", file=sys.stderr)
    
    # Get all defined data
    listing = program.getListing()
    print(f"Exporting data items...", file=sys.stderr)
    
    data_count = 0
    for data in listing.getDefinedData(True):  # True = forward iteration
        addr = data.getAddress().getOffset()
        label = data.getLabel()
        if not label:
            label = f"DAT_{addr:08X}"
        data_type = str(data.getDataType())
        size = data.getLength()
        end_addr = addr + size if size > 0 else addr
        
        symbols.append({
            'address': addr,
            'name': label,
            'type': f'data:{data_type}',
            'size': size,
            'end_address': end_addr
        })
        data_count += 1
    
    print(f"  Found {data_count} data items", file=sys.stderr)
    
    # Sort by address
    symbols.sort(key=lambda s: s['address'])
    
    # Output
    out = open(output_file, 'w') if output_file else sys.stdout
    out.write("address,rom_offset,type,name,size,end_address\n")
    
    for sym in symbols:
        addr = sym['address']
        rom = vram_to_rom(addr)
        out.write(f"0x{addr:08X},0x{rom:X},{sym['type']},{sym['name']},{sym['size']},0x{sym['end_address']:08X}\n")
    
    if output_file:
        out.close()
        print(f"Wrote {len(symbols)} symbols to {output_file}", file=sys.stderr)
    
    return symbols


def main():
    parser = argparse.ArgumentParser(description='Export Ghidra symbols using pyghidra')
    parser.add_argument('--project', type=str, help='Path to Ghidra project directory')
    parser.add_argument('--program', type=str, help='Program name within project')
    parser.add_argument('--binary', type=str, help='Binary file (creates temp project)')
    parser.add_argument('--output', '-o', type=str, help='Output file (default: stdout)')
    parser.add_argument('--ghidra-dir', type=str, help='Ghidra installation directory')
    args = parser.parse_args()
    
    if not args.project and not args.binary:
        parser.error("Must specify either --project or --binary")
    
    # Set Ghidra install dir if provided
    if args.ghidra_dir:
        os.environ['GHIDRA_INSTALL_DIR'] = args.ghidra_dir
    
    # Import pyghidra (after potentially setting env var)
    try:
        import pyghidra
    except ImportError:
        print("ERROR: pyghidra not installed. Run: pip install pyghidra", file=sys.stderr)
        sys.exit(1)
    
    # Check if GHIDRA_INSTALL_DIR is set
    ghidra_dir = os.environ.get('GHIDRA_INSTALL_DIR')
    if not ghidra_dir:
        print("WARNING: GHIDRA_INSTALL_DIR not set. PyGhidra will try to find Ghidra automatically.", file=sys.stderr)
    
    print(f"Starting PyGhidra...", file=sys.stderr)
    
    try:
        if args.binary:
            # Use legacy API to open binary directly
            print(f"Opening binary: {args.binary}", file=sys.stderr)
            with pyghidra.open_program(args.binary, analyze=False) as flat_api:
                program = flat_api.getCurrentProgram()
                print(f"Loaded: {program.getName()}", file=sys.stderr)
                export_from_program(program, args.output)
        else:
            # Use new API to open existing project
            print(f"Opening project: {args.project}", file=sys.stderr)
            pyghidra.start()
            
            with pyghidra.open_project(args.project, args.program or "SLES_010.90") as project:
                program_path = f"/{args.program}" if args.program else "/SLES_010.90"
                with pyghidra.program_context(project, program_path) as program:
                    print(f"Loaded: {program.getName()}", file=sys.stderr)
                    export_from_program(program, args.output)
                    
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
