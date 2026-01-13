#!/usr/bin/env python3
"""
Find potential functions that Ghidra hasn't recognized.

This script queries the Ghidra MCP server to find:
1. Data items that look like function pointers (values in code range 0x8001-0x800A)
2. Cross-references to addresses that aren't recognized as functions
3. Known callback tables and their entries

Usage:
    python3 scripts/find_unrecognized_functions.py [--port PORT]
"""

import argparse
import json
import struct
import sys
from urllib.request import urlopen, Request
from urllib.error import URLError

DEFAULT_PORT = 8192
CODE_START = 0x80010000
CODE_END = 0x800A0000  # Approximate end of code section
DATA_START = 0x8009D000  # Start of data section with callback tables


def ghidra_request(endpoint: str, port: int = DEFAULT_PORT) -> dict:
    """Make a request to the Ghidra MCP bridge."""
    url = f"http://localhost:{port}{endpoint}"
    try:
        with urlopen(url, timeout=30) as response:
            return json.loads(response.read().decode())
    except URLError as e:
        print(f"Error connecting to Ghidra MCP at port {port}: {e}", file=sys.stderr)
        sys.exit(1)


def get_all_functions(port: int) -> dict[int, str]:
    """Get all recognized functions as {address: name}."""
    functions = {}
    offset = 0
    limit = 500
    
    while True:
        result = ghidra_request(f"/functions?offset={offset}&limit={limit}", port)
        if not result.get("result"):
            break
        for func in result["result"]:
            addr = int(func["address"], 16)
            functions[addr] = func["name"]
        if len(result["result"]) < limit:
            break
        offset += limit
    
    return functions


def read_memory(address: int, length: int, port: int) -> bytes:
    """Read raw memory from Ghidra."""
    result = ghidra_request(f"/memory?address={address:08x}&length={length}&format=hex", port)
    # Response may have data at top level or nested in 'result'
    data = result.get("result", result)
    if data.get("hexBytes"):
        hex_str = data["hexBytes"].replace(" ", "")
        return bytes.fromhex(hex_str)
    return b""


def get_xrefs_to(address: int, port: int) -> list:
    """Get cross-references to an address."""
    result = ghidra_request(f"/xrefs?to_addr={address:08x}&limit=100", port)
    return result.get("result", [])


def analyze_callback_table(name: str, address: int, entry_count: int, entry_size: int, 
                           callback_offsets: list[int], port: int, known_functions: dict) -> list:
    """Analyze a callback table for unrecognized function pointers."""
    unrecognized = []
    
    data = read_memory(address, entry_count * entry_size, port)
    if not data:
        print(f"  Could not read memory at 0x{address:08X}", file=sys.stderr)
        return unrecognized
    
    for i in range(entry_count):
        entry_start = i * entry_size
        for offset in callback_offsets:
            if entry_start + offset + 4 > len(data):
                continue
            ptr = struct.unpack_from("<I", data, entry_start + offset)[0]
            
            # Check if it's in code range and not a known function
            if CODE_START <= ptr < CODE_END:
                if ptr not in known_functions:
                    unrecognized.append({
                        "table": name,
                        "index": i,
                        "offset": offset,
                        "address": ptr,
                        "table_addr": address + entry_start + offset
                    })
    
    return unrecognized


def scan_data_section_for_pointers(start: int, end: int, port: int, 
                                    known_functions: dict) -> list:
    """Scan a data section for values that look like code pointers."""
    unrecognized = []
    
    chunk_size = 4096
    for addr in range(start, end, chunk_size):
        length = min(chunk_size, end - addr)
        data = read_memory(addr, length, port)
        if not data:
            continue
        
        for offset in range(0, len(data) - 3, 4):
            ptr = struct.unpack_from("<I", data, offset)[0]
            
            # Check if it looks like a code pointer
            if CODE_START <= ptr < CODE_END:
                # Check alignment (MIPS functions are 4-byte aligned)
                if ptr & 3 == 0:
                    if ptr not in known_functions:
                        unrecognized.append({
                            "data_addr": addr + offset,
                            "target_addr": ptr
                        })
    
    return unrecognized


def main():
    parser = argparse.ArgumentParser(description="Find unrecognized functions in Ghidra")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, 
                        help=f"Ghidra MCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--scan-data", action="store_true",
                        help="Scan data section for potential function pointers")
    parser.add_argument("--output", "-o", type=str, default=None,
                        help="Output file for results (JSON format)")
    args = parser.parse_args()
    
    print("Connecting to Ghidra MCP...", file=sys.stderr)
    
    # Verify connection
    try:
        info = ghidra_request("/program", args.port)
        print(f"Connected to: {info.get('name', 'unknown')}", file=sys.stderr)
    except Exception as e:
        print(f"Failed to connect: {e}", file=sys.stderr)
        sys.exit(1)
    
    print("Loading recognized functions...", file=sys.stderr)
    known_functions = get_all_functions(args.port)
    print(f"Found {len(known_functions)} recognized functions", file=sys.stderr)
    
    all_unrecognized = []
    
    # Known callback tables
    callback_tables = [
        # (name, address, entry_count, entry_size, [offsets of callback pointers])
        ("EntityTypeCallbacks", 0x8009D5F8, 121, 8, [0, 4]),  # init, tick callbacks
        ("PlayerStateCallbacks", 0x800A5D20, 4, 8, [4]),  # state callback at +4
    ]
    
    print("\nAnalyzing known callback tables...", file=sys.stderr)
    for name, addr, count, size, offsets in callback_tables:
        print(f"  {name} @ 0x{addr:08X} ({count} entries)...", file=sys.stderr)
        results = analyze_callback_table(name, addr, count, size, offsets, 
                                         args.port, known_functions)
        all_unrecognized.extend(results)
        if results:
            print(f"    Found {len(results)} unrecognized pointers", file=sys.stderr)
    
    if args.scan_data:
        print("\nScanning data section for potential function pointers...", file=sys.stderr)
        # Scan data sections that commonly contain callback tables
        scan_ranges = [
            (0x8009D000, 0x800A6000),  # Main data section
        ]
        for start, end in scan_ranges:
            print(f"  Scanning 0x{start:08X} - 0x{end:08X}...", file=sys.stderr)
            results = scan_data_section_for_pointers(start, end, args.port, known_functions)
            # Deduplicate with table results
            table_addrs = {r["address"] for r in all_unrecognized if "address" in r}
            for r in results:
                if r["target_addr"] not in table_addrs:
                    all_unrecognized.append({
                        "table": "data_scan",
                        "address": r["target_addr"],
                        "table_addr": r["data_addr"]
                    })
    
    # Deduplicate by target address
    seen_addrs = {}
    for item in all_unrecognized:
        addr = item.get("address") or item.get("target_addr")
        if addr not in seen_addrs:
            seen_addrs[addr] = item
        else:
            # Merge info
            if "table" in item and item["table"] != "data_scan":
                seen_addrs[addr] = item
    
    unique_results = sorted(seen_addrs.values(), key=lambda x: x.get("address", 0))
    
    # Output
    print(f"\n{'='*60}", file=sys.stderr)
    print(f"Found {len(unique_results)} unrecognized function pointers", file=sys.stderr)
    print(f"{'='*60}\n", file=sys.stderr)
    
    # Print summary by table
    by_table = {}
    for item in unique_results:
        table = item.get("table", "unknown")
        if table not in by_table:
            by_table[table] = []
        by_table[table].append(item)
    
    for table, items in sorted(by_table.items()):
        print(f"\n{table} ({len(items)} entries):")
        print("-" * 40)
        for item in sorted(items, key=lambda x: x.get("index", 0)):
            addr = item.get("address", 0)
            if "index" in item:
                print(f"  [{item['index']:3d}] 0x{addr:08X}  (ptr @ 0x{item['table_addr']:08X})")
            else:
                print(f"        0x{addr:08X}  (ptr @ 0x{item['table_addr']:08X})")
    
    # Output symbol_addrs.txt format suggestions
    print(f"\n\n{'='*60}")
    print("Suggested additions to symbol_addrs.txt:")
    print("(Copy/paste the lines you want to add)")
    print(f"{'='*60}\n")
    
    for item in unique_results:
        addr = item.get("address", 0)
        table = item.get("table", "")
        index = item.get("index", -1)
        
        if table == "EntityTypeCallbacks" and index >= 0:
            offset = item.get("offset", 0)
            cb_type = "Init" if offset == 0 else "Tick"
            print(f"EntityType{index:03d}_{cb_type}Callback = 0x{addr:08X};")
        elif table == "PlayerStateCallbacks" and index >= 0:
            print(f"PlayerStateCallback_{index} = 0x{addr:08X};")
        else:
            print(f"FUN_{addr:08x} = 0x{addr:08X};  // ptr @ 0x{item.get('table_addr', 0):08X}")
    
    # Save to JSON if requested
    if args.output:
        with open(args.output, "w") as f:
            json.dump({
                "recognized_functions": len(known_functions),
                "unrecognized_pointers": len(unique_results),
                "results": unique_results
            }, f, indent=2)
        print(f"\nResults saved to {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
