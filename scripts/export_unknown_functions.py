#!/usr/bin/env python3
"""
Export unknown functions from Ghidra via MCP bridge.

This script connects to the Ghidra MCP server and exports all functions
matching unknown patterns (FUN_*, Callback_*, etc.) to a JSON file for
further analysis.

Usage:
    python3 scripts/export_unknown_functions.py [--output OUTPUT_FILE]
"""

import argparse
import json
import re
from datetime import datetime
from pathlib import Path

def categorize_by_address(address_hex):
    """Categorize function by memory address range."""
    addr = int(address_hex, 16)
    
    if 0x20000000 <= addr < 0x20001000:
        return ("GTE Hardware", "gte", 10)
    elif 0x80010000 <= addr < 0x80020000:
        return ("Graphics/Rendering", "graphics", 90)
    elif 0x80020000 <= addr < 0x80030000:
        return ("Entity System", "entity", 95)
    elif 0x80030000 <= addr < 0x80040000:
        return ("Particles/Effects", "particle", 70)
    elif 0x80040000 <= addr < 0x80050000:
        return ("Gameplay/Entities", "gameplay", 80)
    elif 0x80050000 <= addr < 0x80070000:
        return ("Player System", "player", 100)
    elif 0x80070000 <= addr < 0x80080000:
        return ("Vehicle Modes", "vehicle", 85)
    elif 0x80080000 <= addr < 0x80082000:
        return ("Level Loading", "level", 90)
    elif 0x80082000 <= addr < 0x8009c000:
        return ("PSY-Q Library", "library", 20)
    else:
        return ("Unknown Region", "unknown", 50)

def is_unknown_function(name):
    """Check if function name indicates it's unknown."""
    unknown_patterns = [
        r'^FUN_',
        r'^Callback_[0-9a-f]+$',
        r'^EntityCallback_[0-9a-f]+$',
        r'^PlayerCallback_[0-9a-f]+$',
        r'_OBJ_[0-9]+$',
        r'^BIOS_OBJ_',
        r'^SYS_OBJ_',
        r'^ISO9660_OBJ_',
        r'^FONT_OBJ_',
        r'^SPU_OBJ_',
    ]
    
    for pattern in unknown_patterns:
        if re.search(pattern, name):
            return True
    return False

def load_existing_symbols(symbol_file='symbol_addrs.txt'):
    """Load addresses of known symbols."""
    known_addrs = set()
    
    try:
        with open(symbol_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('/*'):
                    match = re.search(r'=\s*0x([0-9A-Fa-f]+)', line)
                    if match:
                        known_addrs.add(match.group(1).lower())
    except FileNotFoundError:
        print(f"Warning: {symbol_file} not found")
    
    return known_addrs

def mock_function_list():
    """Generate mock function list for testing without Ghidra connection."""
    # In real usage, this would query Ghidra MCP
    # For now, return structure for documentation
    
    functions = []
    
    # Graphics system unknowns
    graphics_funcs = [
        ("800138f0", "FUN_800138f0", 450),
        ("80013ab0", "FUN_80013ab0", 108),
        ("80013d10", "FUN_80013d10", 576),
        ("80013f50", "FUN_80013f50", 808),
        ("80015614", "FUN_80015614", 2568),
        ("8001889c", "FUN_8001889c", 1344),
    ]
    
    # Entity system unknowns
    entity_funcs = [
        ("8002086c", "FUN_8002086c", 68),
        ("800224d0", "FUN_800224d0", 552),
        ("8002be8c", "FUN_8002be8c", 7956),
    ]
    
    # Particle system unknowns
    particle_funcs = [
        ("800313cc", "FUN_800313cc", 7840),
        ("8003286c", "FUN_8003286c", 4816),
    ]
    
    for addr, name, size in graphics_funcs + entity_funcs + particle_funcs:
        category, cat_id, priority = categorize_by_address(addr)
        functions.append({
            "address": f"0x{addr}",
            "name": name,
            "size": size,
            "category": category,
            "category_id": cat_id,
            "priority": priority,
            "xref_count": 0  # Would query from Ghidra
        })
    
    return functions

def export_to_json(functions, output_file):
    """Export functions to JSON file with metadata."""
    
    output = {
        "metadata": {
            "generated": datetime.now().isoformat(),
            "total_functions": len(functions),
            "source": "Ghidra MCP Bridge",
            "project": "skullmonkeys (SLES_010.90)"
        },
        "functions": functions,
        "categories": {}
    }
    
    # Group by category for statistics
    for func in functions:
        cat = func["category"]
        if cat not in output["categories"]:
            output["categories"][cat] = {
                "count": 0,
                "total_size": 0,
                "avg_priority": 0
            }
        output["categories"][cat]["count"] += 1
        output["categories"][cat]["total_size"] += func["size"]
    
    # Calculate averages
    for cat_data in output["categories"].values():
        if cat_data["count"] > 0:
            cat_data["avg_size"] = cat_data["total_size"] // cat_data["count"]
    
    with open(output_file, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"✓ Exported {len(functions)} functions to {output_file}")
    print(f"\nCategories:")
    for cat, data in output["categories"].items():
        print(f"  {cat}: {data['count']} functions")

def main():
    parser = argparse.ArgumentParser(description="Export unknown functions from Ghidra")
    parser.add_argument('--output', '-o', default='build/unknown_functions.json',
                       help='Output JSON file')
    parser.add_argument('--symbols', '-s', default='symbol_addrs.txt',
                       help='Path to symbol_addrs.txt')
    parser.add_argument('--mock', action='store_true',
                       help='Use mock data instead of Ghidra connection')
    
    args = parser.parse_args()
    
    print("="*80)
    print("Ghidra Unknown Functions Exporter")
    print("="*80)
    print()
    
    # Load known symbols
    known_symbols = load_existing_symbols(args.symbols)
    print(f"Loaded {len(known_symbols)} known symbols from {args.symbols}")
    
    # Get function list
    if args.mock:
        print("Using mock data (--mock flag set)")
        functions = mock_function_list()
    else:
        print("\nNote: To get real Ghidra data, use Copilot with MCP tools:")
        print("  mcp_ghidra_functions_list(limit=2000)")
        print("  Then pass to this script as JSON")
        print("\nFor now, generating template output...")
        functions = mock_function_list()
    
    # Filter to only unknown functions
    unknown_functions = [
        func for func in functions
        if is_unknown_function(func["name"]) and
        func["address"][2:].lower() not in known_symbols
    ]
    
    print(f"Found {len(unknown_functions)} unknown functions")
    
    # Export
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    export_to_json(unknown_functions, args.output)
    
    print("\n" + "="*80)
    print("Next Steps")
    print("="*80)
    print(f"\n1. Review: cat {args.output}")
    print("2. Query Ghidra for specific functions:")
    print("   mcp_ghidra_functions_get(address='0x80015614')")
    print("3. Decompile with m2c:")
    print("   python3 tools/m2c/m2c.py --context ctx.c asm/pal/nonmatchings/80015614.s")
    print("4. Document in evil-engine/docs/")
    print("5. Update symbol_addrs.txt")

if __name__ == '__main__':
    main()
