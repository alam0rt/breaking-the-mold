#!/usr/bin/env python3
"""Test BLB parsing against all available BLB files."""

from pathlib import Path
from blb import BLBHeader

BLB_DIR = Path(__file__).parent.parent / "disks" / "blb"

def test_blb(blb_path: Path) -> bool:
    """Test parsing a single BLB file."""
    print(f"\n{'='*60}")
    print(f"Testing: {blb_path.name}")
    print('='*60)
    
    try:
        with open(blb_path, 'rb') as f:
            data = f.read(4096)
        
        header = BLBHeader.from_bytes(data)
        
        print(f"  Levels: {header.level_count}, Movies: {header.asset_count}, Sectors: {header.sector_table_count}")
        
        # First level
        if header.level_entries:
            level = header.level_entries[0]
            print(f"  First level: {level.name!r} (ID: {level.level_id})")
            print(f"    Sector offset: {level.sector_offset}, count: {level.sector_count}")
        
        # Last level
        if len(header.level_entries) > 1:
            level = header.level_entries[-1]
            print(f"  Last level: {level.name!r} (ID: {level.level_id})")
        
        # First movie
        if header.movies:
            movie = header.movies[0]
            print(f"  First movie: {movie.movie_id} - {movie.filename}")
            print(f"    Sector count: {movie.sector_count}")
        
        # State data
        print(f"  State data:")
        print(f"    Raw size: {len(header.state_data.raw_data)} bytes")
        print(f"    Game mode @ 0xF36: {header.state_data.game_mode}")
        print(f"    Secondary flag @ 0xF37: {header.state_data.secondary_flag}")
        print(f"    Asset index @ 0xF92: {header.state_data.asset_index}")
        print(f"    Unknown F91: {header.state_data.unknown_f91}")
        
        # Playback sequence arrays (mode/index pairs for game flow)
        mode_array = header.state_data.mode_array
        index_array = header.state_data.index_array
        print(f"    Mode array (0xF38-0xF90): {len(mode_array)} bytes")
        print(f"      First 16 bytes: {mode_array[:16].hex()}")
        print(f"    Index array (0xF93-0xFFF): {len(index_array)} bytes")
        print(f"      First 16 bytes: {index_array[:16].hex()}")
        
        # Check for non-zero values in playback sequence
        nonzero_modes = [i for i, b in enumerate(mode_array) if b != 0]
        nonzero_indices = [i for i, b in enumerate(index_array) if b != 0]
        if nonzero_modes:
            print(f"    Non-zero mode offsets: {nonzero_modes[:10]}{'...' if len(nonzero_modes) > 10 else ''}")
        if nonzero_indices:
            print(f"    Non-zero index offsets: {nonzero_indices[:10]}{'...' if len(nonzero_indices) > 10 else ''}")
        
        return True
        
    except Exception as e:
        print(f"  ERROR: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    blb_files = list(BLB_DIR.glob("*.blb")) + list(BLB_DIR.glob("*.BLB"))
    blb_files = sorted(set(blb_files))
    
    print(f"Found {len(blb_files)} BLB files to test")
    
    results = {}
    for blb_path in blb_files:
        results[blb_path.name] = test_blb(blb_path)
    
    print("\n" + "="*60)
    print("SUMMARY")
    print("="*60)
    passed = sum(1 for v in results.values() if v)
    failed = sum(1 for v in results.values() if not v)
    print(f"Passed: {passed}/{len(results)}")
    if failed:
        print(f"Failed: {', '.join(k for k, v in results.items() if not v)}")
    
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    exit(main())
