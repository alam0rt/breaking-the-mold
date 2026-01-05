#!/usr/bin/env python3
"""
blb_ls.py - Recursive listing of BLB file contents

Lists all known and unknown sections of a BLB file in an `ls -alR` style format.
Shows header sections, level segments, containers, and individual assets.

Usage:
    python3 scripts/blb_ls.py [BLB_PATH] [OPTIONS]
    
Options:
    --header      Show header sections only
    --levels      Show level segments only  
    --assets      Show asset breakdown only
    --unknown     Highlight unknown/uncovered regions
    --json        Output as JSON for further processing
    --verbose     Show raw hex dumps for unknown regions
"""

import struct
import sys
import json
from pathlib import Path
from dataclasses import dataclass, field
from typing import Optional

# Add parent to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from blb import (
    BLBHeader, BLBFile, AssetID, TileFlags, SegmentType,
    LevelMetadataEntry, MovieEntry, SectorTableEntry, CreditsEntry, StateData,
    TileHeader, LayerEntry, Tilemap, TilemapEntry, Palette, PSXColor,
    SpriteContainer, TilemapContainer, PaletteContainer, TileData,
    Sprite, SpriteFrameMetadata, AnimationEntry, SpriteHeader, RLECommand,
    AssetTOCEntry, parse_container_toc, parse_layer_entries,
    HEADER_SIZE, SECTOR_SIZE,
    LEVEL_METADATA_OFFSET, LEVEL_METADATA_ENTRY_SIZE,
    MOVIE_TABLE_OFFSET, MOVIE_ENTRY_SIZE,
    SECTOR_TABLE_OFFSET, SECTOR_TABLE_ENTRY_SIZE,
    LEVEL_COUNT_OFFSET, ASSET_COUNT_OFFSET, SECTOR_TABLE_COUNT_OFFSET,
)

# Try to get STATE_DATA_OFFSET if available
try:
    from blb import STATE_DATA_OFFSET, STATE_DATA_SIZE
except ImportError:
    STATE_DATA_OFFSET = 0xF34
    STATE_DATA_SIZE = 204


# ANSI color codes
class C:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN = "\033[96m"
    GRAY = "\033[90m"
    WHITE = "\033[97m"
    
    # Background
    BG_RED = "\033[41m"
    BG_GREEN = "\033[42m"
    BG_YELLOW = "\033[43m"


# Asset type descriptions and parsing status
ASSET_INFO = {
    # Secondary Segment - Tile Data
    0x064: ("TileHeader", "parsed", "Level dimensions, tile counts, colors (36 bytes)"),
    0x065: ("TileHeader101", "unknown", "Unknown geometry data (12 bytes)"),
    0x0C8: ("TilemapContainer", "parsed", "Layer tilemaps sub-TOC"),
    0x0C9: ("LayerEntries", "parsed", "Layer definitions (92 bytes each)"),
    0x12C: ("TilePixels", "parsed", "8bpp indexed tile pixels"),
    0x12D: ("PaletteIndices", "parsed", "Palette assignment per tile"),
    0x12E: ("TileFlags", "parsed", "Tile size/transparency flags"),
    0x12F: ("AnimatedTiles", "partial", "Animated tile lookup table"),
    0x190: ("PaletteContainer", "parsed", "256-color palette sub-TOC"),
    0x191: ("PaletteAnim", "partial", "Palette animation data"),
    
    # Primary Segment - Level Geometry
    0x258: ("Geometry", "partial", "Level geometry/RLE sprites"),
    0x259: ("Collision", "partial", "Collision/layout data"),
    0x25A: ("LevelPalette", "parsed", "Level palette (15-bit PSX)"),
    
    # Tertiary Segment - Audio & Sprites
    0x1F4: ("SpriteMetadata", "partial", "Sprite positioning data"),
    0x1F5: ("SpriteRLE", "partial", "Sprite RLE pixel data"),
    0x1F6: ("Audio502", "unknown", "Audio configuration"),
    0x1F7: ("Audio503", "unknown", "Audio configuration"),
    0x2BC: ("SPUSamples", "partial", "SPU ADPCM audio samples"),
}


@dataclass
class FileRegion:
    """Represents a region of the BLB file."""
    start: int
    size: int
    name: str
    type: str  # 'header', 'level', 'container', 'asset', 'gap'
    parent: Optional[str] = None
    children: list = field(default_factory=list)
    status: str = 'known'  # 'known', 'partial', 'unknown'
    details: dict = field(default_factory=dict)
    
    @property
    def end(self) -> int:
        return self.start + self.size
    
    def __repr__(self):
        return f"FileRegion({self.name}, {self.start:08X}-{self.end:08X}, {self.size} bytes)"


def format_size(size: int) -> str:
    """Format size in human-readable form."""
    if size < 1024:
        return f"{size} B"
    elif size < 1024 * 1024:
        return f"{size/1024:.1f} KB"
    else:
        return f"{size/1024/1024:.2f} MB"


def format_offset(offset: int) -> str:
    """Format offset as hex."""
    return f"0x{offset:08X}"


def analyze_header(header: BLBHeader, raw_header: bytes) -> list[FileRegion]:
    """Analyze and list all header sections."""
    regions = []
    
    # Level metadata table
    level_table_size = header.level_count * LEVEL_METADATA_ENTRY_SIZE
    regions.append(FileRegion(
        start=LEVEL_METADATA_OFFSET,
        size=level_table_size,
        name="LevelTable",
        type="header",
        status="known",
        details={
            "entry_count": header.level_count,
            "entry_size": LEVEL_METADATA_ENTRY_SIZE,
            "levels": [e.level_id for e in header.level_entries],
        }
    ))
    
    # Individual level entries
    for entry in header.level_entries:
        regions.append(FileRegion(
            start=LEVEL_METADATA_OFFSET + entry.index * LEVEL_METADATA_ENTRY_SIZE,
            size=LEVEL_METADATA_ENTRY_SIZE,
            name=f"Level[{entry.index}]/{entry.level_id}",
            type="header",
            parent="LevelTable",
            status="known",
            details={
                "level_id": entry.level_id,
                "name": entry.name,
                "primary_sector": entry.sector_offset,
                "primary_count": entry.sector_count,
                "secondary_sector": entry.secondary_offset,
                "secondary_count": entry.secondary_count,
                "tert_block_count": entry.tert_block_count,
            }
        ))
    
    # Movie table
    movie_table_size = header.asset_count * MOVIE_ENTRY_SIZE
    regions.append(FileRegion(
        start=MOVIE_TABLE_OFFSET,
        size=movie_table_size,
        name="MovieTable",
        type="header",
        status="known",
        details={
            "entry_count": header.asset_count,
            "entry_size": MOVIE_ENTRY_SIZE,
            "movies": [e.movie_id for e in header.movie_entries],
        }
    ))
    
    # Sector table
    sector_table_size = header.sector_table_count * SECTOR_TABLE_ENTRY_SIZE
    regions.append(FileRegion(
        start=SECTOR_TABLE_OFFSET,
        size=sector_table_size,
        name="SectorTable",
        type="header",
        status="known",
        details={
            "entry_count": header.sector_table_count,
            "entry_size": SECTOR_TABLE_ENTRY_SIZE,
        }
    ))
    
    # Unknown region: 0xED0-0xF10 (64 bytes, u32 array)
    regions.append(FileRegion(
        start=0xED0,
        size=64,
        name="UnknownU32Array",
        type="header",
        status="unknown",
        details={"description": "16 u32 values, purpose TBD"}
    ))
    
    # Credits table: 0xF10-0xF31
    regions.append(FileRegion(
        start=0xF10,
        size=0x21,
        name="CreditsTable",
        type="header",
        status="partial",
        details={"description": "Credits sequence entries, 12 bytes each"}
    ))
    
    # Count bytes: 0xF31-0xF34
    regions.append(FileRegion(
        start=0xF31,
        size=3,
        name="Counts",
        type="header",
        status="known",
        details={
            "level_count": header.level_count,
            "asset_count": header.asset_count,
            "sector_table_count": header.sector_table_count,
        }
    ))
    
    # State/playback data: 0xF34-0x1000
    regions.append(FileRegion(
        start=STATE_DATA_OFFSET,
        size=STATE_DATA_SIZE,
        name="PlaybackSequence",
        type="header",
        status="known",
        details={"description": "Mode/index arrays for playback sequence"}
    ))
    
    return regions


def analyze_container(data: bytes, container_name: str, base_offset: int) -> list[FileRegion]:
    """Analyze a container's TOC and assets."""
    regions = []
    
    if len(data) < 4:
        return regions
    
    try:
        entries = parse_container_toc(data)
    except Exception as e:
        regions.append(FileRegion(
            start=base_offset,
            size=len(data),
            name=f"{container_name}/ParseError",
            type="container",
            status="unknown",
            details={"error": str(e)}
        ))
        return regions
    
    # Container TOC
    toc_size = 4 + len(entries) * 12
    regions.append(FileRegion(
        start=base_offset,
        size=toc_size,
        name=f"{container_name}/TOC",
        type="container",
        status="known",
        details={"entry_count": len(entries)}
    ))
    
    # Each asset in the container
    for entry in entries:
        asset_id = entry.asset_id
        info = ASSET_INFO.get(asset_id, (f"Asset{asset_id}", "unknown", "Unknown asset type"))
        
        asset_region = FileRegion(
            start=base_offset + entry.offset,
            size=entry.size,
            name=f"{container_name}/{info[0]}",
            type="asset",
            parent=container_name,
            status=info[1],
            details={
                "asset_id": asset_id,
                "asset_id_hex": f"0x{asset_id:03X}",
                "description": info[2],
            }
        )
        
        # Parse sub-containers recursively
        if asset_id == 0x0C8 and entry.data:  # TilemapContainer
            try:
                container = TilemapContainer.from_bytes(entry.data)
                asset_region.details["tilemap_count"] = len(container.tilemaps)
                asset_region.children = [
                    f"Tilemap[{i}]: {len(tm)} entries"
                    for i, tm in enumerate(container.tilemaps)
                ]
            except:
                pass
        
        elif asset_id == 0x0C9 and entry.data:  # LayerEntries
            try:
                layers = parse_layer_entries(entry.data)
                asset_region.details["layer_count"] = len(layers)
                asset_region.children = [
                    f"Layer[{i}]: {l.width}x{l.height} @ ({l.x_offset},{l.y_offset})"
                    for i, l in enumerate(layers)
                ]
            except:
                pass
        
        elif asset_id == 0x190 and entry.data:  # PaletteContainer
            try:
                container = PaletteContainer.from_bytes(entry.data)
                asset_region.details["palette_count"] = len(container.palettes)
            except:
                pass
        
        elif asset_id == 0x064 and entry.data:  # TileHeader
            try:
                th = TileHeader.from_bytes(entry.data)
                asset_region.details.update({
                    "level_size": f"{th.level_width}x{th.level_height} tiles",
                    "tile_count_16x16": th.count_16x16,
                    "tile_count_8x8": th.count_8x8_a + th.count_8x8_b,
                    "bg_color": f"RGB({th.bg_r},{th.bg_g},{th.bg_b})",
                })
            except:
                pass
        
        regions.append(asset_region)
    
    return regions


def analyze_level_segments(blb: BLBFile, level: LevelMetadataEntry) -> list[FileRegion]:
    """Analyze all segments (primary, secondary, tertiary) for a level."""
    regions = []
    f = blb._file
    
    level_name = f"Level[{level.index}]/{level.level_id}"
    
    # Primary segment
    if level.sector_count > 0:
        primary_offset = level.sector_offset * SECTOR_SIZE
        primary_size = level.sector_count * SECTOR_SIZE
        
        f.seek(primary_offset)
        primary_data = f.read(primary_size)
        
        primary_region = FileRegion(
            start=primary_offset,
            size=primary_size,
            name=f"{level_name}/Primary",
            type="level",
            status="known",
            details={"segment": "primary", "sectors": level.sector_count}
        )
        regions.append(primary_region)
        
        # Parse primary container
        sub_regions = analyze_container(primary_data, f"{level_name}/Primary", primary_offset)
        regions.extend(sub_regions)
    
    # Secondary segment (base)
    if level.secondary_count > 0:
        secondary_offset = level.secondary_offset * SECTOR_SIZE
        secondary_size = level.secondary_count * SECTOR_SIZE
        
        f.seek(secondary_offset)
        secondary_data = f.read(secondary_size)
        
        secondary_region = FileRegion(
            start=secondary_offset,
            size=secondary_size,
            name=f"{level_name}/Secondary",
            type="level",
            status="known",
            details={"segment": "secondary", "sectors": level.secondary_count}
        )
        regions.append(secondary_region)
        
        # Parse secondary container
        sub_regions = analyze_container(secondary_data, f"{level_name}/Secondary", secondary_offset)
        regions.extend(sub_regions)
    
    # Secondary sub-blocks (stages 1-5)
    for i in range(5):
        if i < len(level.sec_sub_offsets) and i < len(level.sec_sub_counts):
            offset = level.sec_sub_offsets[i]
            count = level.sec_sub_counts[i]
            if count > 0:
                sec_sub_offset = offset * SECTOR_SIZE
                sec_sub_size = count * SECTOR_SIZE
                
                f.seek(sec_sub_offset)
                sec_sub_data = f.read(sec_sub_size)
                
                regions.append(FileRegion(
                    start=sec_sub_offset,
                    size=sec_sub_size,
                    name=f"{level_name}/SecSub[{i}]",
                    type="level",
                    status="known",
                    details={"segment": "secondary_sub", "stage": i+1, "sectors": count}
                ))
                
                sub_regions = analyze_container(sec_sub_data, f"{level_name}/SecSub[{i}]", sec_sub_offset)
                regions.extend(sub_regions)
    
    # Tertiary sub-blocks
    for i in range(level.tert_block_count):
        if i < len(level.tert_sub_offsets) and i < len(level.tert_sub_counts):
            offset = level.tert_sub_offsets[i]
            count = level.tert_sub_counts[i]
            if count > 0:
                tert_offset = offset * SECTOR_SIZE
                tert_size = count * SECTOR_SIZE
                
                f.seek(tert_offset)
                tert_data = f.read(tert_size)
                
                regions.append(FileRegion(
                    start=tert_offset,
                    size=tert_size,
                    name=f"{level_name}/Tertiary[{i}]",
                    type="level",
                    status="known",
                    details={"segment": "tertiary", "block": i, "sectors": count}
                ))
                
                sub_regions = analyze_container(tert_data, f"{level_name}/Tertiary[{i}]", tert_offset)
                regions.extend(sub_regions)
    
    return regions


def find_gaps(regions: list[FileRegion], file_size: int) -> list[FileRegion]:
    """Find uncovered regions in the file."""
    # Sort by start offset
    sorted_regions = sorted(regions, key=lambda r: r.start)
    
    gaps = []
    prev_end = 0
    
    for region in sorted_regions:
        if region.start > prev_end:
            gaps.append(FileRegion(
                start=prev_end,
                size=region.start - prev_end,
                name="<uncovered>",
                type="gap",
                status="unknown",
            ))
        prev_end = max(prev_end, region.end)
    
    # Gap at end of file
    if prev_end < file_size:
        gaps.append(FileRegion(
            start=prev_end,
            size=file_size - prev_end,
            name="<uncovered>",
            type="gap",
            status="unknown",
        ))
    
    return gaps


def print_tree(regions: list[FileRegion], indent: int = 0, show_unknown: bool = True):
    """Print regions in tree format."""
    prefix = "  " * indent
    
    for region in regions:
        # Color based on status
        if region.status == "known":
            color = C.GREEN
        elif region.status == "partial":
            color = C.YELLOW
        else:
            color = C.RED
        
        # Type icon
        if region.type == "header":
            icon = "📄"
        elif region.type == "level":
            icon = "📦"
        elif region.type == "container":
            icon = "📁"
        elif region.type == "asset":
            icon = "  "
        else:
            icon = "❓"
        
        # Format line
        offset_str = f"{region.start:08X}"
        size_str = format_size(region.size).rjust(10)
        
        if region.type == "gap" and not show_unknown:
            continue
        
        print(f"{prefix}{icon} {color}{offset_str}{C.RESET}  {size_str}  {region.name}")
        
        # Print details
        if region.details:
            for key, value in region.details.items():
                if key not in ('description',):
                    print(f"{prefix}   {C.DIM}├─ {key}: {value}{C.RESET}")
        
        # Print children
        if region.children:
            for child in region.children[:5]:  # Limit to first 5
                print(f"{prefix}   {C.DIM}├─ {child}{C.RESET}")
            if len(region.children) > 5:
                print(f"{prefix}   {C.DIM}└─ ... and {len(region.children)-5} more{C.RESET}")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="List BLB file contents recursively")
    parser.add_argument("blb_path", nargs="?", help="Path to BLB file")
    parser.add_argument("--header", action="store_true", help="Show header sections only")
    parser.add_argument("--levels", action="store_true", help="Show level segments only")
    parser.add_argument("--level", type=int, help="Show specific level only")
    parser.add_argument("--assets", action="store_true", help="Show asset summary only")
    parser.add_argument("--playback", action="store_true", help="Show full playback sequence")
    parser.add_argument("--unknown", action="store_true", help="Show unknown/gap regions")
    parser.add_argument("--json", action="store_true", help="Output as JSON")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    
    args = parser.parse_args()
    
    # Default BLB path
    blb_path = Path(args.blb_path) if args.blb_path else Path(__file__).parent.parent / "disks" / "blb" / "GAME.BLB"
    
    if not blb_path.exists():
        print(f"Error: {blb_path} not found", file=sys.stderr)
        sys.exit(1)
    
    file_size = blb_path.stat().st_size
    
    print(f"{C.BOLD}BLB File: {blb_path}{C.RESET}")
    print(f"Size: {format_size(file_size)} ({file_size:,} bytes)")
    print()
    
    all_regions = []
    
    with BLBFile(blb_path) as blb:
        # Read raw header
        blb._file.seek(0)
        raw_header = blb._file.read(HEADER_SIZE)
        
        # ===== HEADER ANALYSIS =====
        if not args.levels and not args.assets:
            print(f"{C.BOLD}{'='*70}{C.RESET}")
            print(f"{C.BOLD}HEADER (0x0000 - 0x1000){C.RESET}")
            print(f"{C.BOLD}{'='*70}{C.RESET}")
            
            header_regions = analyze_header(blb.header, raw_header)
            all_regions.extend(header_regions)
            
            # Print header sections
            for region in header_regions:
                if region.parent is None:  # Top-level only
                    print_tree([region])
            
            # Show movie table details
            print(f"\n{C.CYAN}Movie Table ({len(blb.header.movie_entries)} entries):{C.RESET}")
            for i, movie in enumerate(blb.header.movie_entries):
                print(f"  [{i:2d}] {movie.movie_id:5s} {movie.sector_count:5d} sectors  {movie.filename}")
            
            # Show sector table details (loading screens)
            print(f"\n{C.CYAN}Sector Table ({blb.header.sector_table_count} entries):{C.RESET}")
            for entry in blb.header.sector_entries:
                flags_str = f"lvl={entry.level_index:2d}" if entry.entry_flags == 0 else f"flg=0x{entry.entry_flags:02X}"
                print(f"  {entry.code:5s} {flags_str}  sector {entry.sector_offset:5d} × {entry.sector_count:2d}  {entry.short_name}")
            
            # Show credits table
            if hasattr(blb.header, 'credits_entries') and blb.header.credits_entries:
                print(f"\n{C.CYAN}Credits Table ({len(blb.header.credits_entries)} entries):{C.RESET}")
                for i, entry in enumerate(blb.header.credits_entries):
                    print(f"  [{i}] code={entry.code!r:8s} param_a={entry.param_a:4d} param_b={entry.param_b:4d}")
            
            # Show playback sequence
            if blb.header.state_data:
                max_entries = 90 if args.playback else 30
                print(f"\n{C.CYAN}Playback Sequence ({max_entries} entries{', use --playback for all' if not args.playback else ''}):{C.RESET}")
                print(f"  {'Pos':>4} {'Mode':>5} {'Index':>6} Interpretation")
                print(f"  {'-'*55}")
                for pos in range(max_entries):
                    mode, idx = blb.header.state_data.get_sequence_item(pos)
                    if mode == 0 and idx == 0:
                        continue  # Skip empty entries
                    
                    # Interpret the mode
                    if mode == 1:
                        # Movie mode
                        movie = blb.header.movie_entries[idx] if idx < len(blb.header.movie_entries) else None
                        desc = f"Movie: {movie.movie_id}" if movie else f"Movie[{idx}]"
                    elif mode == 2:
                        desc = f"Credits[{idx}]"
                    elif mode == 3:
                        level = blb.header.level_entries[idx] if idx < len(blb.header.level_entries) else None
                        desc = f"Level: {level.level_id} ({level.name})" if level else f"Level[{idx}]"
                    elif mode == 4:
                        desc = f"Special sector loading"
                    elif mode == 5:
                        desc = f"Initial sector loading"
                    elif mode == 6:
                        desc = f"End/Special"
                    else:
                        desc = f"Unknown mode"
                    
                    print(f"  {pos:4d} {mode:5d} {idx:6d} {desc}")
            print()
        
        # ===== LEVEL ANALYSIS =====
        if not args.header and not args.assets:
            print(f"{C.BOLD}{'='*70}{C.RESET}")
            print(f"{C.BOLD}LEVELS ({blb.header.level_count} levels){C.RESET}")
            print(f"{C.BOLD}{'='*70}{C.RESET}")
            
            for level in blb.header.level_entries:
                if args.level is not None and level.index != args.level:
                    continue
                
                print(f"\n{C.CYAN}Level {level.index}: {level.level_id} - {level.name}{C.RESET}")
                print(f"  Primary: sector {level.sector_offset}, {level.sector_count} sectors")
                print(f"  Secondary: sector {level.secondary_offset}, {level.secondary_count} sectors")
                print(f"  Tertiary blocks: {level.tert_block_count}")
                
                level_regions = analyze_level_segments(blb, level)
                all_regions.extend(level_regions)
                
                # Group by segment type
                if args.verbose:
                    for region in level_regions:
                        if region.type in ('container', 'asset'):
                            print_tree([region], indent=1)
                else:
                    # Just show asset summary
                    assets = [r for r in level_regions if r.type == 'asset']
                    for seg_type in ['Primary', 'Secondary', 'SecSub', 'Tertiary']:
                        seg_assets = [a for a in assets if seg_type in a.name]
                        if seg_assets:
                            print(f"  {seg_type} assets:")
                            for asset in seg_assets[:10]:
                                status_color = C.GREEN if asset.status == "known" else C.YELLOW if asset.status == "partial" else C.RED
                                print(f"    {status_color}●{C.RESET} {asset.details.get('asset_id_hex', '???')} {asset.name.split('/')[-1]}: {format_size(asset.size)}")
                            if len(seg_assets) > 10:
                                print(f"    ... and {len(seg_assets)-10} more")
        
        # ===== ASSET SUMMARY =====
        if args.assets or (not args.header and not args.levels):
            print(f"\n{C.BOLD}{'='*70}{C.RESET}")
            print(f"{C.BOLD}ASSET SUMMARY{C.RESET}")
            print(f"{C.BOLD}{'='*70}{C.RESET}")
            
            # Aggregate by asset ID
            asset_summary = {}
            for region in all_regions:
                if region.type == 'asset':
                    aid = region.details.get('asset_id', 0)
                    if aid not in asset_summary:
                        asset_summary[aid] = {"count": 0, "total_size": 0, "status": region.status}
                    asset_summary[aid]["count"] += 1
                    asset_summary[aid]["total_size"] += region.size
            
            print(f"\n{'Asset ID':<12} {'Name':<20} {'Count':>6} {'Total Size':>12} {'Status':<10}")
            print("-" * 70)
            
            for aid in sorted(asset_summary.keys()):
                info = ASSET_INFO.get(aid, (f"Unknown", "unknown", ""))
                summary = asset_summary[aid]
                status_color = C.GREEN if summary['status'] == "known" else C.YELLOW if summary['status'] == "partial" else C.RED
                print(f"0x{aid:03X}        {info[0]:<20} {summary['count']:>6} {format_size(summary['total_size']):>12} {status_color}{summary['status']:<10}{C.RESET}")
            
            print()
        
        # ===== COVERAGE ANALYSIS =====
        if args.unknown:
            gaps = find_gaps(all_regions, file_size)
            
            print(f"\n{C.BOLD}{'='*70}{C.RESET}")
            print(f"{C.RED}UNCOVERED REGIONS{C.RESET}")
            print(f"{C.BOLD}{'='*70}{C.RESET}")
            
            total_gap = sum(g.size for g in gaps)
            print(f"Total uncovered: {format_size(total_gap)} ({100*total_gap/file_size:.1f}%)")
            print()
            
            for gap in gaps:
                print(f"  {gap.start:08X} - {gap.end:08X}: {format_size(gap.size)}")
        
        # ===== FINAL SUMMARY =====
        print(f"\n{C.BOLD}{'='*70}{C.RESET}")
        print(f"{C.BOLD}COVERAGE SUMMARY{C.RESET}")
        print(f"{C.BOLD}{'='*70}{C.RESET}")
        
        # Calculate coverage
        covered_bytes = sum(r.size for r in all_regions if r.type != 'gap')
        known_bytes = sum(r.size for r in all_regions if r.status == 'known')
        partial_bytes = sum(r.size for r in all_regions if r.status == 'partial')
        unknown_bytes = sum(r.size for r in all_regions if r.status == 'unknown')
        
        gaps = find_gaps(all_regions, file_size)
        gap_bytes = sum(g.size for g in gaps)
        
        print(f"  {C.GREEN}Known:     {format_size(known_bytes):>12} ({100*known_bytes/file_size:.1f}%){C.RESET}")
        print(f"  {C.YELLOW}Partial:   {format_size(partial_bytes):>12} ({100*partial_bytes/file_size:.1f}%){C.RESET}")
        print(f"  {C.RED}Unknown:   {format_size(unknown_bytes):>12} ({100*unknown_bytes/file_size:.1f}%){C.RESET}")
        print(f"  {C.GRAY}Uncovered: {format_size(gap_bytes):>12} ({100*gap_bytes/file_size:.1f}%){C.RESET}")
        print()
        
        # JSON output
        if args.json:
            output = {
                "file": str(blb_path),
                "file_size": file_size,
                "regions": [
                    {
                        "start": r.start,
                        "end": r.end,
                        "size": r.size,
                        "name": r.name,
                        "type": r.type,
                        "status": r.status,
                        "details": r.details,
                    }
                    for r in all_regions
                ],
                "coverage": {
                    "known_bytes": known_bytes,
                    "partial_bytes": partial_bytes,
                    "unknown_bytes": unknown_bytes,
                    "gap_bytes": gap_bytes,
                }
            }
            print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()
