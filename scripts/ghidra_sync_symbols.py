#!/usr/bin/env python3
"""
Bidirectional symbol synchronization between Ghidra and symbol_addrs.txt.

This script can:
1. Export all functions from Ghidra to symbol_addrs.txt (--export)
2. Import symbol_addrs.txt to Ghidra (--import, like sync_symbols_to_ghidra.py)
3. Validate consistency between both (--validate)

Usage:
    # Export from Ghidra to symbol_addrs.txt
    python3 scripts/ghidra_sync_symbols.py --export

    # Import from symbol_addrs.txt to Ghidra
    python3 scripts/ghidra_sync_symbols.py --import

    # Validate both are in sync
    python3 scripts/ghidra_sync_symbols.py --validate

    # Dry run mode
    python3 scripts/ghidra_sync_symbols.py --export --dry-run
"""

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from urllib.request import urlopen, Request
from urllib.error import URLError

DEFAULT_PORT = 8192
SYMBOL_FILE = "symbol_addrs.txt"
RAM_BASE = 0x80010000
ROM_BASE = 0x800


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


def get_all_functions(port: int, limit: int = 10000) -> List[dict]:
    """Get all functions from Ghidra."""
    result = ghidra_request(f"/functions?limit={limit}", port)
    if result.get("success"):
        # MCP API returns functions in 'result', not 'functions'
        return result.get("result", result.get("functions", []))
    return []


def get_function_at(address: int, port: int) -> Optional[dict]:
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


class SymbolEntry:
    """Represents a symbol entry with address, name, and optional metadata."""
    
    def __init__(self, name: str, address: int, size: Optional[int] = None, 
                 type_hint: Optional[str] = None, comment: Optional[str] = None):
        self.name = name
        self.address = address
        self.size = size
        self.type_hint = type_hint
        self.comment = comment
    
    def to_line(self) -> str:
        """Format as a symbol_addrs.txt line."""
        line = f"{self.name} = 0x{self.address:08X};"
        
        annotations = []
        if self.size:
            annotations.append(f"size:0x{self.size:X}")
        if self.type_hint:
            annotations.append(f"type:{self.type_hint}")
        
        if annotations:
            line += " // " + " ".join(annotations)
        
        if self.comment and not annotations:
            line += f" // {self.comment}"
        elif self.comment:
            line += f" {self.comment}"
        
        return line
    
    @staticmethod
    def from_line(line: str) -> Optional['SymbolEntry']:
        """Parse a symbol_addrs.txt line."""
        line = line.strip()
        if not line or line.startswith('//'):
            return None
        
        # Pattern: name = 0xADDRESS; // optional annotations
        pattern = re.compile(r'^(\w+)\s*=\s*0x([0-9A-Fa-f]+)\s*;(.*)$')
        match = pattern.match(line)
        
        if not match:
            return None
        
        name = match.group(1)
        address = int(match.group(2), 16)
        comment_part = match.group(3).strip()
        
        size = None
        type_hint = None
        comment = None
        
        if comment_part.startswith('//'):
            comment_part = comment_part[2:].strip()
            
            # Parse annotations
            size_match = re.search(r'size:(0x[0-9A-Fa-f]+)', comment_part)
            if size_match:
                size = int(size_match.group(1), 16)
            
            type_match = re.search(r'type:(\w+(?:\[\])?)', comment_part)
            if type_match:
                type_hint = type_match.group(1)
            
            # Remove annotations to get plain comment
            comment = re.sub(r'size:0x[0-9A-Fa-f]+', '', comment_part)
            comment = re.sub(r'type:\w+(?:\[\])?', '', comment)
            comment = comment.strip()
            if not comment:
                comment = None
        
        return SymbolEntry(name, address, size, type_hint, comment)


def parse_symbol_addrs(filepath: str) -> Tuple[Dict[int, SymbolEntry], List[str]]:
    """Parse symbol_addrs.txt and return {address: SymbolEntry} and preserved comments."""
    symbols = {}
    preserved_lines = []
    
    try:
        with open(filepath, 'r') as f:
            current_section_comment = []
            
            for line in f:
                stripped = line.strip()
                
                # Preserve section headers and standalone comments
                if not stripped or stripped.startswith('//'):
                    preserved_lines.append(line.rstrip())
                    continue
                
                entry = SymbolEntry.from_line(line)
                if entry:
                    symbols[entry.address] = entry
                else:
                    # Preserve unrecognized lines
                    preserved_lines.append(line.rstrip())
    
    except FileNotFoundError:
        print(f"Warning: {filepath} not found, will create new file", file=sys.stderr)
    
    return symbols, preserved_lines


def write_symbol_addrs(filepath: str, symbols: Dict[int, SymbolEntry], 
                        preserved_lines: List[str], dry_run: bool = False):
    """Write symbols back to symbol_addrs.txt, preserving structure."""
    
    if dry_run:
        print(f"\n(DRY RUN - would write to {filepath})\n", file=sys.stderr)
        return
    
    # Group symbols by section (heuristic: find comment blocks)
    lines = []
    
    # Add header
    lines.append("// Auto-generated symbol addresses from Ghidra")
    lines.append("// Manual edits will be preserved if they don't conflict")
    lines.append("")
    
    # Sort symbols by address
    sorted_symbols = sorted(symbols.values(), key=lambda s: s.address)
    
    # Write symbols
    prev_addr = None
    for symbol in sorted_symbols:
        # Add spacing for significant address jumps (new sections)
        if prev_addr and symbol.address - prev_addr > 0x1000:
            lines.append("")
        
        lines.append(symbol.to_line())
        prev_addr = symbol.address
    
    # Write to file
    with open(filepath, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    
    print(f"Wrote {len(symbols)} symbols to {filepath}", file=sys.stderr)


def export_from_ghidra(port: int, symbol_file: str, dry_run: bool = False,
                       preserve_manual: bool = True):
    """Export all functions from Ghidra to symbol_addrs.txt."""
    print("Fetching functions from Ghidra...", file=sys.stderr)
    functions = get_all_functions(port)
    
    if not functions:
        print("Error: No functions found in Ghidra", file=sys.stderr)
        return False
    
    print(f"Found {len(functions)} functions in Ghidra", file=sys.stderr)
    
    # Load existing symbols to preserve manual annotations
    existing_symbols = {}
    preserved_lines = []
    
    if preserve_manual and Path(symbol_file).exists():
        existing_symbols, preserved_lines = parse_symbol_addrs(symbol_file)
        print(f"Loaded {len(existing_symbols)} existing symbols", file=sys.stderr)
    
    # Build new symbol map
    new_symbols = {}
    
    for func in functions:
        address = func.get("entryPoint", 0)
        if isinstance(address, str):
            address = int(address, 16)
        
        name = func.get("name", "")
        
        # Skip auto-generated names unless there's no existing entry
        if name.startswith("FUN_") or name.startswith("LAB_"):
            if address not in existing_symbols:
                continue  # Skip unnamed functions
            else:
                # Use existing name
                name = existing_symbols[address].name
        
        # Get function size from Ghidra
        size = None
        body_range = func.get("body", {})
        if body_range:
            min_addr = body_range.get("minAddress", "")
            max_addr = body_range.get("maxAddress", "")
            if min_addr and max_addr:
                if isinstance(min_addr, str):
                    min_addr = int(min_addr, 16)
                if isinstance(max_addr, str):
                    max_addr = int(max_addr, 16)
                size = max_addr - min_addr + 1
        
        # Preserve existing annotations if present
        existing = existing_symbols.get(address)
        type_hint = existing.type_hint if existing else None
        comment = existing.comment if existing else None
        
        # Use existing size if Ghidra size is not available
        if not size and existing and existing.size:
            size = existing.size
        
        new_symbols[address] = SymbolEntry(name, address, size, type_hint, comment)
    
    print(f"Exporting {len(new_symbols)} symbols...", file=sys.stderr)
    write_symbol_addrs(symbol_file, new_symbols, preserved_lines, dry_run)
    
    return True


def import_to_ghidra(port: int, symbol_file: str, dry_run: bool = False, 
                     filter_pattern: Optional[str] = None):
    """Import symbols from symbol_addrs.txt to Ghidra."""
    print(f"Loading symbols from {symbol_file}...", file=sys.stderr)
    symbols, _ = parse_symbol_addrs(symbol_file)
    
    if not symbols:
        print(f"Error: No symbols found in {symbol_file}", file=sys.stderr)
        return False
    
    print(f"Loaded {len(symbols)} symbols", file=sys.stderr)
    
    if filter_pattern:
        symbols = {addr: entry for addr, entry in symbols.items() 
                   if filter_pattern in entry.name}
        print(f"Filtered to {len(symbols)} symbols matching '{filter_pattern}'", 
              file=sys.stderr)
    
    if dry_run:
        print("\n(DRY RUN - no changes will be made)\n", file=sys.stderr)
    
    renamed = 0
    already_correct = 0
    not_found = 0
    errors = 0
    
    for addr, entry in sorted(symbols.items()):
        func = get_function_at(addr, port)
        
        if not func:
            not_found += 1
            continue
        
        current_name = func.get("name", "")
        
        if current_name == entry.name:
            already_correct += 1
            continue
        
        # Rename if needed
        print(f"  0x{addr:08X}: {current_name} -> {entry.name}", end="")
        
        if dry_run:
            print(" (would rename)")
            renamed += 1
        else:
            result = rename_function(addr, entry.name, port)
            if result.get("success") or result.get("name") == entry.name:
                print(" [OK]")
                renamed += 1
            else:
                print(f" [ERROR: {result.get('error', 'unknown')}]")
                errors += 1
    
    print(f"\nSummary:", file=sys.stderr)
    print(f"  Already correct: {already_correct}", file=sys.stderr)
    print(f"  Renamed: {renamed}", file=sys.stderr)
    print(f"  Not found: {not_found}", file=sys.stderr)
    print(f"  Errors: {errors}", file=sys.stderr)
    
    return True


def validate_sync(port: int, symbol_file: str):
    """Validate that Ghidra and symbol_addrs.txt are in sync."""
    print("Validating synchronization...", file=sys.stderr)
    
    # Load symbols from file
    file_symbols, _ = parse_symbol_addrs(symbol_file)
    print(f"Loaded {len(file_symbols)} symbols from {symbol_file}", file=sys.stderr)
    
    # Load functions from Ghidra
    ghidra_funcs = get_all_functions(port)
    print(f"Loaded {len(ghidra_funcs)} functions from Ghidra", file=sys.stderr)
    
    # Build Ghidra function map
    ghidra_symbols = {}
    for func in ghidra_funcs:
        address = func.get("entryPoint", 0)
        if isinstance(address, str):
            address = int(address, 16)
        name = func.get("name", "")
        if not name.startswith("FUN_") and not name.startswith("LAB_"):
            ghidra_symbols[address] = name
    
    # Compare
    mismatches = []
    in_file_not_ghidra = []
    in_ghidra_not_file = []
    
    # Check file -> Ghidra
    for addr, entry in file_symbols.items():
        if addr not in ghidra_symbols:
            in_file_not_ghidra.append((addr, entry.name))
        elif ghidra_symbols[addr] != entry.name:
            mismatches.append((addr, entry.name, ghidra_symbols[addr]))
    
    # Check Ghidra -> file
    for addr, name in ghidra_symbols.items():
        if addr not in file_symbols:
            in_ghidra_not_file.append((addr, name))
    
    # Report
    if mismatches:
        print("\n=== Name Mismatches ===")
        for addr, file_name, ghidra_name in mismatches:
            print(f"  0x{addr:08X}: file={file_name}, ghidra={ghidra_name}")
    
    if in_file_not_ghidra:
        print("\n=== In file but not in Ghidra ===")
        for addr, name in in_file_not_ghidra[:10]:  # Limit output
            print(f"  0x{addr:08X}: {name}")
        if len(in_file_not_ghidra) > 10:
            print(f"  ... and {len(in_file_not_ghidra) - 10} more")
    
    if in_ghidra_not_file:
        print("\n=== In Ghidra but not in file ===")
        for addr, name in in_ghidra_not_file[:10]:  # Limit output
            print(f"  0x{addr:08X}: {name}")
        if len(in_ghidra_not_file) > 10:
            print(f"  ... and {len(in_ghidra_not_file) - 10} more")
    
    print(f"\nSummary:", file=sys.stderr)
    print(f"  Mismatches: {len(mismatches)}", file=sys.stderr)
    print(f"  In file not Ghidra: {len(in_file_not_ghidra)}", file=sys.stderr)
    print(f"  In Ghidra not file: {len(in_ghidra_not_file)}", file=sys.stderr)
    
    if not mismatches and not in_file_not_ghidra and not in_ghidra_not_file:
        print("\n✓ Everything is in sync!", file=sys.stderr)
        return True
    
    return False


def main():
    parser = argparse.ArgumentParser(
        description="Bidirectional symbol sync between Ghidra and symbol_addrs.txt"
    )
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"Ghidra MCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--symbols", type=str, default=SYMBOL_FILE,
                        help=f"Symbol file path (default: {SYMBOL_FILE})")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would happen without making changes")
    
    # Mode selection
    mode_group = parser.add_mutually_exclusive_group(required=True)
    mode_group.add_argument("--export", action="store_true",
                            help="Export functions from Ghidra to symbol_addrs.txt")
    mode_group.add_argument("--import", action="store_true", dest="import_mode",
                            help="Import symbols from symbol_addrs.txt to Ghidra")
    mode_group.add_argument("--validate", action="store_true",
                            help="Validate that Ghidra and symbol_addrs.txt are in sync")
    
    # Additional options
    parser.add_argument("--filter", type=str, default=None,
                        help="Only process symbols matching this pattern")
    parser.add_argument("--no-preserve", action="store_true",
                        help="Don't preserve manual annotations during export")
    
    args = parser.parse_args()
    
    try:
        if args.export:
            success = export_from_ghidra(
                args.port, 
                args.symbols, 
                args.dry_run, 
                preserve_manual=not args.no_preserve
            )
        elif args.import_mode:
            success = import_to_ghidra(
                args.port,
                args.symbols,
                args.dry_run,
                args.filter
            )
        elif args.validate:
            success = validate_sync(args.port, args.symbols)
        
        sys.exit(0 if success else 1)
    
    except KeyboardInterrupt:
        print("\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
