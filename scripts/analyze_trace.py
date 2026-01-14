#!/usr/bin/env python3
"""
Skullmonkeys Trace Analyzer

Parses JSONL trace files from game_watcher.lua and provides analysis tools
for verifying decompilation accuracy and debugging game behavior.

Usage:
    python3 analyze_trace.py /tmp/skullmonkeys_trace.jsonl
    python3 analyze_trace.py trace.jsonl --stats
    python3 analyze_trace.py trace.jsonl --player-path output.csv
"""

import json
import sys
import argparse
from pathlib import Path
from collections import defaultdict, Counter
from typing import Dict, List, Any


def load_trace(filepath: Path) -> List[Dict]:
    """Load JSONL trace file"""
    entries = []
    with open(filepath, 'r') as f:
        for line_num, line in enumerate(f, 1):
            try:
                entries.append(json.loads(line))
            except json.JSONDecodeError as e:
                print(f"Warning: Line {line_num} invalid JSON: {e}", file=sys.stderr)
    return entries


def print_statistics(entries: List[Dict]):
    """Print trace statistics"""
    print("=== Trace Statistics ===\n")
    
    # Event type counts
    event_types = Counter(e['type'] for e in entries)
    print("Event Types:")
    for event_type, count in event_types.most_common():
        print(f"  {event_type:25} {count:8,} ({100*count/len(entries):.1f}%)")
    
    print(f"\nTotal Events: {len(entries):,}")
    
    # Frame range
    frames = [e['frame'] for e in entries if 'frame' in e]
    if frames:
        print(f"Frame Range: {min(frames)} - {max(frames)} ({max(frames) - min(frames)} frames)")
    
    # Level loads
    level_loads = [e for e in entries if e['type'] == 'LevelLoad']
    if level_loads:
        print(f"\nLevel Loads: {len(level_loads)}")
        for load in level_loads:
            blb = load['data'].get('blb_metadata', {})
            if blb.get('valid'):
                level = blb['level']
                current = blb['current']
                print(f"  Frame {load['frame']:6}: {level['id']} ({level['name']}) Stage {current['stage_index']}")
    
    # Player state changes
    state_changes = [e for e in entries if e['type'] == 'PlayerStateChange']
    if state_changes:
        states = Counter(e['data']['callback'] for e in state_changes)
        print(f"\nPlayer State Changes: {len(state_changes)}")
        for state, count in states.most_common(10):
            print(f"  {state:20} {count:6,}")
    
    # Entity types seen
    entity_types = set()
    for e in entries:
        if e['type'] == 'FrameState' and 'entities' in e['data']:
            for ent in e['data']['entities']:
                entity_types.add((ent['type'], ent['type_name']))
    
    if entity_types:
        print(f"\nEntity Types Seen: {len(entity_types)}")
        for type_id, type_name in sorted(entity_types):
            print(f"  Type {type_id:3}: {type_name}")


def extract_player_path(entries: List[Dict], output_csv: Path):
    """Extract player position trace to CSV"""
    import csv
    
    positions = []
    for e in entries:
        if e['type'] == 'FrameState' and 'player' in e['data']:
            player = e['data']['player']
            positions.append({
                'frame': e['frame'],
                'x': player['x'],
                'y': player['y'],
                'vx': player.get('vx', 0),
                'vy': player.get('vy', 0),
                'facing': player.get('facing', 'unknown'),
                'state': player.get('state', 'unknown'),
                'input': player.get('input_hex', '0x0000'),
            })
    
    with open(output_csv, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['frame', 'x', 'y', 'vx', 'vy', 'facing', 'state', 'input'])
        writer.writeheader()
        writer.writerows(positions)
    
    print(f"Wrote {len(positions)} player positions to {output_csv}")


def verify_entity_consistency(entries: List[Dict]):
    """Verify entity data consistency"""
    print("=== Entity Consistency Check ===\n")
    
    issues = []
    entity_tracking = defaultdict(list)  # ptr -> [(frame, data)]
    
    for e in entries:
        if e['type'] == 'FrameState' and 'entities' in e['data']:
            for ent in e['data']['entities']:
                ptr = ent['ptr']
                entity_tracking[ptr].append((e['frame'], ent))
    
    # Check for entities that teleport
    for ptr, history in entity_tracking.items():
        if len(history) < 2:
            continue
        
        for i in range(len(history) - 1):
            frame1, ent1 = history[i]
            frame2, ent2 = history[i + 1]
            
            dx = abs(ent2['x'] - ent1['x'])
            dy = abs(ent2['y'] - ent1['y'])
            
            # If moved >200 pixels in one frame (likely respawn/teleport)
            if dx > 200 or dy > 200:
                issues.append({
                    'type': 'teleport',
                    'ptr': ptr,
                    'entity_type': ent1['type_name'],
                    'frame': frame2,
                    'distance': (dx**2 + dy**2)**0.5,
                })
    
    if issues:
        print(f"Found {len(issues)} potential issues:\n")
        for issue in issues[:10]:  # Show first 10
            print(f"  Frame {issue['frame']}: {issue['entity_type']} ({issue['ptr']}) "
                  f"teleported {issue['distance']:.1f} pixels")
    else:
        print("No consistency issues found.")


def extract_asset_loading(entries: List[Dict]):
    """Extract asset loading sequence"""
    print("=== Asset Loading Sequence ===\n")
    
    loads = [e for e in entries if e['type'] == 'LoadAssetContainer']
    
    by_level = defaultdict(list)
    for load in loads:
        data = load['data']
        level_id = data.get('level_id', 'UNKNOWN')
        by_level[level_id].append(data)
    
    for level_id, level_loads in sorted(by_level.items()):
        print(f"{level_id}:")
        segments = defaultdict(list)
        for load in level_loads:
            segments[load['segment_type']].append(load['asset_index'])
        
        for segment, indices in sorted(segments.items()):
            print(f"  {segment:10} stages: {sorted(set(indices))}")
        print()


def compare_traces(trace1_path: Path, trace2_path: Path):
    """Compare two traces for differences (e.g., matching vs reference)"""
    print("=== Trace Comparison ===\n")
    
    entries1 = load_trace(trace1_path)
    entries2 = load_trace(trace2_path)
    
    print(f"Trace 1: {len(entries1)} entries")
    print(f"Trace 2: {len(entries2)} entries")
    
    # Compare frame states
    frames1 = {e['frame']: e for e in entries1 if e['type'] == 'FrameState'}
    frames2 = {e['frame']: e for e in entries2 if e['type'] == 'FrameState'}
    
    common_frames = set(frames1.keys()) & set(frames2.keys())
    print(f"\nCommon frames: {len(common_frames)}")
    
    # Compare player positions in common frames
    position_diffs = []
    for frame in sorted(common_frames):
        player1 = frames1[frame]['data'].get('player')
        player2 = frames2[frame]['data'].get('player')
        
        if player1 and player2:
            dx = abs(player1['x'] - player2['x'])
            dy = abs(player1['y'] - player2['y'])
            if dx > 1 or dy > 1:
                position_diffs.append((frame, dx, dy))
    
    if position_diffs:
        print(f"\nPlayer position differences: {len(position_diffs)}")
        for frame, dx, dy in position_diffs[:10]:
            print(f"  Frame {frame}: Δx={dx}, Δy={dy}")
    else:
        print("\nPlayer positions match!")


def main():
    parser = argparse.ArgumentParser(description="Analyze Skullmonkeys trace files")
    parser.add_argument('trace', type=Path, help="JSONL trace file")
    parser.add_argument('--stats', action='store_true', help="Show statistics")
    parser.add_argument('--player-path', type=Path, help="Export player path to CSV")
    parser.add_argument('--verify', action='store_true', help="Verify consistency")
    parser.add_argument('--assets', action='store_true', help="Show asset loading")
    parser.add_argument('--compare', type=Path, help="Compare with another trace")
    
    args = parser.parse_args()
    
    if not args.trace.exists():
        print(f"Error: {args.trace} not found", file=sys.stderr)
        return 1
    
    entries = load_trace(args.trace)
    print(f"Loaded {len(entries)} entries from {args.trace}\n")
    
    if args.stats or not any([args.player_path, args.verify, args.assets, args.compare]):
        print_statistics(entries)
    
    if args.player_path:
        extract_player_path(entries, args.player_path)
    
    if args.verify:
        verify_entity_consistency(entries)
    
    if args.assets:
        extract_asset_loading(entries)
    
    if args.compare:
        compare_traces(args.trace, args.compare)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
