#!/usr/bin/env python3
"""
Entity Callback Decompiler - Batch decompile entity init callbacks via Ghidra MCP.

Extracts key information from each entity type callback:
- Sprite ID (from SetEntitySpriteId calls)
- Entity struct size (from AllocateFromHeap)
- Init helper function used
- Tick callback assigned

Usage:
    python3 tools/scripts/decompile_entity_callbacks.py [--output PATH]

Requires:
    - Ghidra with MCP server running (ghidra-mcp)
    - SLES_010.90 binary loaded in Ghidra

Output:
    - entity_callbacks_decompiled.json: Full decompilation data
    - entity_callbacks_summary.md: Human-readable summary
"""

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Optional
import urllib.request
import urllib.error

# Ghidra MCP server settings
GHIDRA_HOST = "localhost"
GHIDRA_PORT = 8192

# Entity type callback table (from docs/reference/entity-types.md)
# Format: type -> callback_address
ENTITY_CALLBACKS = {
    0: "0x8007efd0", 1: "0x8007f730", 2: "0x80080328", 3: "0x8007efd0",
    4: "0x8007efd0", 5: "0x8007f7b0", 6: "0x8007f830", 7: "0x80080408",
    8: "0x80081504", 9: "0x800804e8", 10: "0x8007f244", 11: "0x80080478",
    12: "0x8007f8b0", 17: "0x8007f930", 18: "0x8007f9b0", 19: "0x8007fa30",
    20: "0x8007faac", 21: "0x8007fb28", 22: "0x80080398", 23: "0x80080558",
    24: "0x8007f460", 25: "0x800805c8", 26: "0x8007f2cc", 27: "0x8007f354",
    28: "0x80080638", 29: "0x800806a8", 30: "0x80080a98", 31: "0x80080af8",
    32: "0x80080af8", 33: "0x80080af8", 34: "0x80080b60", 35: "0x80080b60",
    36: "0x80080b60", 37: "0x80080bc8", 38: "0x80080bc8", 39: "0x80080c8c",
    40: "0x80080cfc", 41: "0x80080d6c", 42: "0x80080ddc", 43: "0x80080ddc",
    44: "0x80080ddc", 45: "0x80080f1c", 46: "0x80080c2c", 47: "0x80080e4c",
    48: "0x80080e4c", 49: "0x8007fba4", 50: "0x8007fc20", 51: "0x8007fc9c",
    52: "0x80080c8c", 53: "0x80080ddc", 54: "0x80080ddc", 55: "0x80080ddc",
    57: "0x8007fd18", 58: "0x8007fd94", 59: "0x8007fe10", 60: "0x80080ddc",
    61: "0x80080718", 62: "0x8007fe8c", 63: "0x8007fefc", 64: "0x8007ff6c",
    65: "0x80080f8c", 66: "0x8007ffdc", 67: "0x80080050", 68: "0x800800c4",
    69: "0x80080788", 70: "0x800807f8", 71: "0x80080fec", 72: "0x80080868",
    75: "0x800808d8", 76: "0x8007f3dc", 79: "0x8008121c", 80: "0x80080ebc",
    81: "0x80080948", 82: "0x8008127c", 83: "0x800809b8", 84: "0x8007f5b0",
    85: "0x800812ec", 86: "0x8007f050", 87: "0x8007f050", 88: "0x8007f050",
    89: "0x8008134c", 90: "0x80080138", 91: "0x800801b4", 92: "0x80080230",
    93: "0x800802ac", 94: "0x80081428", 95: "0x800814a4", 96: "0x8007f638",
    97: "0x8008134c", 98: "0x8008134c", 99: "0x8007f4d0", 100: "0x8008105c",
    101: "0x800810cc", 102: "0x8008113c", 103: "0x800811ac", 104: "0x800812ec",
    105: "0x800812ec", 106: "0x8007f0d0", 107: "0x8007f0d0", 108: "0x8007f0d0",
    109: "0x8007f540", 110: "0x8008134c", 111: "0x8008134c", 112: "0x8007f140",
    113: "0x8007f140", 114: "0x8007f140", 115: "0x8007f1c0", 116: "0x8007f1c0",
    117: "0x8007f1c0", 118: "0x8007f460", 119: "0x80080a28", 120: "0x8007f6c0",
}


def ghidra_request(endpoint: str, method: str = "GET", data: dict = None) -> dict:
    """Make a request to Ghidra MCP server."""
    url = f"http://{GHIDRA_HOST}:{GHIDRA_PORT}{endpoint}"
    
    req = urllib.request.Request(url, method=method)
    req.add_header("Content-Type", "application/json")
    
    if data:
        req.data = json.dumps(data).encode()
    
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            return json.loads(resp.read())
    except urllib.error.URLError as e:
        return {"error": str(e)}
    except json.JSONDecodeError:
        return {"error": "Invalid JSON response"}


def decompile_function(address: str) -> Optional[str]:
    """Decompile a function at the given address."""
    # Remove 0x prefix if present for API
    addr = address.replace("0x", "")
    result = ghidra_request(f"/functions/{addr}/decompile")
    
    if "error" in result or not result.get("success"):
        return None
    
    return result.get("result", {}).get("decompiled_text", "")


def get_function_info(address: str) -> dict:
    """Get function metadata."""
    addr = address.replace("0x", "")
    result = ghidra_request(f"/functions/{addr}")
    
    if "error" in result or not result.get("success"):
        return {}
    
    return result.get("result", {})


def extract_sprite_id(decompiled: str) -> Optional[str]:
    """Extract sprite ID from decompiled code."""
    # Look for common patterns:
    # SetEntitySpriteId(entity, 0x12345678, ...)
    # InitEntityWithSprite(entity, def, 0x12345678, ...)
    # or direct assignment with hex constant
    
    patterns = [
        r'SetEntitySpriteId\s*\([^,]+,\s*(0x[0-9a-fA-F]+)',
        r'InitEntityWithSprite\s*\([^,]+,[^,]+,\s*(0x[0-9a-fA-F]+)',
        r'InitGenericSpriteEntity\s*\([^,]+,[^,]+,\s*(0x[0-9a-fA-F]+)',
        r'CreateCollectibleWithFlags\s*\([^,]+,[^,]+,[^,]+,\s*(0x[0-9a-fA-F]+)',
        # Sprite table reference (e.g., g_SpriteList_*)
        r'(g_SpriteList_\w+)',
        r'(g_PlayerSpriteTable)',
    ]
    
    for pattern in patterns:
        match = re.search(pattern, decompiled)
        if match:
            return match.group(1)
    
    # Also look for 32-bit hex constants that might be sprite IDs
    # Sprite IDs are typically large values like 0x09406d8a
    hex_matches = re.findall(r'0x[0-9a-fA-F]{8}', decompiled)
    if hex_matches:
        # Return the first one that's not obviously something else
        for h in hex_matches:
            val = int(h, 16)
            # Sprite IDs are typically > 0x01000000 and not in RAM range (0x8XXXXXXX)
            if 0x01000000 < val < 0x80000000:
                return h
    
    return None


def extract_struct_size(decompiled: str) -> Optional[int]:
    """Extract entity struct size from AllocateFromHeap call."""
    # AllocateFromHeap(base, SIZE, ...)
    pattern = r'AllocateFromHeap\s*\([^,]+,\s*(0x[0-9a-fA-F]+|[0-9]+)'
    match = re.search(pattern, decompiled)
    if match:
        size_str = match.group(1)
        if size_str.startswith('0x'):
            return int(size_str, 16)
        return int(size_str)
    return None


def extract_init_helper(decompiled: str) -> Optional[str]:
    """Extract the init helper function called."""
    # Common init helpers
    helpers = [
        "CreateCollectibleWithFlags",
        "InitGenericSpriteEntity",
        "InitPathFollowingEntity",
        "InitEntityWithSprite",
        "InitEntityWithTable",
        "InitEntityWithCallbackVtable",
        "InitEntityWithDeathSpawn",
        "InitEntityState_Idle",
        "InitEntityState_Animated",
    ]
    
    for helper in helpers:
        if helper in decompiled:
            return helper
    
    # Look for FUN_XXXXXXXX calls that might be init helpers
    pattern = r'(FUN_[0-9a-fA-F]+)\s*\('
    matches = re.findall(pattern, decompiled)
    if matches:
        # Return the first function call that's not AllocateFromHeap, etc.
        for m in matches:
            if m not in ["FUN_80010000"]:  # Skip known non-init functions
                return m
    
    return None


def extract_tick_callback(decompiled: str) -> Optional[str]:
    """Extract tick callback assignment (entity[?] = callback)."""
    # Look for callback assignments like:
    # entity[1] = TickCallback;
    # *(entity + 0x18) = callback;
    patterns = [
        r'\*\s*\([^)]+\s*\+\s*0x18\)\s*=\s*([^;]+)',
        r'entity\s*\[\s*6\s*\]\s*=\s*([^;]+)',  # entity[6] at 4-byte offsets = +0x18
    ]
    
    for pattern in patterns:
        match = re.search(pattern, decompiled)
        if match:
            return match.group(1).strip()
    
    return None


def analyze_callback(entity_type: int, address: str) -> dict:
    """Analyze a single entity callback function."""
    result = {
        "type": entity_type,
        "address": address,
        "function_name": None,
        "sprite_id": None,
        "struct_size": None,
        "init_helper": None,
        "tick_callback": None,
        "decompiled": None,
        "error": None,
    }
    
    # Get function info
    func_info = get_function_info(address)
    if func_info:
        result["function_name"] = func_info.get("name")
    
    # Decompile
    decompiled = decompile_function(address)
    if not decompiled:
        result["error"] = "Failed to decompile"
        return result
    
    result["decompiled"] = decompiled
    result["sprite_id"] = extract_sprite_id(decompiled)
    result["struct_size"] = extract_struct_size(decompiled)
    result["init_helper"] = extract_init_helper(decompiled)
    result["tick_callback"] = extract_tick_callback(decompiled)
    
    return result


def main():
    parser = argparse.ArgumentParser(description="Decompile entity callbacks via Ghidra MCP")
    parser.add_argument("--output", type=Path, default=Path("docs/analysis"),
                        help="Output directory")
    parser.add_argument("--types", type=str, default=None,
                        help="Comma-separated list of types to analyze (default: all)")
    parser.add_argument("--skip-known", action="store_true",
                        help="Skip already-identified types")
    args = parser.parse_args()
    
    # Test connection
    print("Testing Ghidra MCP connection...")
    test = ghidra_request("/program")
    if "error" in test:
        print(f"Error connecting to Ghidra MCP: {test['error']}")
        print("Make sure Ghidra is running with the MCP extension enabled")
        return 1
    
    print(f"Connected to Ghidra: {test.get('result', {}).get('name', 'unknown')}")
    
    # Get unique callbacks to analyze
    unique_callbacks = {}
    for etype, addr in ENTITY_CALLBACKS.items():
        if addr not in unique_callbacks:
            unique_callbacks[addr] = []
        unique_callbacks[addr].append(etype)
    
    print(f"Found {len(unique_callbacks)} unique callbacks for {len(ENTITY_CALLBACKS)} types")
    
    # Filter types if specified
    if args.types:
        target_types = set(int(t.strip()) for t in args.types.split(','))
        unique_callbacks = {
            addr: [t for t in types if t in target_types]
            for addr, types in unique_callbacks.items()
            if any(t in target_types for t in types)
        }
    
    # Analyze each unique callback
    results = []
    for i, (addr, types) in enumerate(sorted(unique_callbacks.items())):
        print(f"[{i+1}/{len(unique_callbacks)}] Analyzing {addr} (types {types})...")
        
        result = analyze_callback(types[0], addr)
        result["all_types"] = types
        results.append(result)
        
        if result["error"]:
            print(f"  Error: {result['error']}")
        else:
            print(f"  Name: {result['function_name']}")
            print(f"  Sprite: {result['sprite_id']}")
            print(f"  Size: {result['struct_size']}")
            print(f"  Helper: {result['init_helper']}")
    
    # Output
    args.output.mkdir(parents=True, exist_ok=True)
    
    # JSON output (without full decompiled code for size)
    json_results = []
    for r in results:
        jr = {k: v for k, v in r.items() if k != "decompiled"}
        jr["has_decompiled"] = r["decompiled"] is not None
        json_results.append(jr)
    
    json_path = args.output / "entity_callbacks_decompiled.json"
    json_path.write_text(json.dumps(json_results, indent=2))
    print(f"\nWrote {json_path}")
    
    # Markdown summary
    lines = [
        "# Entity Callback Decompilation Summary",
        "",
        f"Analyzed {len(results)} unique callbacks",
        "",
        "## Results",
        "",
        "| Address | Types | Name | Sprite ID | Size | Init Helper |",
        "|---------|-------|------|-----------|------|-------------|",
    ]
    
    for r in results:
        types_str = ', '.join(str(t) for t in r['all_types'][:3])
        if len(r['all_types']) > 3:
            types_str += f" +{len(r['all_types'])-3}"
        
        name = r['function_name'] or "?"
        sprite = r['sprite_id'] or "-"
        size = f"0x{r['struct_size']:X}" if r['struct_size'] else "-"
        helper = r['init_helper'] or "-"
        
        lines.append(f"| {r['address']} | {types_str} | {name} | {sprite} | {size} | {helper} |")
    
    md_path = args.output / "entity_callbacks_summary.md"
    md_path.write_text('\n'.join(lines))
    print(f"Wrote {md_path}")
    
    # Summary stats
    with_sprite = sum(1 for r in results if r['sprite_id'])
    with_helper = sum(1 for r in results if r['init_helper'])
    print(f"\nSummary:")
    print(f"  Callbacks with sprite ID: {with_sprite}/{len(results)}")
    print(f"  Callbacks with init helper: {with_helper}/{len(results)}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
