#!/usr/bin/env python3
"""Print playback sequence info for a BLB file."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from blb import BLBFile

MODE_NAMES = {
    0: "Invalid/Reset",
    1: "Movie",
    2: "Credits",
    3: "Level",
    4: "Sector (special)",
    5: "Sector (initial)",
    6: "End",
}

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <blb_file>")
        sys.exit(1)
    
    blb = BLBFile(sys.argv[1])
    header = blb.header
    state = header.state_data
    
    # Build lookup tables
    levels = header.level_entries
    movies = header.movie_entries
    
    print(f"Playback Sequence for: {Path(sys.argv[1]).name}")
    print("=" * 80)
    print(f"Levels: {header.level_count}, Movies: {header.asset_count}")
    print(f"Base mode @ 0xF36: {state.game_mode} ({MODE_NAMES.get(state.game_mode, '?')})")
    print(f"Base index @ 0xF92: {state.asset_index}")
    print()
    
    print("Sequence (offset -> mode, index, details):")
    print("-" * 80)
    
    for i in range(len(state.mode_array)):
        mode = state.mode_array[i]
        idx = state.index_array[i] if i < len(state.index_array) else 0
        if mode != 0 or idx != 0:
            mode_name = MODE_NAMES.get(mode, "?")
            detail = ""
            
            if mode == 1:  # Movie
                if idx < len(movies):
                    m = movies[idx]
                    detail = f"-> {m.movie_id.rstrip(chr(0))}: {m.filename.rstrip(chr(0))}"
                else:
                    detail = f"-> (movie {idx} out of range)"
            elif mode == 3:  # Level
                if idx < len(levels):
                    lv = levels[idx]
                    detail = f"-> {lv.level_id.rstrip(chr(0))}: {lv.name.rstrip(chr(0))}"
                else:
                    detail = f"-> (level {idx} out of range)"
            elif mode == 2:  # Credits
                detail = f"-> credits sequence {idx}"
            elif mode == 6:  # End
                detail = f"-> end marker {idx}"
            
            print(f"  [{i:2d}] mode={mode} ({mode_name:15s}) idx={idx:2d}  {detail}")

if __name__ == "__main__":
    main()
