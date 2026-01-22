#!/usr/bin/env python3
"""
Export all symbols from Ghidra in address order.

This script queries the Ghidra MCP server to get all functions and data symbols,
then outputs them sorted by address. Useful for understanding the binary layout
and for creating splat configuration.

Usage:
    python3 scripts/export_ghidra_symbols.py > symbols.txt
    python3 scripts/export_ghidra_symbols.py --format=yaml  # For splat config
    python3 scripts/export_ghidra_symbols.py --range 0x80010000-0x80020000
    python3 scripts/export_ghidra_symbols.py --functions-only
    python3 scripts/export_ghidra_symbols.py --data-only
"""

import sys
import json
import argparse
from urllib.request import urlopen
from typing import List, Dict, Optional, Tuple

# Configuration
GHIDRA_PORT = 8192
GHIDRA_URL = f"http://localhost:{GHIDRA_PORT}"

# Memory layout constants
RAM_BASE = 0x80010000
ROM_BASE = 0x800

def vram_to_rom(vram: int) -> int:
    """Convert VRAM address to ROM offset."""
    if vram >= RAM_BASE:
        return (vram - RAM_BASE) + ROM_BASE
    return -1

def ghidra_request(endpoint: str) -> dict:
    """Make request to Ghidra MCP server."""
    url = f"{GHIDRA_URL}{endpoint}"
    try:
        with urlopen(url, timeout=30) as response:
            return json.loads(response.read().decode())
    except Exception as e:
        print(f"ERROR: Failed to connect to Ghidra: {e}", file=sys.stderr)
        print(f"Is Ghidra MCP server running on port {GHIDRA_PORT}?", file=sys.stderr)
        sys.exit(1)

def get_all_functions() -> List[Dict]:
    """Get all functions from Ghidra."""
    functions = []
    offset = 0
    limit = 500
    
    print(f"Fetching functions from Ghidra...", file=sys.stderr)
    
    while True:
        result = ghidra_request(f"/functions?offset={offset}&limit={limit}")
        if not result.get("success"):
            print(f"ERROR: Failed to get functions: {result}", file=sys.stderr)
            break
        
        batch = result.get("result", [])
        if not batch:
            break
        
        functions.extend(batch)
        total = result.get("size", len(functions))
        print(f"  Fetched {len(functions)}/{total} functions...", file=sys.stderr)
        
        if len(batch) < limit:
            break
        offset += limit
    
    return functions

def get_all_data() -> List[Dict]:
    """Get all data items from Ghidra."""
    data_items = []
    offset = 0
    limit = 500
    
    print(f"Fetching data items from Ghidra...", file=sys.stderr)
    
    while True:
        result = ghidra_request(f"/data?offset={offset}&limit={limit}")
        if not result.get("success"):
            print(f"ERROR: Failed to get data: {result}", file=sys.stderr)
            break
        
        batch = result.get("result", [])
        if not batch:
            break
        
        data_items.extend(batch)
        total = result.get("size", len(data_items))
        print(f"  Fetched {len(data_items)}/{total} data items...", file=sys.stderr)
        
        if len(batch) < limit:
            break
        offset += limit
    
    return data_items

def get_function_size(address: int) -> int:
    """Get function size by counting instructions."""
    try:
        result = ghidra_request(f"/functions/{address:08x}/disassembly")
        if result.get("success"):
            instructions = result.get("result", {}).get("instructions", [])
            return len(instructions) * 4
    except:
        pass
    return 0

def parse_address_range(range_str: str) -> Tuple[int, int]:
    """Parse address range like '0x80010000-0x80020000'."""
    if '-' in range_str:
        start, end = range_str.split('-')
        return int(start, 0), int(end, 0)
    else:
        return int(range_str, 0), 0xFFFFFFFF

def main():
    parser = argparse.ArgumentParser(description='Export Ghidra symbols in address order')
    parser.add_argument('--format', choices=['text', 'yaml', 'csv', 'json'], default='text',
                        help='Output format (default: text)')
    parser.add_argument('--range', type=str, default=None,
                        help='Address range to export (e.g., 0x80010000-0x80020000)')
    parser.add_argument('--functions-only', action='store_true',
                        help='Export only functions')
    parser.add_argument('--data-only', action='store_true',
                        help='Export only data items')
    parser.add_argument('--with-sizes', action='store_true',
                        help='Include function sizes via API (very slow, makes individual calls)')
    parser.add_argument('--infer-sizes', action='store_true',
                        help='Infer sizes from next symbol address (fast, no extra API calls)')
    parser.add_argument('--exclude-external', action='store_true',
                        help='Exclude external/GTE functions')
    args = parser.parse_args()
    
    # Parse address range
    addr_min, addr_max = (0, 0xFFFFFFFF)
    if args.range:
        addr_min, addr_max = parse_address_range(args.range)
    
    # Collect all symbols
    symbols = []
    
    if not args.data_only:
        functions = get_all_functions()
        for func in functions:
            addr_str = func.get("address", "0")
            addr = int(addr_str, 16) if isinstance(addr_str, str) else addr_str
            name = func.get("name", f"func_{addr:08X}")
            
            # Skip external functions (GTE macros, etc.)
            if args.exclude_external and addr < 0x80000000:
                continue
            
            # Filter by range
            if not (addr_min <= addr <= addr_max):
                continue
            
            size = 0
            if args.with_sizes:
                size = get_function_size(addr)
            
            symbols.append({
                'type': 'function',
                'address': addr,
                'name': name,
                'size': size,
                'rom_offset': vram_to_rom(addr)
            })
    
    if not args.functions_only:
        data_items = get_all_data()
        for item in data_items:
            addr_str = item.get("address", "0")
            addr = int(addr_str, 16) if isinstance(addr_str, str) else addr_str
            label = item.get("label", f"D_{addr:08X}")
            data_type = item.get("dataType", "undefined")
            
            # Skip external data
            if args.exclude_external and addr < 0x80000000:
                continue
            
            # Filter by range
            if not (addr_min <= addr <= addr_max):
                continue
            
            symbols.append({
                'type': 'data',
                'address': addr,
                'name': label,
                'data_type': data_type,
                'rom_offset': vram_to_rom(addr)
            })
    
    # Sort by address
    symbols.sort(key=lambda x: x['address'])
    
    # Infer sizes from next symbol if requested
    if args.infer_sizes:
        print(f"Inferring sizes from symbol boundaries...", file=sys.stderr)
        for i, sym in enumerate(symbols):
            if i + 1 < len(symbols):
                next_addr = symbols[i + 1]['address']
                inferred_size = next_addr - sym['address']
                # Only set if reasonable (< 1MB)
                if 0 < inferred_size < 0x100000:
                    sym['size'] = inferred_size
    
    print(f"\nTotal symbols: {len(symbols)}", file=sys.stderr)
    
    # Output
    if args.format == 'text':
        print(f"# Ghidra symbols export")
        print(f"# Total: {len(symbols)} symbols")
        print(f"# {'='*70}")
        print(f"# {'Address':<12} {'ROM':<8} {'Type':<10} {'Name'}")
        print(f"# {'-'*70}")
        for sym in symbols:
            addr = sym['address']
            rom = sym['rom_offset']
            stype = sym['type']
            name = sym['name']
            
            rom_str = f"0x{rom:05X}" if rom >= 0 else "N/A"
            
            if sym['type'] == 'function' and sym.get('size'):
                print(f"  0x{addr:08X}  {rom_str}  {stype:<10} {name} (size: 0x{sym['size']:X})")
            elif sym['type'] == 'data':
                dtype = sym.get('data_type', '')
                print(f"  0x{addr:08X}  {rom_str}  {stype:<10} {name} [{dtype}]")
            else:
                print(f"  0x{addr:08X}  {rom_str}  {stype:<10} {name}")
    
    elif args.format == 'yaml':
        # Output as splat YAML subsegments
        print("# Splat subsegments from Ghidra")
        print("# Copy relevant sections to config/splat.pal.yaml")
        print("subsegments:")
        
        last_addr = None
        for sym in symbols:
            addr = sym['address']
            rom = sym['rom_offset']
            name = sym['name']
            
            if rom < 0:
                continue
            
            if sym['type'] == 'function':
                # Skip if same address as last
                if last_addr == rom:
                    print(f"  # Duplicate: {name} @ 0x{addr:08X}")
                    continue
                print(f"  - [0x{rom:X}, asm, {name}]")
            else:
                print(f"  - [0x{rom:X}, rodata]  # {name}")
            
            last_addr = rom
    
    elif args.format == 'csv':
        import csv as csv_module
        import io
        writer = csv_module.writer(sys.stdout, quoting=csv_module.QUOTE_MINIMAL)
        writer.writerow(["address", "rom_offset", "type", "name", "size", "data_type"])
        for sym in symbols:
            addr = sym['address']
            rom = sym['rom_offset']
            stype = sym['type']
            name = sym['name']
            size = sym.get('size', '')
            dtype = sym.get('data_type', '')
            writer.writerow([f"0x{addr:08X}", f"0x{rom:X}", stype, name, size, dtype])
    
    elif args.format == 'json':
        print(json.dumps(symbols, indent=2))

if __name__ == "__main__":
    main()
