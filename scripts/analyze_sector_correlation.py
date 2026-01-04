#!/usr/bin/env python3
"""
Analyze sector table entries and their relationship to levels.

The playback sequence has mode=4/5 entries that reference the sector table.
These might indicate shared loading resources between levels.
"""

import sys
from pathlib import Path
from collections import defaultdict

sys.path.insert(0, str(Path(__file__).parent))
from blb import BLBFile

def main():
    if len(sys.argv) < 2:
        blb_path = Path(__file__).parent.parent / "disks" / "blb" / "GAME.BLB"
    else:
        blb_path = sys.argv[1]
    
    blb = BLBFile(blb_path)
    header = blb.header
    state = header.state_data
    levels = header.level_entries
    sectors = header.sector_entries
    
    print(f"Analyzing: {Path(blb_path).name}")
    print("=" * 120)
    
    # First, show the sector table
    print("\nSector Table Entries:")
    print("-" * 120)
    print(f"{'Idx':>3} {'LvIdx':>5} {'Flags':>5} {'Unk':>4} {'Code':<5} {'Short':<5} {'SecOff':>7} {'SecCnt':>7} {'ByteOff':>10}")
    print("-" * 120)
    
    for s in sectors:
        print(f"{s.index:3d} {s.level_index:5d} 0x{s.entry_flags:02X}  0x{s.unknown_byte:02X} {s.code:<5} {s.short_name:<5} "
              f"{s.sector_offset:7d} {s.sector_count:7d} 0x{s.byte_offset:08X}")
    
    # Now correlate with playback sequence
    print("\n" + "=" * 120)
    print("Playback Sequence with Sector Entries (mode 4/5):")
    print("-" * 120)
    
    prev_level = None
    for i in range(len(state.mode_array)):
        mode = state.mode_array[i]
        idx = state.index_array[i] if i < len(state.index_array) else 0
        
        if mode == 4 or mode == 5:  # Sector modes
            if idx < len(sectors):
                s = sectors[idx]
                level_name = ""
                if s.level_index < len(levels):
                    level_name = levels[s.level_index].level_id
                print(f"  [{i:2d}] mode={mode} sector[{idx:2d}]: code={s.code:<5} level_idx={s.level_index:2d} ({level_name:<5}) "
                      f"flags=0x{s.entry_flags:02X} unk=0x{s.unknown_byte:02X} off={s.sector_offset}")
        elif mode == 3:  # Level
            if idx < len(levels):
                lv = levels[idx]
                prev_level = lv
                print(f"  [{i:2d}] mode=3 LEVEL[{idx:2d}]: {lv.level_id:<5} '{lv.name[:25]}' "
                      f"(u06={lv.unknown_06}, u0A={lv.unknown_0A})")
    
    # Check if sector entries point to the same level_index as the next level in sequence
    print("\n" + "=" * 120)
    print("Sector Entry to Level Correlation Analysis:")
    print("-" * 120)
    
    sequence = []
    for i in range(len(state.mode_array)):
        mode = state.mode_array[i]
        idx = state.index_array[i] if i < len(state.index_array) else 0
        if mode in (3, 4, 5):
            sequence.append((i, mode, idx))
    
    print("\nPattern: mode4/5 sector entries followed by mode3 level entries")
    print()
    
    for i, (seq_i, mode, idx) in enumerate(sequence):
        if mode in (4, 5) and idx < len(sectors):
            s = sectors[idx]
            # Look for next level entry
            next_level = None
            for j in range(i + 1, len(sequence)):
                if sequence[j][1] == 3:
                    next_level_idx = sequence[j][2]
                    if next_level_idx < len(levels):
                        next_level = levels[next_level_idx]
                    break
            
            match = ""
            if next_level:
                # Check if sector entry's level_index matches the upcoming level
                if s.level_index == next_level.index:
                    match = "✓ MATCH"
                elif s.code.strip('\x00') == next_level.level_id.strip('\x00'):
                    match = "✓ CODE MATCH"
                else:
                    match = f"✗ sector→lvl[{s.level_index}], seq→lvl[{next_level.index}]"
            
            print(f"  sector[{idx:2d}] code={s.code:<5} level_idx={s.level_index:2d} "
                  f"→ next_level={next_level.level_id if next_level else 'None':<5} {match}")
    
    # Analyze sector entries by level_index to find shared resources
    print("\n" + "=" * 120)
    print("Sector Entries Grouped by Level Index (shared loading screens?):")
    print("-" * 120)
    
    by_level_idx = defaultdict(list)
    for s in sectors:
        by_level_idx[s.level_index].append(s)
    
    for level_idx, sector_list in sorted(by_level_idx.items()):
        if level_idx < len(levels):
            lv = levels[level_idx]
            lv_info = f"{lv.level_id} (u06={lv.unknown_06}, u0A={lv.unknown_0A})"
        else:
            lv_info = f"(special index {level_idx})"
        
        sector_codes = [s.code for s in sector_list]
        print(f"  level_idx={level_idx:2d} {lv_info:<35}: sectors={sector_codes}")

if __name__ == "__main__":
    main()
