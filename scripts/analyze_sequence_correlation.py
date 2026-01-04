#!/usr/bin/env python3
"""
Analyze correlation between playback sequence and level metadata fields.

Hypothesis: The unknown fields (0x04, 0x06, 0x0A) in level metadata might
correlate with data interleaving patterns or shared asset loading.
"""

import sys
from pathlib import Path
from collections import defaultdict

sys.path.insert(0, str(Path(__file__).parent))
from blb import BLBFile, BLBHeader

def main():
    if len(sys.argv) < 2:
        blb_path = Path(__file__).parent.parent / "disks" / "blb" / "GAME.BLB"
    else:
        blb_path = sys.argv[1]
    
    blb = BLBFile(blb_path)
    header = blb.header
    state = header.state_data
    levels = header.level_entries
    
    print(f"Analyzing: {Path(blb_path).name}")
    print("=" * 100)
    
    # Extract playback sequence
    sequence = []
    for i in range(len(state.mode_array)):
        mode = state.mode_array[i]
        idx = state.index_array[i] if i < len(state.index_array) else 0
        if mode == 3:  # Level mode
            sequence.append((i, idx))
    
    print(f"\nPlayback Sequence Order (mode=3 levels only):")
    print("-" * 100)
    print(f"{'Seq#':>4} {'LvlIdx':>6} {'ID':<5} {'Name':<25} {'unk04':>6} {'unk06':>6} {'unk0A':>6} {'tert_cnt':>8} {'sec_off':>8}")
    print("-" * 100)
    
    for seq_pos, (seq_idx, level_idx) in enumerate(sequence):
        if level_idx < len(levels):
            lv = levels[level_idx]
            print(f"{seq_pos:4d} {level_idx:6d} {lv.level_id:<5} {lv.name[:25]:<25} "
                  f"{lv.unknown_04:6d} {lv.unknown_06:6d} {lv.unknown_0A:6d} "
                  f"{lv.tert_block_count:8d} {lv.secondary_offset:8d}")
    
    print("\n" + "=" * 100)
    print("Grouping by (unknown_06, unknown_0A) pairs:")
    print("-" * 100)
    
    # Group levels by (unknown_06, unknown_0A) pairs
    pairs = defaultdict(list)
    for idx, lv in enumerate(levels):
        pairs[(lv.unknown_06, lv.unknown_0A)].append((idx, lv))
    
    for (u06, u0a), lvs in sorted(pairs.items()):
        ids = ", ".join(lv.level_id for _, lv in lvs)
        seq_positions = []
        for lv_idx, lv in lvs:
            for seq_pos, (seq_i, seq_lv_idx) in enumerate(sequence):
                if seq_lv_idx == lv_idx:
                    seq_positions.append(seq_pos)
                    break
        seq_str = ", ".join(str(p) for p in seq_positions)
        print(f"  ({u06:2d}, {u0a:2d}): {ids:<50} seq_order: [{seq_str}]")
    
    print("\n" + "=" * 100)
    print("Grouping by unknown_04 ranges:")
    print("-" * 100)
    
    # Group by unknown_04 (seems like an offset)
    by_unk04 = sorted([(lv.unknown_04, idx, lv) for idx, lv in enumerate(levels)])
    for u04, idx, lv in by_unk04:
        print(f"  unk04={u04:6d}  [{idx:2d}] {lv.level_id}: {lv.name[:30]}")
    
    print("\n" + "=" * 100)
    print("Sector Offset Analysis (looking for consecutive sectors):")
    print("-" * 100)
    
    # Analyze sector layout - are levels with same (u06, u0a) close on disc?
    for idx, lv in enumerate(levels):
        pri_start = lv.sector_offset
        pri_end = pri_start + lv.sector_count
        sec_start = lv.secondary_offset
        sec_end = sec_start + lv.secondary_count
        print(f"  [{idx:2d}] {lv.level_id}: pri={pri_start:5d}-{pri_end:5d} ({lv.sector_count:4d}), "
              f"sec={sec_start:5d}-{sec_end:5d} ({lv.secondary_count:4d}), "
              f"(u06,u0A)=({lv.unknown_06:2d},{lv.unknown_0A:2d})")
    
    print("\n" + "=" * 100)
    print("Hypothesis Test: Do levels with same (u06, u0A) share sector ranges?")
    print("-" * 100)
    
    for (u06, u0a), lvs in sorted(pairs.items()):
        if len(lvs) > 1:
            min_pri = min(lv.sector_offset for _, lv in lvs)
            max_pri = max(lv.sector_offset + lv.sector_count for _, lv in lvs)
            min_sec = min(lv.secondary_offset for _, lv in lvs)
            max_sec = max(lv.secondary_offset + lv.secondary_count for _, lv in lvs)
            ids = ", ".join(lv.level_id for _, lv in lvs)
            print(f"  ({u06:2d}, {u0a:2d}): {ids}")
            print(f"           primary range: {min_pri:5d} - {max_pri:5d}")
            print(f"           secondary range: {min_sec:5d} - {max_sec:5d}")
    
    print("\n" + "=" * 100)
    print("Tertiary block count correlation:")
    print("-" * 100)
    
    # Group by tert_block_count
    by_tert = defaultdict(list)
    for idx, lv in enumerate(levels):
        by_tert[lv.tert_block_count].append((idx, lv))
    
    for tert_cnt, lvs in sorted(by_tert.items()):
        ids = ", ".join(lv.level_id for _, lv in lvs)
        u06_u0a = set((lv.unknown_06, lv.unknown_0A) for _, lv in lvs)
        print(f"  tert_block_count={tert_cnt}: {ids}")
        print(f"           (u06, u0A) values: {sorted(u06_u0a)}")

if __name__ == "__main__":
    main()
