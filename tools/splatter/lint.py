#!/usr/bin/env python3
"""
Lint - Validate splat configuration file.

Checks for common issues like:
- Segments out of order
- Duplicate start offsets
- Invalid segment types
- Section ordering violations
- Splitting at non-function boundaries (with Ghidra data)

Usage:
    python3 tools/splatter/lint.py                    # Basic validation
    python3 tools/splatter/lint.py --check-boundaries # Also check function boundaries via Ghidra
    python3 tools/splatter/lint.py --suggest          # Suggest split points based on function gaps

Examples:
    lint.py                     # Quick validation
    lint.py --verbose           # Show all segments
    lint.py --fix               # Auto-fix ordering issues (future)
"""

import sys
import argparse
from pathlib import Path

# Add lib to path
sys.path.insert(0, str(Path(__file__).parent))

from lib.splat_config import SplatConfig, ValidationResult, rom_to_vram


# Configuration
PROJECT_ROOT = Path(__file__).parent.parent.parent
CONFIG_FILE = PROJECT_ROOT / "config" / "splat.pal.yaml"


def error(msg: str):
    """Print error and exit."""
    print(f"❌ ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def success(msg: str):
    """Print success message."""
    print(f"✓ {msg}")


def info(msg: str):
    """Print info message."""
    print(f"  {msg}")


def print_segments(config: SplatConfig, verbose: bool = False) -> None:
    """Print segment summary."""
    segments = config.parse_subsegments()
    
    # Group by type
    by_type: dict = {}
    for seg in segments:
        t = seg.seg_type
        if t not in by_type:
            by_type[t] = []
        by_type[t].append(seg)
    
    print(f"\nSegment Summary ({len(segments)} total):")
    print("-" * 50)
    
    for seg_type, segs in sorted(by_type.items()):
        named = sum(1 for s in segs if s.name)
        print(f"  {seg_type:10s}: {len(segs):3d} segments ({named} named)")
    
    if verbose:
        print(f"\nAll Segments:")
        print("-" * 70)
        print(f"{'Index':>5s} {'ROM Offset':>12s} {'VRAM':>12s} {'Type':>8s} {'Name'}")
        print("-" * 70)
        for seg in segments:
            name = seg.name or "(unnamed)"
            print(f"{seg.index:5d} 0x{seg.start:08X} 0x{seg.vram_start:08X} {seg.seg_type:>8s} {name}")


def main():
    parser = argparse.ArgumentParser(
        description="Validate splat configuration file",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
This tool validates the splat YAML configuration for common issues.

Checks performed:
- Segments in ascending order by start offset
- No duplicate start offsets  
- Valid segment types
- Section ordering (rodata → text → data → sdata)

Examples:
    %(prog)s                    # Basic validation
    %(prog)s --verbose          # Show all segments
    %(prog)s --suggest          # Suggest split points (requires Ghidra)
"""
    )
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Show detailed segment list")
    parser.add_argument("--suggest", action="store_true",
                        help="Suggest split points based on function gaps (requires Ghidra MCP)")
    parser.add_argument("--min-gap", type=int, default=0x800,
                        help="Minimum gap size for split suggestions (default: 0x800)")
    parser.add_argument("--config", type=Path, default=CONFIG_FILE,
                        help=f"Config file to validate (default: {CONFIG_FILE})")
    
    args = parser.parse_args()
    
    print(f"\n{'='*60}")
    print(f"Validating: {args.config}")
    print(f"{'='*60}\n")
    
    # Load config
    if not args.config.exists():
        error(f"Config file not found: {args.config}")
    
    config = SplatConfig(args.config)
    try:
        config.load()
    except Exception as e:
        error(f"Failed to load config: {e}")
    
    # Print summary
    print_segments(config, args.verbose)
    
    # Validate
    print(f"\n{'='*60}")
    print("Validation Results")
    print(f"{'='*60}\n")
    
    result = config.validate()
    
    if result.errors or result.warnings:
        result.print_results()
    
    if result.is_valid:
        if result.warnings:
            print(f"\n✓ Config is valid ({len(result.warnings)} warnings)")
        else:
            success("Config is valid - no issues found")
    else:
        print(f"\n❌ Config has {len(result.errors)} error(s)")
        sys.exit(1)
    
    # Suggest split points if requested
    if args.suggest:
        print(f"\n{'='*60}")
        print("Split Point Suggestions")
        print(f"{'='*60}\n")
        
        try:
            # Try to get function list from Ghidra via MCP
            suggestions = get_split_suggestions(config, args.min_gap)
            
            if suggestions:
                print(f"Found {len(suggestions)} potential split points (gaps >= 0x{args.min_gap:X}):\n")
                for vram, gap_size in suggestions[:20]:  # Show top 20
                    rom = vram - 0x80010000 + 0x800
                    print(f"  0x{vram:08X} (ROM: 0x{rom:X}) - gap of 0x{gap_size:X} bytes")
                
                if len(suggestions) > 20:
                    print(f"\n  ... and {len(suggestions) - 20} more")
            else:
                info("No significant gaps found")
                
        except Exception as e:
            info(f"Could not get split suggestions: {e}")
            info("Make sure Ghidra MCP is running")
    
    print()


def get_split_suggestions(config: SplatConfig, min_gap: int) -> list:
    """
    Get split point suggestions by querying Ghidra for function boundaries.
    
    Returns list of (vram_addr, gap_size) tuples.
    """
    # Try to import and use Ghidra MCP
    try:
        import requests
        
        # Query Ghidra for function list
        resp = requests.get("http://localhost:8192/functions?limit=3000", timeout=5)
        if resp.status_code != 200:
            raise Exception(f"Ghidra API returned {resp.status_code}")
        
        data = resp.json()
        functions = data.get('result', [])
        
        # Extract addresses and filter to game code range
        func_addrs = []
        for f in functions:
            addr = int(f['address'], 16)
            # Only include addresses in the game code range (0x80010000 - 0x80090000)
            if 0x80010000 <= addr <= 0x80090000:
                func_addrs.append(addr)
        
        if len(func_addrs) < 2:
            return []
        
        # Sort and find gaps
        func_addrs.sort()
        gaps = []
        
        for i in range(len(func_addrs) - 1):
            gap = func_addrs[i + 1] - func_addrs[i]
            if gap >= min_gap:
                gaps.append((func_addrs[i + 1], gap))
        
        # Sort by gap size (largest first)
        gaps.sort(key=lambda x: x[1], reverse=True)
        
        return gaps
        
    except ImportError:
        raise Exception("requests library not available")
    except requests.exceptions.ConnectionError:
        raise Exception("Cannot connect to Ghidra MCP at localhost:8192")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user", file=sys.stderr)
        sys.exit(130)
