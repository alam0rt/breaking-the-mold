#!/usr/bin/env python3
"""
Detect compilation unit boundaries in PSX binary.

Analyzes function/data layout to find likely .c file boundaries:
1. Large gaps between functions (linker padding between .o files)
2. Transitions from code to rodata and back
3. Function reference patterns (static vs extern)
4. Symbol naming clusters

Usage:
    python3 scripts/detect_compilation_units.py
    python3 scripts/detect_compilation_units.py --verbose
    python3 scripts/detect_compilation_units.py --output units.json
"""

import argparse
import json
import subprocess
import re
from pathlib import Path
from collections import defaultdict
from urllib.request import urlopen
from typing import List, Dict, Tuple, Optional

GHIDRA_URL = "http://localhost:8192"
BINARY_PATH = Path("disks/pal/SLES_010.90")

# Thresholds for detecting boundaries
GAP_THRESHOLD = 0x40        # Gaps >= 64 bytes suggest file boundary
RODATA_THRESHOLD = 0x20     # Rodata sections >= 32 bytes
ALIGNMENT_SIZE = 0x10       # PSX linker typically aligns to 16 bytes


def get_functions_from_ghidra() -> List[Dict]:
    """Fetch all functions from Ghidra."""
    functions = []
    offset = 0
    limit = 500
    
    while True:
        try:
            url = f"{GHIDRA_URL}/functions?offset={offset}&limit={limit}"
            with urlopen(url, timeout=30) as resp:
                data = json.load(resp)
                batch = data.get("result", [])
                if not batch:
                    break
                functions.extend(batch)
                offset += limit
                if len(batch) < limit:
                    break
        except Exception as e:
            print(f"Error fetching functions: {e}")
            break
    
    return functions


def get_data_from_ghidra() -> List[Dict]:
    """Fetch defined data items from Ghidra."""
    data_items = []
    offset = 0
    limit = 500
    
    while True:
        try:
            url = f"{GHIDRA_URL}/data?offset={offset}&limit={limit}"
            with urlopen(url, timeout=30) as resp:
                data = json.load(resp)
                batch = data.get("result", [])
                if not batch:
                    break
                data_items.extend(batch)
                offset += limit
                if len(batch) < limit:
                    break
        except Exception as e:
            print(f"Error fetching data: {e}")
            break
    
    return data_items


def parse_symbol_addrs() -> Dict[int, Dict]:
    """Parse symbol_addrs.txt for additional symbol info."""
    symbols = {}
    symbol_file = Path("symbol_addrs.txt")
    
    if not symbol_file.exists():
        return symbols
    
    pattern = re.compile(r'^(\w+)\s*=\s*0x([0-9A-Fa-f]+)\s*;?\s*(?://\s*(.*))?$')
    
    for line in symbol_file.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith('//'):
            continue
        
        match = pattern.match(line)
        if match:
            name, addr_hex, comment = match.groups()
            addr = int(addr_hex, 16)
            
            # Parse size from comment if present
            size = 0
            if comment:
                size_match = re.search(r'size:0x([0-9A-Fa-f]+)', comment)
                if size_match:
                    size = int(size_match.group(1), 16)
            
            symbols[addr] = {
                'name': name,
                'address': addr,
                'size': size,
                'comment': comment or ''
            }
    
    return symbols


def analyze_function_gaps(functions: List[Dict]) -> List[Dict]:
    """Find gaps between functions that suggest file boundaries."""
    boundaries = []
    
    # Sort functions by address
    sorted_funcs = sorted(functions, key=lambda f: int(f.get('address', '0'), 16))
    
    prev_end = None
    prev_func = None
    
    for func in sorted_funcs:
        addr = int(func.get('address', '0'), 16)
        size = func.get('size', 0) or 0
        name = func.get('name', 'unknown')
        
        # Skip non-game code (BIOS, libraries at high addresses)
        if addr < 0x80010000 or addr > 0x800A0000:
            continue
        
        if prev_end is not None:
            gap = addr - prev_end
            
            if gap >= GAP_THRESHOLD:
                boundaries.append({
                    'type': 'function_gap',
                    'address': addr,
                    'gap_size': gap,
                    'prev_function': prev_func.get('name', 'unknown') if prev_func else None,
                    'prev_end': prev_end,
                    'next_function': name,
                    'confidence': min(100, 50 + (gap // 0x10) * 5)  # Higher gap = higher confidence
                })
        
        prev_end = addr + size if size > 0 else addr + 4
        prev_func = func
    
    return boundaries


def analyze_rodata_transitions(functions: List[Dict], data_items: List[Dict]) -> List[Dict]:
    """Find transitions from code to rodata that suggest file boundaries."""
    boundaries = []
    
    # Build sorted list of all items
    items = []
    
    for func in functions:
        addr = int(func.get('address', '0'), 16)
        if 0x80010000 <= addr <= 0x800A0000:
            items.append({
                'address': addr,
                'type': 'code',
                'name': func.get('name', 'unknown'),
                'size': func.get('size', 0) or 0
            })
    
    for data in data_items:
        addr = int(data.get('address', '0'), 16)
        if 0x80010000 <= addr <= 0x800A0000:
            items.append({
                'address': addr,
                'type': 'data',
                'name': data.get('label', 'unknown'),
                'size': 4  # Approximate
            })
    
    items.sort(key=lambda x: x['address'])
    
    # Look for code -> data -> code transitions
    prev_type = None
    data_start = None
    data_size = 0
    
    for i, item in enumerate(items):
        if item['type'] == 'data' and prev_type == 'code':
            # Start of data section
            data_start = item['address']
            data_size = item['size']
        elif item['type'] == 'data' and prev_type == 'data':
            # Continue data section
            data_size = item['address'] - data_start + item['size']
        elif item['type'] == 'code' and prev_type == 'data':
            # End of data section - this might be a file boundary
            if data_size >= RODATA_THRESHOLD:
                boundaries.append({
                    'type': 'rodata_section',
                    'address': item['address'],
                    'data_start': data_start,
                    'data_size': data_size,
                    'next_function': item['name'],
                    'confidence': min(100, 40 + (data_size // 0x20) * 10)
                })
            data_start = None
            data_size = 0
        
        prev_type = item['type']
    
    return boundaries


def analyze_naming_patterns(symbols: Dict[int, Dict]) -> List[Dict]:
    """Find clusters of similarly-named functions that suggest file boundaries."""
    boundaries = []
    
    # Sort symbols by address
    sorted_syms = sorted(symbols.items(), key=lambda x: x[0])
    
    # Look for naming pattern changes
    prev_prefix = None
    cluster_start = None
    cluster_count = 0
    
    for addr, sym in sorted_syms:
        name = sym['name']
        
        # Skip auto-generated names
        if name.startswith(('FUN_', 'DAT_', 'PTR_', 'LAB_', 'null_')):
            continue
        
        # Extract prefix (e.g., "Entity" from "EntityTickLoop")
        prefix_match = re.match(r'^([A-Z][a-z]+)', name)
        if not prefix_match:
            prefix_match = re.match(r'^([a-z]+_)', name)  # e.g., "init_"
        
        prefix = prefix_match.group(1) if prefix_match else None
        
        if prefix and prefix != prev_prefix:
            if cluster_count >= 3:  # End of a cluster
                boundaries.append({
                    'type': 'naming_cluster',
                    'address': addr,
                    'prev_prefix': prev_prefix,
                    'new_prefix': prefix,
                    'cluster_count': cluster_count,
                    'confidence': min(100, 30 + cluster_count * 5)
                })
            cluster_start = addr
            cluster_count = 1
        elif prefix == prev_prefix:
            cluster_count += 1
        
        prev_prefix = prefix
    
    return boundaries


def analyze_alignment_padding(binary_path: Path) -> List[Dict]:
    """Look for 16-byte aligned boundaries with NOP padding."""
    boundaries = []
    
    if not binary_path.exists():
        return boundaries
    
    data = binary_path.read_bytes()
    
    # PSX executable starts at offset 0x800
    code_start = 0x800
    
    # Look for sequences of NOPs (0x00000000) at aligned addresses
    nop_pattern = b'\x00\x00\x00\x00'
    
    i = code_start
    while i < len(data) - 16:
        # Check if we're at a 16-byte boundary
        vram_addr = 0x80010000 + (i - code_start)
        
        if vram_addr % 0x10 == 0:
            # Check for NOP padding (4+ consecutive NOPs)
            nop_count = 0
            j = i
            while j < len(data) - 4 and data[j:j+4] == nop_pattern:
                nop_count += 1
                j += 4
            
            if nop_count >= 2:  # 8+ bytes of NOPs
                boundaries.append({
                    'type': 'nop_padding',
                    'address': vram_addr,
                    'nop_count': nop_count,
                    'padding_size': nop_count * 4,
                    'confidence': min(100, 30 + nop_count * 10)
                })
                i = j
                continue
        
        i += 4
    
    return boundaries


def merge_boundaries(all_boundaries: List[Dict], merge_distance: int = 0x100) -> List[Dict]:
    """Merge nearby boundary detections into single file boundaries."""
    if not all_boundaries:
        return []
    
    # Sort by address
    sorted_bounds = sorted(all_boundaries, key=lambda x: x['address'])
    
    merged = []
    current_group = [sorted_bounds[0]]
    
    for bound in sorted_bounds[1:]:
        if bound['address'] - current_group[-1]['address'] <= merge_distance:
            current_group.append(bound)
        else:
            # Finalize current group
            merged.append(finalize_group(current_group))
            current_group = [bound]
    
    # Don't forget last group
    merged.append(finalize_group(current_group))
    
    return merged


def finalize_group(group: List[Dict]) -> Dict:
    """Combine multiple boundary signals into one."""
    # Use the lowest address
    addr = min(b['address'] for b in group)
    
    # Combine confidence scores
    total_confidence = sum(b['confidence'] for b in group)
    avg_confidence = total_confidence // len(group)
    
    # Collect evidence types
    evidence_types = list(set(b['type'] for b in group))
    
    # Boost confidence if multiple signal types agree
    if len(evidence_types) > 1:
        avg_confidence = min(100, avg_confidence + len(evidence_types) * 10)
    
    return {
        'address': addr,
        'confidence': avg_confidence,
        'evidence': evidence_types,
        'signals': len(group),
        'details': group
    }


def suggest_file_names(boundaries: List[Dict], symbols: Dict[int, Dict]) -> List[Dict]:
    """Suggest source file names based on nearby function names."""
    for bound in boundaries:
        addr = bound['address']
        
        # Find the first named function at or after this address
        candidates = [
            sym for a, sym in symbols.items()
            if a >= addr and not sym['name'].startswith(('FUN_', 'DAT_', 'PTR_'))
        ]
        
        if candidates:
            first_func = min(candidates, key=lambda x: x['address'])
            name = first_func['name']
            
            # Convert CamelCase to snake_case for file name
            file_name = re.sub(r'([a-z])([A-Z])', r'\1_\2', name).lower()
            
            # Extract likely module name
            parts = file_name.split('_')
            if len(parts) >= 2:
                module = parts[0]
            else:
                module = file_name[:8]
            
            bound['suggested_file'] = f"{module}.c"
            bound['first_function'] = name
    
    return boundaries


def main():
    parser = argparse.ArgumentParser(description="Detect compilation unit boundaries")
    parser.add_argument("--verbose", "-v", action="store_true", help="Show detailed output")
    parser.add_argument("--output", "-o", help="Output JSON file")
    parser.add_argument("--min-confidence", type=int, default=50, help="Minimum confidence threshold")
    args = parser.parse_args()
    
    print("Fetching functions from Ghidra...")
    functions = get_functions_from_ghidra()
    print(f"  Found {len(functions)} functions")
    
    print("Fetching data items from Ghidra...")
    data_items = get_data_from_ghidra()
    print(f"  Found {len(data_items)} data items")
    
    print("Parsing symbol_addrs.txt...")
    symbols = parse_symbol_addrs()
    print(f"  Found {len(symbols)} symbols")
    
    print("\nAnalyzing for file boundaries...")
    
    all_boundaries = []
    
    # Method 1: Function gaps
    gaps = analyze_function_gaps(functions)
    print(f"  Function gaps: {len(gaps)} candidates")
    all_boundaries.extend(gaps)
    
    # Method 2: Rodata transitions
    rodata = analyze_rodata_transitions(functions, data_items)
    print(f"  Rodata sections: {len(rodata)} candidates")
    all_boundaries.extend(rodata)
    
    # Method 3: Naming patterns
    naming = analyze_naming_patterns(symbols)
    print(f"  Naming clusters: {len(naming)} candidates")
    all_boundaries.extend(naming)
    
    # Method 4: NOP padding (from binary)
    if BINARY_PATH.exists():
        padding = analyze_alignment_padding(BINARY_PATH)
        print(f"  NOP padding: {len(padding)} candidates")
        all_boundaries.extend(padding)
    
    # Merge nearby detections
    print("\nMerging nearby boundaries...")
    merged = merge_boundaries(all_boundaries)
    print(f"  Merged to {len(merged)} boundaries")
    
    # Filter by confidence
    filtered = [b for b in merged if b['confidence'] >= args.min_confidence]
    print(f"  {len(filtered)} boundaries above {args.min_confidence}% confidence")
    
    # Suggest file names
    filtered = suggest_file_names(filtered, symbols)
    
    # Sort by address
    filtered.sort(key=lambda x: x['address'])
    
    # Output results
    print(f"\n{'='*70}")
    print("DETECTED COMPILATION UNIT BOUNDARIES")
    print(f"{'='*70}\n")
    
    for i, bound in enumerate(filtered):
        addr = bound['address']
        conf = bound['confidence']
        evidence = ', '.join(bound['evidence'])
        suggested = bound.get('suggested_file', 'unknown.c')
        first_func = bound.get('first_function', '')
        
        print(f"{i+1:3}. 0x{addr:08X}  [{conf:3}%]  {suggested:20}  ({evidence})")
        if args.verbose and first_func:
            print(f"     First function: {first_func}")
            for detail in bound.get('details', [])[:3]:
                print(f"       - {detail['type']}: {detail.get('gap_size', detail.get('data_size', ''))}")
        print()
    
    # Output JSON if requested
    if args.output:
        output_path = Path(args.output)
        output_path.write_text(json.dumps(filtered, indent=2))
        print(f"\nSaved to {output_path}")
    
    # Summary
    print(f"\n{'='*70}")
    print("SUMMARY")
    print(f"{'='*70}")
    print(f"Total boundaries detected: {len(filtered)}")
    print(f"Address range: 0x{filtered[0]['address']:08X} - 0x{filtered[-1]['address']:08X}")
    
    # Estimate file count
    high_conf = [b for b in filtered if b['confidence'] >= 70]
    print(f"High confidence (≥70%): {len(high_conf)} likely source files")


if __name__ == "__main__":
    main()
