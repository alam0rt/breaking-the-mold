#!/usr/bin/env python3
"""
Analyze all BLB files in disks/blb/ to compare structures across versions.

Compares:
- Level counts and metadata
- Movie tables
- Sector/loading screen tables
- Unknown fields (v04, v06, v0A patterns)
- State data region
"""

import sys
from pathlib import Path

# Add scripts dir to path
sys.path.insert(0, str(Path(__file__).parent))

from blb import BLBHeader, HEADER_SIZE

BLB_DIR = Path(__file__).parent.parent / "disks" / "blb"


def analyze_blb(filepath: Path) -> dict:
    """Analyze a single BLB file and return summary dict."""
    with open(filepath, "rb") as f:
        header_data = f.read(HEADER_SIZE)
        f.seek(0, 2)  # Seek to end
        file_size = f.tell()
    
    header = BLBHeader.from_bytes(header_data)
    
    # Gather level info
    levels = []
    for lvl in header.level_entries:
        levels.append({
            "idx": lvl.index,
            "id": lvl.level_id,
            "name": lvl.name.strip('\x00'),
            "sectors": lvl.sector_count,
            "tert_count": lvl.tert_block_count,
            "v04": lvl.unknown_04,
            "v06": lvl.unknown_06,
            "v08": lvl.entry1_offset_lo,
            "v0A": lvl.unknown_0A,
        })
    
    # Gather movie info
    movies = []
    for mov in header.movies:
        movies.append({
            "id": mov.movie_id,
            "short": mov.short_name,
            "filename": mov.filename.strip('\x00'),
            "sectors": mov.sector_count,
        })
    
    # Gather sector table info  
    sector_entries = []
    for ent in header.sector_entries:
        sector_entries.append({
            "idx": ent.index,
            "code": ent.code,
            "offset": ent.sector_offset,
            "count": ent.sector_count,
            "flags": ent.entry_flags,
        })
    
    return {
        "filename": filepath.name,
        "file_size": file_size,
        "file_size_mb": file_size / (1024 * 1024),
        "level_count": header.level_count,
        "movie_count": header.asset_count,
        "sector_entry_count": header.sector_table_count,
        "levels": levels,
        "movies": movies,
        "sector_entries": sector_entries,
        "state_data": {
            "game_mode": header.state_data.game_mode,
            "asset_index": header.state_data.asset_index,
        }
    }


def print_separator(char="=", width=80):
    print(char * width)


def main():
    blb_files = sorted(BLB_DIR.glob("*.blb")) + sorted(BLB_DIR.glob("*.BLB"))
    
    if not blb_files:
        print(f"No BLB files found in {BLB_DIR}")
        return 1
    
    print(f"Found {len(blb_files)} BLB files to analyze\n")
    
    all_data = {}
    for blb_path in blb_files:
        try:
            data = analyze_blb(blb_path)
            all_data[blb_path.name] = data
            print(f"✓ Parsed {blb_path.name}")
        except Exception as e:
            print(f"✗ Failed to parse {blb_path.name}: {e}")
    
    print()
    print_separator()
    print("SUMMARY: All BLB Files")
    print_separator()
    
    # Print overview table
    print(f"\n{'Filename':<20} {'Size MB':>8} {'Levels':>7} {'Movies':>7} {'Sectors':>8}")
    print("-" * 55)
    for name, data in sorted(all_data.items()):
        print(f"{name:<20} {data['file_size_mb']:>8.2f} {data['level_count']:>7} "
              f"{data['movie_count']:>7} {data['sector_entry_count']:>8}")
    
    # Compare levels across versions
    print_separator()
    print("\nLEVEL COMPARISON")
    print_separator()
    
    # Group by level count
    by_level_count = {}
    for name, data in all_data.items():
        cnt = data['level_count']
        by_level_count.setdefault(cnt, []).append(name)
    
    for cnt, files in sorted(by_level_count.items()):
        print(f"\n{cnt} levels: {', '.join(files)}")
        
        # Show level IDs for first file in group
        first_file = files[0]
        levels = all_data[first_file]['levels']
        ids = [f"{l['id']}" for l in levels]
        print(f"  Level IDs: {', '.join(ids)}")
    
    # Compare movies across versions
    print_separator()
    print("\nMOVIE COMPARISON")
    print_separator()
    
    for name, data in sorted(all_data.items()):
        movies = data['movies']
        if movies:
            movie_ids = [m['id'] for m in movies if m['id']]
            print(f"\n{name} ({len(movies)} movies):")
            for m in movies:
                if m['filename']:
                    print(f"  {m['id']:<6} {m['sectors']:>5} sectors  {m['filename']}")
        else:
            print(f"\n{name}: No movies")
    
    # Analyze unknown fields (v04, v06, v0A) patterns
    print_separator()
    print("\nUNKNOWN FIELD ANALYSIS (v04, v06, v0A)")
    print_separator()
    
    for name, data in sorted(all_data.items()):
        levels = data['levels']
        if not levels:
            continue
            
        print(f"\n{name}:")
        print(f"  {'ID':<6} {'v04':>6} {'v06':>6} {'v08':>6} {'v0A':>6} {'TertCnt':>8}")
        print(f"  {'-'*44}")
        
        for lvl in levels:
            print(f"  {lvl['id']:<6} {lvl['v04']:>6} {lvl['v06']:>6} "
                  f"{lvl['v08']:>6} {lvl['v0A']:>6} {lvl['tert_count']:>8}")
    
    # Cross-version field comparison
    print_separator()
    print("\nCROSS-VERSION FIELD VALUE RANGES")
    print_separator()
    
    # Collect all values across all versions
    all_v04 = set()
    all_v06 = set()
    all_v08 = set()
    all_v0A = set()
    all_tert = set()
    
    for name, data in all_data.items():
        for lvl in data['levels']:
            all_v04.add(lvl['v04'])
            all_v06.add(lvl['v06'])
            all_v08.add(lvl['v08'])
            all_v0A.add(lvl['v0A'])
            all_tert.add(lvl['tert_count'])
    
    print(f"\nv04 range: {min(all_v04)} - {max(all_v04)} (unique: {len(all_v04)})")
    print(f"v06 range: {min(all_v06)} - {max(all_v06)} (unique: {len(all_v06)})")
    print(f"v08 range: {min(all_v08)} - {max(all_v08)} (unique: {len(all_v08)})")
    print(f"v0A range: {min(all_v0A)} - {max(all_v0A)} (unique: {len(all_v0A)})")
    print(f"tert_count range: {min(all_tert)} - {max(all_tert)} (unique: {len(all_tert)})")
    
    # Check if values differ between versions for same level
    print_separator()
    print("\nVERSION DIFFERENCES (same level ID, different values)")
    print_separator()
    
    # Build level ID -> values map per version
    level_values = {}  # level_id -> {version: {field: value}}
    for name, data in all_data.items():
        for lvl in data['levels']:
            lid = lvl['id']
            if lid not in level_values:
                level_values[lid] = {}
            level_values[lid][name] = {
                'v04': lvl['v04'],
                'v06': lvl['v06'],
                'v08': lvl['v08'],
                'v0A': lvl['v0A'],
                'tert': lvl['tert_count'],
                'sectors': lvl['sectors'],
            }
    
    # Check for differences
    differences_found = False
    for lid, versions in sorted(level_values.items()):
        if len(versions) < 2:
            continue
        
        # Compare field values across versions
        fields = ['v04', 'v06', 'v08', 'v0A', 'tert', 'sectors']
        for field in fields:
            values = {ver: vals[field] for ver, vals in versions.items()}
            unique_values = set(values.values())
            if len(unique_values) > 1:
                if not differences_found:
                    differences_found = True
                print(f"\n{lid} - {field} differs:")
                for ver, val in sorted(values.items()):
                    print(f"  {ver}: {val}")
    
    if not differences_found:
        print("\nNo differences found in field values between versions for same levels!")
    
    # State data comparison
    print_separator()
    print("\nSTATE DATA REGION (0xF34+)")
    print_separator()
    
    for name, data in sorted(all_data.items()):
        state = data['state_data']
        print(f"{name}: game_mode={state['game_mode']}, asset_index={state['asset_index']}")
    
    # JP demo analysis (fewer levels)
    print_separator()
    print("\nJP DEMO/FULL VERSION ANALYSIS")
    print_separator()
    
    jp_files = [n for n in all_data.keys() if 'papx' in n.lower() or 'slps' in n.lower()]
    if jp_files:
        for jp_name in jp_files:
            jp_data = all_data[jp_name]
            print(f"\n{jp_name}:")
            print(f"  Levels: {jp_data['level_count']}")
            for lvl in jp_data['levels']:
                print(f"    {lvl['idx']:>2}: {lvl['id']} - {lvl['name']}")
            print(f"  Movies: {jp_data['movie_count']}")
            for mov in jp_data['movies']:
                if mov['filename']:
                    print(f"    {mov['id']}: {mov['filename']}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
