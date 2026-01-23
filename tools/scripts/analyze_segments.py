#!/usr/bin/env python3
"""
Analyze Ghidra-exported symbols and suggest segment groupings.

This script reads the exported symbols and groups them by:
1. Address ranges (detecting natural gaps)
2. Function name patterns (Init, Tick, Destroy, etc.)
3. Cross-references (functions that call each other)

Usage:
    python3 tools/scripts/analyze_segments.py [--export symbols.txt]
"""

import sys
import re
from collections import defaultdict

def load_symbols(filepath=None):
    """Load symbols from export file or stdin."""
    functions = []
    data_items = []
    
    lines = []
    if filepath:
        with open(filepath, 'r') as f:
            lines = f.readlines()
    else:
        # Run the export command
        import subprocess
        result = subprocess.run(
            ['python3', 'tools/scripts/export_ghidra_symbols.py', '--format=text'],
            capture_output=True, text=True
        )
        lines = result.stdout.split('\n')
    
    for line in lines:
        if not line.strip() or line.startswith('#'):
            continue
        parts = line.split()
        if len(parts) >= 4:
            try:
                addr = int(parts[0], 16)
                sym_type = parts[2]
                name = parts[3]
                
                # Only process RAM addresses
                if 0x80010000 <= addr <= 0x80090FFF:
                    if sym_type == 'function':
                        functions.append((addr, name))
                    else:
                        data_items.append((addr, name))
            except ValueError:
                continue
    
    return sorted(functions), sorted(data_items)

def categorize_function(name):
    """Categorize a function based on its name pattern."""
    name_lower = name.lower()
    
    patterns = [
        (r'init|create|spawn', 'init'),
        (r'destroy|free|delete', 'destroy'),
        (r'tick|update|callback', 'tick'),
        (r'render|draw|display', 'render'),
        (r'collision|overlap|check', 'collision'),
        (r'player', 'player'),
        (r'enemy|attack|damage', 'enemy'),
        (r'boss|klogg|joe.*head|glenn', 'boss'),
        (r'finn|runn|vehicle', 'vehicle'),
        (r'menu|button|cursor|password', 'menu'),
        (r'level|load|asset|blb', 'level'),
        (r'hud|digit|timer|countdown', 'hud'),
        (r'sound|audio|spu|voice', 'audio'),
        (r'projectile|bullet|missile|homing', 'projectile'),
        (r'collect|orb|swirl|pickup', 'collectible'),
        (r'decor|path.*follow|platform', 'decor'),
        (r'anim|frame|sprite', 'animation'),
        (r'vram|texture|clut', 'vram'),
        (r'tile|layer|tilemap', 'tilemap'),
    ]
    
    for pattern, category in patterns:
        if re.search(pattern, name_lower):
            return category
    
    return 'unknown'

def find_natural_boundaries(functions, gap_threshold=0x200):
    """Find address gaps that suggest natural segment boundaries."""
    boundaries = []
    
    for i in range(len(functions) - 1):
        curr_addr = functions[i][0]
        next_addr = functions[i + 1][0]
        gap = next_addr - curr_addr
        
        if gap > gap_threshold:
            boundaries.append({
                'address': next_addr,
                'gap': gap,
                'before': functions[i][1],
                'after': functions[i + 1][1]
            })
    
    return boundaries

def analyze_segment(functions, start_addr, end_addr, name):
    """Analyze functions within a segment."""
    segment_funcs = [(a, n) for a, n in functions if start_addr <= a <= end_addr]
    
    categories = defaultdict(list)
    for addr, fname in segment_funcs:
        cat = categorize_function(fname)
        categories[cat].append((addr, fname))
    
    return {
        'name': name,
        'start': start_addr,
        'end': end_addr,
        'total': len(segment_funcs),
        'categories': dict(categories)
    }

def print_segment_analysis(analysis):
    """Print detailed segment analysis."""
    print(f"\n{'='*60}")
    print(f"Segment: {analysis['name']}")
    print(f"Range: {hex(analysis['start'])} - {hex(analysis['end'])}")
    print(f"Total functions: {analysis['total']}")
    print(f"{'='*60}")
    
    # Sort categories by count
    sorted_cats = sorted(
        analysis['categories'].items(),
        key=lambda x: len(x[1]),
        reverse=True
    )
    
    for cat, funcs in sorted_cats:
        print(f"\n  {cat.upper()} ({len(funcs)} functions):")
        for addr, name in funcs[:5]:
            print(f"    {hex(addr)}: {name}")
        if len(funcs) > 5:
            print(f"    ... and {len(funcs)-5} more")

def suggest_subsegments(analysis):
    """Suggest subsegment boundaries based on category clustering."""
    suggestions = []
    
    # Sort all functions by address
    all_funcs = []
    for cat, funcs in analysis['categories'].items():
        for addr, name in funcs:
            all_funcs.append((addr, name, cat))
    all_funcs.sort()
    
    if not all_funcs:
        return suggestions
    
    # Find runs of same category
    current_cat = all_funcs[0][2]
    run_start = all_funcs[0][0]
    run_count = 1
    
    for i in range(1, len(all_funcs)):
        addr, name, cat = all_funcs[i]
        prev_addr = all_funcs[i-1][0]
        
        # New subsegment if category changes or large gap
        if cat != current_cat or (addr - prev_addr) > 0x400:
            if run_count >= 5:  # Only suggest if meaningful cluster
                suggestions.append({
                    'start': run_start,
                    'category': current_cat,
                    'count': run_count
                })
            current_cat = cat
            run_start = addr
            run_count = 1
        else:
            run_count += 1
    
    # Don't forget the last run
    if run_count >= 5:
        suggestions.append({
            'start': run_start,
            'category': current_cat,
            'count': run_count
        })
    
    return suggestions

def main():
    print("Loading symbols from Ghidra export...")
    functions, data_items = load_symbols()
    
    print(f"Loaded {len(functions)} functions and {len(data_items)} data items")
    
    # Current segments from YAML
    segments = [
        (0x80010000, 0x800131EF, 'EARLY'),
        (0x800131F0, 0x8001A0C7, 'RENDER'),
        (0x8001A0C8, 0x8002A377, 'ENTITY'),
        (0x8002A378, 0x80058167, 'OBJECT'),
        (0x80058168, 0x80071047, 'PLAYER'),
        (0x80071048, 0x80082FFF, 'GAMELOOP'),
        (0x80083000, 0x80090FFF, 'LIBS'),
    ]
    
    print("\n" + "="*60)
    print("NATURAL BOUNDARIES (gaps > 512 bytes)")
    print("="*60)
    
    boundaries = find_natural_boundaries(functions)
    for b in boundaries[:20]:
        if 0x80010000 <= b['address'] <= 0x80083000:
            print(f"\n{hex(b['address'])}: gap of {b['gap']} bytes")
            print(f"  Before: {b['before'][:40]}")
            print(f"  After:  {b['after'][:40]}")
    
    print("\n" + "="*60)
    print("SEGMENT ANALYSIS")
    print("="*60)
    
    for start, end, name in segments:
        if name == 'LIBS':
            continue  # Skip LIBS, it's PSY-Q
        
        analysis = analyze_segment(functions, start, end, name)
        print_segment_analysis(analysis)
        
        print("\n  SUGGESTED SUBSEGMENTS:")
        suggestions = suggest_subsegments(analysis)
        for s in suggestions:
            rom_offset = (s['start'] - 0x80010000) + 0x800
            print(f"    - [0x{rom_offset:05X}, asm, Game/{name}/{s['category']}]  # {s['count']} funcs @ {hex(s['start'])}")

if __name__ == '__main__':
    main()
