#!/usr/bin/env python3
"""
Scan recently created callback functions for LAB_ references that should be functions.

This script decompiles Callback_* functions and extracts addresses assigned to
entity callback fields (like param_1[1], param_1[3], etc.) which are likely
function pointers that Ghidra hasn't recognized.
"""

import argparse
import json
import re
import sys
from urllib.request import urlopen, Request
from urllib.error import URLError

DEFAULT_PORT = 8192


def ghidra_request(endpoint: str, port: int = DEFAULT_PORT) -> dict:
    """Make a request to the Ghidra MCP bridge."""
    url = f"http://localhost:{port}{endpoint}"
    try:
        with urlopen(url, timeout=30) as response:
            return json.loads(response.read().decode())
    except URLError as e:
        print(f"Error connecting to Ghidra MCP at port {port}: {e}", file=sys.stderr)
        sys.exit(1)


def get_callback_functions(port: int) -> list:
    """Get all functions named Callback_*."""
    functions = []
    offset = 0
    limit = 100
    
    while True:
        result = ghidra_request(f"/functions?name_contains=Callback_&offset={offset}&limit={limit}", port)
        if not result.get("result"):
            break
        functions.extend(result["result"])
        if len(result["result"]) < limit:
            break
        offset += limit
    
    return functions


def get_all_functions(port: int) -> set:
    """Get all recognized function addresses."""
    functions = set()
    offset = 0
    limit = 500
    
    while True:
        result = ghidra_request(f"/functions?offset={offset}&limit={limit}", port)
        if not result.get("result"):
            break
        for func in result["result"]:
            functions.add(int(func["address"], 16))
        if len(result["result"]) < limit:
            break
        offset += limit
    
    return functions


def decompile_function(address: str, port: int) -> str:
    """Decompile a function and return the code."""
    result = ghidra_request(f"/functions/{address}/decompile", port)
    if result.get("success") and result.get("result"):
        return result["result"].get("decompiled", "")
    return ""


def extract_lab_addresses(code: str) -> list:
    """Extract LAB_* addresses from decompiled code."""
    # Match patterns like &LAB_80012345 or LAB_80012345
    pattern = r'&?LAB_([0-9a-fA-F]{8})'
    matches = re.findall(pattern, code)
    return [int(m, 16) for m in matches]


def main():
    parser = argparse.ArgumentParser(description="Find LAB_ references in callback functions")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--output", "-o", type=str, default=None)
    args = parser.parse_args()
    
    print("Loading recognized functions...", file=sys.stderr)
    known_functions = get_all_functions(args.port)
    print(f"Found {len(known_functions)} recognized functions", file=sys.stderr)
    
    print("Finding Callback_* functions...", file=sys.stderr)
    callbacks = get_callback_functions(args.port)
    print(f"Found {len(callbacks)} Callback_* functions to analyze", file=sys.stderr)
    
    unrecognized = {}  # address -> list of referencing functions
    
    for i, func in enumerate(callbacks):
        addr = func["address"]
        name = func["name"]
        
        if (i + 1) % 20 == 0:
            print(f"  Analyzed {i+1}/{len(callbacks)}...", file=sys.stderr)
        
        code = decompile_function(addr, args.port)
        if not code:
            continue
        
        lab_addrs = extract_lab_addresses(code)
        for lab_addr in lab_addrs:
            if lab_addr not in known_functions:
                if lab_addr not in unrecognized:
                    unrecognized[lab_addr] = []
                unrecognized[lab_addr].append(name)
    
    print(f"\n{'='*60}", file=sys.stderr)
    print(f"Found {len(unrecognized)} unrecognized LAB_ addresses", file=sys.stderr)
    print(f"{'='*60}\n", file=sys.stderr)
    
    # Sort by address
    sorted_addrs = sorted(unrecognized.keys())
    
    for addr in sorted_addrs:
        refs = unrecognized[addr]
        print(f"LAB_{addr:08x} - referenced by: {', '.join(refs[:3])}" + 
              (f" (+{len(refs)-3} more)" if len(refs) > 3 else ""))
    
    print(f"\n\n{'='*60}")
    print("Ghidra script entries:")
    print(f"{'='*60}\n")
    
    for addr in sorted_addrs:
        print(f"    (0x{addr:08X}, \"LAB_{addr:08x}\"),")
    
    if args.output:
        with open(args.output, "w") as f:
            json.dump({
                "count": len(unrecognized),
                "addresses": [{"address": addr, "refs": unrecognized[addr]} for addr in sorted_addrs]
            }, f, indent=2)
        print(f"\nResults saved to {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
