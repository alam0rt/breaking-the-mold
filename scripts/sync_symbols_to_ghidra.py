#!/usr/bin/env python3
"""
Sync function names from symbol_addrs.txt to Ghidra.

This script reads symbol_addrs.txt and renames functions in Ghidra
to match the documented names.

Usage:
    python3 scripts/sync_symbols_to_ghidra.py [--dry-run] [--port PORT]
"""

import argparse
import json
import re
import sys
from urllib.request import urlopen, Request
from urllib.error import URLError

DEFAULT_PORT = 8192
SYMBOL_FILE = "symbol_addrs.txt"


def ghidra_request(endpoint: str, port: int = DEFAULT_PORT, method: str = "GET", data: dict = None) -> dict:
    """Make a request to the Ghidra MCP bridge."""
    url = f"http://localhost:{port}{endpoint}"
    try:
        if data:
            req = Request(
                url,
                data=json.dumps(data).encode('utf-8'),
                headers={'Content-Type': 'application/json'},
                method=method
            )
        else:
            req = Request(url, method=method)
        with urlopen(req, timeout=30) as response:
            return json.loads(response.read().decode())
    except URLError as e:
        return {"error": str(e)}


def get_function_at(address: int, port: int) -> dict | None:
    """Get function info at address."""
    result = ghidra_request(f"/functions/{address:08x}", port)
    if result.get("success") or result.get("name"):
        return result.get("result", result)
    return None


def rename_function(address: int, new_name: str, port: int) -> dict:
    """Rename a function via the MCP API."""
    return ghidra_request(
        f"/functions/{address:08x}",
        port,
        method="PUT",
        data={"name": new_name}
    )


def parse_symbol_addrs(filepath: str) -> dict[int, str]:
    """Parse symbol_addrs.txt and return {address: name} mapping."""
    symbols = {}
    
    # Pattern: name = 0xADDRESS; // optional comment
    pattern = re.compile(r'^(\w+)\s*=\s*0x([0-9A-Fa-f]+)\s*;')
    
    try:
        with open(filepath, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('//'):
                    continue
                match = pattern.match(line)
                if match:
                    name = match.group(1)
                    addr = int(match.group(2), 16)
                    symbols[addr] = name
    except FileNotFoundError:
        print(f"Error: {filepath} not found", file=sys.stderr)
        sys.exit(1)
    
    return symbols


def main():
    parser = argparse.ArgumentParser(description="Sync function names from symbol_addrs.txt to Ghidra")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"Ghidra MCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would be renamed without making changes")
    parser.add_argument("--filter", type=str, default=None,
                        help="Only process symbols matching this pattern (e.g., 'EntityType')")
    parser.add_argument("--symbols", type=str, default=SYMBOL_FILE,
                        help=f"Symbol file to read (default: {SYMBOL_FILE})")
    args = parser.parse_args()
    
    print("Loading symbols from", args.symbols, file=sys.stderr)
    symbols = parse_symbol_addrs(args.symbols)
    print(f"Loaded {len(symbols)} symbols", file=sys.stderr)
    
    if args.filter:
        symbols = {addr: name for addr, name in symbols.items() if args.filter in name}
        print(f"Filtered to {len(symbols)} symbols matching '{args.filter}'", file=sys.stderr)
    
    if args.dry_run:
        print("\n(DRY RUN - no changes will be made)\n", file=sys.stderr)
    
    renamed = 0
    already_correct = 0
    not_found = 0
    errors = 0
    
    for addr, expected_name in sorted(symbols.items()):
        func = get_function_at(addr, args.port)
        
        if not func:
            # Not a function in Ghidra
            continue
        
        current_name = func.get("name", "")
        
        if current_name == expected_name:
            already_correct += 1
            continue
        
        # Check if it needs renaming (has auto-generated name)
        needs_rename = (
            current_name.startswith("FUN_") or
            current_name.startswith("LAB_") or
            current_name != expected_name
        )
        
        if needs_rename:
            print(f"  0x{addr:08X}: {current_name} -> {expected_name}", end="")
            
            if args.dry_run:
                print(" (would rename)")
                renamed += 1
            else:
                result = rename_function(addr, expected_name, args.port)
                if result.get("success") or result.get("name") == expected_name:
                    print(" [OK]")
                    renamed += 1
                else:
                    print(f" [ERROR: {result.get('error', 'unknown')}]")
                    errors += 1
    
    print(f"\nSummary:", file=sys.stderr)
    print(f"  Already correct: {already_correct}", file=sys.stderr)
    print(f"  Renamed: {renamed}", file=sys.stderr)
    print(f"  Errors: {errors}", file=sys.stderr)


if __name__ == "__main__":
    main()
