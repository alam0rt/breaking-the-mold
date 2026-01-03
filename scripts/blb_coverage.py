#!/usr/bin/env python3
"""
BLB Header Coverage Analysis

Analyzes how much of the BLB header (first 4096 bytes) is read, parsed,
and understood by the blb.py library.

Reports:
- Bytes covered by known structures
- Bytes that are unprocessed/unknown
- Coverage percentage
"""

from pathlib import Path
import sys

# ANSI color codes
class Colors:
    RESET = "\033[0m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN = "\033[96m"
    GRAY = "\033[90m"
    BOLD = "\033[1m"
    DIM = "\033[2m"


from blb import (
    BLBHeader,
    BLBFile,
    HEADER_SIZE,
    SECTOR_SIZE,
    SECTOR_TABLE_OFFSET,
    SECTOR_TABLE_ENTRY_SIZE,
    LEVEL_METADATA_OFFSET,
    LEVEL_METADATA_ENTRY_SIZE,
    LEVEL_NAME_OFFSET,
    LEVEL_COUNT_OFFSET,
    ASSET_COUNT_OFFSET,
    SECTOR_TABLE_COUNT_OFFSET,
    MOVIE_TABLE_OFFSET,
    MOVIE_ENTRY_SIZE,
    LevelMetadataEntry,
    MovieEntry,
    SectorTableEntry,
    is_unknown_field,
)


def analyze_file_coverage(blb_path: Path) -> dict:
    """
    Analyze how much of the entire BLB file is covered by extractable data.
    
    Returns a dict with file coverage statistics.
    """
    import os
    
    file_size = os.path.getsize(blb_path)
    header = BLBHeader.from_file(blb_path)
    
    # Track covered byte ranges (start, end, description)
    covered_ranges = []
    
    # Header is always covered
    covered_ranges.append((0, HEADER_SIZE, "header"))
    
    # Level primary, secondary, tertiary data
    for level in header.level_entries:
        if level.sector_count > 0:
            start = level.sector_offset * SECTOR_SIZE
            end = start + (level.sector_count * SECTOR_SIZE)
            covered_ranges.append((start, end, f"level_{level.index}_primary"))
        
        if level.secondary_count > 0:
            start = level.secondary_offset * SECTOR_SIZE
            end = start + (level.secondary_count * SECTOR_SIZE)
            covered_ranges.append((start, end, f"level_{level.index}_secondary"))
        
        # Tertiary uses multiple sub-blocks
        if level.tert_sub_counts:
            for i, (offset, count) in enumerate(zip(level.tert_sub_offsets, level.tert_sub_counts)):
                if count > 0:
                    start = offset * SECTOR_SIZE
                    end = start + (count * SECTOR_SIZE)
                    covered_ranges.append((start, end, f"level_{level.index}_tertiary_{i}"))
    
    # Sector table entries
    for entry in header.sector_entries:
        if entry.sector_count > 0:
            start = entry.sector_offset * SECTOR_SIZE
            end = start + (entry.sector_count * SECTOR_SIZE)
            covered_ranges.append((start, end, f"sector_{entry.index}_{entry.code}"))
    
    # Merge overlapping ranges and calculate total coverage
    # Sort by start position
    covered_ranges.sort(key=lambda x: x[0])
    
    # Merge overlapping ranges
    merged = []
    for start, end, desc in covered_ranges:
        if merged and start <= merged[-1][1]:
            # Overlapping - extend previous range
            merged[-1] = (merged[-1][0], max(merged[-1][1], end), merged[-1][2] + "+" + desc)
        else:
            merged.append((start, end, desc))
    
    # Calculate total covered bytes
    total_covered = sum(end - start for start, end, _ in merged)
    
    # Find gaps
    gaps = []
    prev_end = 0
    for start, end, desc in merged:
        if start > prev_end:
            gaps.append((prev_end, start, start - prev_end))
        prev_end = end
    
    # Check for gap at end of file
    if prev_end < file_size:
        gaps.append((prev_end, file_size, file_size - prev_end))
    
    return {
        "file_size": file_size,
        "covered_bytes": total_covered,
        "uncovered_bytes": file_size - total_covered,
        "coverage_percent": (total_covered / file_size) * 100 if file_size > 0 else 0,
        "covered_ranges": merged,
        "gaps": gaps,
    }


def analyze_coverage(blb_path: Path) -> dict:
    """
    Analyze which bytes of the BLB header are read/processed by the library.
    
    Returns a dict with coverage statistics.
    """
    header = BLBHeader.from_file(blb_path)
    
    # Track which bytes are "covered" (read/interpreted by the library)
    covered = [False] * HEADER_SIZE
    
    # Track coverage by category
    coverage_map = {}
    
    # Track which categories are "unknown" (parsed but purpose not understood)
    unknown_categories = set()
    
    def mark_range(start: int, end: int, category: str, is_unknown: bool = False):
        """Mark a byte range as covered."""
        if category not in coverage_map:
            coverage_map[category] = []
        coverage_map[category].append((start, end))
        for i in range(start, min(end, HEADER_SIZE)):
            covered[i] = True
        if is_unknown:
            unknown_categories.add(category)
    
    # 1. Level metadata table (0x70 bytes per entry, starting at 0x000)
    for i in range(header.level_count):
        entry_start = LEVEL_METADATA_OFFSET + (i * LEVEL_METADATA_ENTRY_SIZE)
        
        # Parse all level metadata fields (matches LevelMetadataEntry structure)
        # Check is_unknown_field for each field
        mark_range(entry_start + 0x00, entry_start + 0x02, "level_sector_offset")
        mark_range(entry_start + 0x02, entry_start + 0x04, "level_sector_count")
        mark_range(entry_start + 0x04, entry_start + 0x0C, "level_static_data", 
                   is_unknown_field(LevelMetadataEntry, "static_data"))
        mark_range(entry_start + 0x0C, entry_start + 0x0D, "level_index")
        mark_range(entry_start + 0x0D, entry_start + 0x0E, "level_flag",
                   is_unknown_field(LevelMetadataEntry, "flag"))
        mark_range(entry_start + 0x0E, entry_start + 0x1C, "level_unknown_0e",
                   is_unknown_field(LevelMetadataEntry, "unknown_0e"))
        mark_range(entry_start + 0x1C, entry_start + 0x1E, "level_unknown_1c",
                   is_unknown_field(LevelMetadataEntry, "unknown_1c"))
        mark_range(entry_start + 0x1E, entry_start + 0x20, "level_secondary_offset")
        mark_range(entry_start + 0x20, entry_start + 0x22, "level_unknown_20",
                   is_unknown_field(LevelMetadataEntry, "unknown_20"))
        mark_range(entry_start + 0x22, entry_start + 0x2C, "level_dynamic_data_1",
                   is_unknown_field(LevelMetadataEntry, "dynamic_data_1"))
        mark_range(entry_start + 0x2C, entry_start + 0x2E, "level_secondary_count")
        mark_range(entry_start + 0x2E, entry_start + 0x3E, "level_unknown_2e",
                   is_unknown_field(LevelMetadataEntry, "unknown_2e"))
        mark_range(entry_start + 0x3E, entry_start + 0x40, "level_unknown_3e",
                   is_unknown_field(LevelMetadataEntry, "unknown_3e"))
        mark_range(entry_start + 0x40, entry_start + 0x42, "level_tertiary_offset")
        mark_range(entry_start + 0x42, entry_start + 0x44, "level_unknown_42",
                   is_unknown_field(LevelMetadataEntry, "unknown_42"))
        mark_range(entry_start + 0x44, entry_start + 0x4E, "level_dynamic_data_2",
                   is_unknown_field(LevelMetadataEntry, "dynamic_data_2"))
        mark_range(entry_start + 0x4E, entry_start + 0x50, "level_tertiary_count")
        mark_range(entry_start + 0x50, entry_start + 0x56, "level_unknown_50",
                   is_unknown_field(LevelMetadataEntry, "unknown_50"))
        mark_range(entry_start + 0x56, entry_start + 0x5B, "level_id")
        mark_range(entry_start + 0x5B, entry_start + 0x70, "level_name")
    
    # 2. Movie table (0x1C bytes per entry, starting at 0xB60)
    for i in range(len(header.movie_entries)):
        entry_start = MOVIE_TABLE_OFFSET + (i * MOVIE_ENTRY_SIZE)
        
        # Parse all movie fields (matches MovieEntry structure)
        mark_range(entry_start + 0x00, entry_start + 0x02, "movie_unknown_00",
                   is_unknown_field(MovieEntry, "unknown_00"))
        mark_range(entry_start + 0x02, entry_start + 0x04, "movie_sector_count")
        mark_range(entry_start + 0x04, entry_start + 0x09, "movie_id")
        mark_range(entry_start + 0x09, entry_start + 0x0C, "movie_short_name")
        mark_range(entry_start + 0x0C, entry_start + 0x1C, "movie_filename")
    
    # 3. Sector offset table (0x10 bytes per entry, starting at 0xCD0)
    for i in range(header.sector_table_count):
        entry_start = SECTOR_TABLE_OFFSET + (i * SECTOR_TABLE_ENTRY_SIZE)
        
        # Mark individual fields we parse (matches SectorTableEntry structure)
        mark_range(entry_start + 0x00, entry_start + 0x01, "sector_level_index")
        mark_range(entry_start + 0x01, entry_start + 0x02, "sector_entry_flags")
        mark_range(entry_start + 0x02, entry_start + 0x03, "sector_entry_unknown",
                   is_unknown_field(SectorTableEntry, "unknown_byte"))
        mark_range(entry_start + 0x03, entry_start + 0x08, "sector_entry_code")
        mark_range(entry_start + 0x08, entry_start + 0x0C, "sector_entry_short")
        mark_range(entry_start + 0x0C, entry_start + 0x0E, "sector_offset")
        mark_range(entry_start + 0x0E, entry_start + 0x10, "sector_count")
    
    # 4. Header count fields
    mark_range(LEVEL_COUNT_OFFSET, LEVEL_COUNT_OFFSET + 1, "level_count")
    mark_range(ASSET_COUNT_OFFSET, ASSET_COUNT_OFFSET + 1, "asset_count")
    mark_range(SECTOR_TABLE_COUNT_OFFSET, SECTOR_TABLE_COUNT_OFFSET + 1, "sector_table_count")
    
    # 5. Unknown regions (stored as raw bytes in header)
    # Gap between level metadata and movie table
    level_meta_end = header.level_count * LEVEL_METADATA_ENTRY_SIZE
    if level_meta_end < MOVIE_TABLE_OFFSET:
        mark_range(level_meta_end, MOVIE_TABLE_OFFSET, "gap_level_to_movie", is_unknown=True)
    
    # Gap between movie table and sector table
    movie_table_end = MOVIE_TABLE_OFFSET + (len(header.movie_entries) * MOVIE_ENTRY_SIZE)
    if movie_table_end < SECTOR_TABLE_OFFSET:
        mark_range(movie_table_end, SECTOR_TABLE_OFFSET, "gap_movie_to_sector", is_unknown=True)
    
    # Unknown region at 0xED0-0xF10 (64 bytes) - u32 array
    mark_range(0xED0, 0xF10, "unknown_ed0", is_unknown=True)
    
    # State/config data region at 0xF34-0x1000 (204 bytes)
    # Some fields are now understood:
    mark_range(0xF34, 0xF36, "state_unknown_f34_f35", is_unknown=True)
    mark_range(0xF36, 0xF37, "state_game_mode")  # Known: game mode (1=movie, 2=credits, 4-5=level)
    mark_range(0xF37, 0xF38, "state_secondary_flag")  # Known: secondary state flag
    mark_range(0xF38, 0xF91, "state_unknown_f38_f90", is_unknown=True)
    mark_range(0xF91, 0xF92, "state_unknown_f91", is_unknown=True)  # Credits-related?
    mark_range(0xF92, 0xF93, "state_asset_index")  # Known: current asset/movie index
    mark_range(0xF93, 0x1000, "state_unknown_f93_end", is_unknown=True)
    
    # Calculate statistics
    total_covered = sum(covered)
    total_bytes = HEADER_SIZE
    
    # Find uncovered regions
    uncovered_regions = []
    in_uncovered = False
    uncovered_start = 0
    
    for i, is_covered in enumerate(covered):
        if not is_covered and not in_uncovered:
            in_uncovered = True
            uncovered_start = i
        elif is_covered and in_uncovered:
            in_uncovered = False
            uncovered_regions.append((uncovered_start, i))
    
    if in_uncovered:
        uncovered_regions.append((uncovered_start, HEADER_SIZE))
    
    # Summarize coverage by category
    category_summary = {}
    for category, ranges in coverage_map.items():
        total = sum(end - start for start, end in ranges)
        category_summary[category] = {
            "bytes": total,
            "ranges": len(ranges),
            "unknown": category in unknown_categories,
        }
    
    # Calculate known vs unknown bytes
    known_bytes = sum(
        info["bytes"] for cat, info in category_summary.items() 
        if not info["unknown"]
    )
    unknown_bytes_parsed = sum(
        info["bytes"] for cat, info in category_summary.items() 
        if info["unknown"]
    )
    
    # Read raw header data for hex dump
    with open(blb_path, "rb") as f:
        raw_header = f.read(HEADER_SIZE)
    
    return {
        "file": str(blb_path),
        "header_size": total_bytes,
        "covered_bytes": total_covered,
        "uncovered_bytes": total_bytes - total_covered,
        "coverage_percent": (total_covered / total_bytes) * 100,
        "known_bytes": known_bytes,
        "unknown_bytes": unknown_bytes_parsed,
        "known_percent": (known_bytes / total_bytes) * 100,
        "level_count": header.level_count,
        "asset_count": header.asset_count,
        "sector_table_count": header.sector_table_count,
        "categories": category_summary,
        "unknown_categories": unknown_categories,
        "uncovered_regions": uncovered_regions,
        "covered_bitmap": covered,
        "raw_header": raw_header,
    }


def print_report(analysis: dict) -> None:
    """Print a formatted coverage report."""
    print("=" * 70)
    print("BLB Header Coverage Report")
    print("=" * 70)
    print(f"File: {analysis['file']}")
    print()
    
    print("Header Counts:")
    print(f"  Level count:        {analysis['level_count']}")
    print(f"  Asset count:        {analysis['asset_count']}")
    print(f"  Sector table count: {analysis['sector_table_count']}")
    print()
    
    print("Coverage Summary:")
    print(f"  Header size:    {analysis['header_size']:,} bytes (0x{analysis['header_size']:X})")
    print(f"  Covered:        {analysis['covered_bytes']:,} bytes ({analysis['coverage_percent']:.1f}%)")
    print(f"  Uncovered:      {analysis['uncovered_bytes']:,} bytes ({100 - analysis['coverage_percent']:.1f}%)")
    print()
    
    print(f"{Colors.BOLD}Understanding Summary:{Colors.RESET}")
    print(f"  {Colors.GREEN}Known:    {analysis['known_bytes']:,} bytes ({analysis['known_percent']:.1f}%){Colors.RESET}")
    print(f"  {Colors.YELLOW}Unknown:  {analysis['unknown_bytes']:,} bytes ({100 - analysis['known_percent']:.1f}%){Colors.RESET}")
    print()
    
    print("Coverage by Category:")
    print(f"  {'Category':<34} {'Bytes':>8} {'Ranges':>6} {'Status':>8}")
    print("  " + "-" * 58)
    
    # Sort categories by bytes covered (descending)
    sorted_cats = sorted(
        analysis["categories"].items(),
        key=lambda x: x[1]["bytes"],
        reverse=True
    )
    
    for category, info in sorted_cats:
        if info["unknown"]:
            status = f"{Colors.YELLOW}unknown{Colors.RESET}"
        else:
            status = f"{Colors.GREEN}known{Colors.RESET}"
        print(f"  {category:<34} {info['bytes']:>8} {info['ranges']:>6} {status}")
    
    print()
    print("Uncovered Regions:")
    if not analysis["uncovered_regions"]:
        print("  None - 100% coverage!")
    else:
        print(f"  {'Start':>8} {'End':>8} {'Size':>8}  Description")
        print("  " + "-" * 50)
        
        for start, end in analysis["uncovered_regions"]:
            size = end - start
            # Try to identify what this region might be
            desc = identify_region(start, end, analysis)
            print(f"  0x{start:04X}   0x{end:04X}   {size:>6}  {desc}")


def identify_region(start: int, end: int, analysis: dict) -> str:
    """Try to identify what an uncovered region might contain."""
    # Known regions based on reverse engineering notes
    
    # Between level metadata and sector table
    level_meta_end = analysis["level_count"] * LEVEL_METADATA_ENTRY_SIZE
    if start >= level_meta_end and end <= SECTOR_TABLE_OFFSET:
        return "Between level metadata and sector table (unknown)"
    
    # After sector table, before count fields
    sector_table_end = SECTOR_TABLE_OFFSET + (analysis["sector_table_count"] * SECTOR_TABLE_ENTRY_SIZE)
    if start >= sector_table_end and end <= LEVEL_COUNT_OFFSET:
        return "Between sector table and count fields (unknown)"
    
    # After count fields
    if start >= SECTOR_TABLE_COUNT_OFFSET + 1:
        return "After count fields (unknown/padding)"
    
    # Within level metadata entries (unparsed fields)
    if start < level_meta_end:
        return "Level metadata (unparsed fields before name)"
    
    return "Unknown purpose"


def print_visual_map(analysis: dict, width: int = 64) -> None:
    """Print a visual representation of header coverage."""
    print()
    print("Visual Coverage Map (. = uncovered, # = covered):")
    print("  Each character = 64 bytes")
    print()
    
    bitmap = analysis["covered_bitmap"]
    bytes_per_char = HEADER_SIZE // width
    
    line = "  "
    for i in range(width):
        start = i * bytes_per_char
        end = start + bytes_per_char
        covered_count = sum(bitmap[start:end])
        
        if covered_count == 0:
            line += "."
        elif covered_count == bytes_per_char:
            line += "#"
        else:
            # Partially covered
            line += "+"
    
    print(line)
    print(f"  |{'─' * 14}|{'─' * 14}|{'─' * 14}|{'─' * 14}|")
    print(f"  0x000      0x400      0x800      0xC00      0xFFF")


def print_unknown_hex(analysis: dict) -> None:
    """Print colorized hex dump of unknown/uncovered regions."""
    print()
    print(f"{Colors.BOLD}Unknown Bytes Hex Dump:{Colors.RESET}")
    print(f"  {Colors.GRAY}(Showing uncovered regions with 16-byte context){Colors.RESET}")
    print()
    
    raw_header = analysis["raw_header"]
    bitmap = analysis["covered_bitmap"]
    
    for region_start, region_end in analysis["uncovered_regions"]:
        region_size = region_end - region_start
        
        # Skip tiny single-byte gaps in the sector table
        if region_size == 1:
            continue
        
        print(f"  {Colors.CYAN}Region 0x{region_start:04X} - 0x{region_end:04X} ({region_size} bytes){Colors.RESET}")
        
        # Align to 16-byte boundary for display
        display_start = (region_start // 16) * 16
        display_end = ((region_end + 15) // 16) * 16
        display_end = min(display_end, HEADER_SIZE)
        
        for row_start in range(display_start, display_end, 16):
            # Address
            line = f"  {Colors.YELLOW}0x{row_start:04X}{Colors.RESET}  "
            
            # Hex bytes
            hex_part = ""
            ascii_part = ""
            for i in range(16):
                addr = row_start + i
                if addr >= HEADER_SIZE:
                    hex_part += "   "
                    ascii_part += " "
                    continue
                    
                byte = raw_header[addr]
                is_covered = bitmap[addr]
                is_in_region = region_start <= addr < region_end
                
                if is_in_region:
                    # Unknown byte - highlight in red
                    hex_part += f"{Colors.RED}{Colors.BOLD}{byte:02X}{Colors.RESET} "
                    # ASCII representation
                    if 0x20 <= byte <= 0x7E:
                        ascii_part += f"{Colors.RED}{chr(byte)}{Colors.RESET}"
                    else:
                        ascii_part += f"{Colors.RED}.{Colors.RESET}"
                elif is_covered:
                    # Known/covered byte - dim gray for context
                    hex_part += f"{Colors.DIM}{byte:02X}{Colors.RESET} "
                    if 0x20 <= byte <= 0x7E:
                        ascii_part += f"{Colors.DIM}{chr(byte)}{Colors.RESET}"
                    else:
                        ascii_part += f"{Colors.DIM}.{Colors.RESET}"
                else:
                    # Outside region but uncovered
                    hex_part += f"{Colors.GRAY}{byte:02X}{Colors.RESET} "
                    if 0x20 <= byte <= 0x7E:
                        ascii_part += f"{Colors.GRAY}{chr(byte)}{Colors.RESET}"
                    else:
                        ascii_part += f"{Colors.GRAY}.{Colors.RESET}"
                
                # Add separator after 8 bytes
                if i == 7:
                    hex_part += " "
            
            print(f"{line}{hex_part} {Colors.MAGENTA}|{Colors.RESET}{ascii_part}{Colors.MAGENTA}|{Colors.RESET}")
        
        print()
    
    # Summary of single-byte unknowns (sector table padding)
    single_byte_regions = [(s, e) for s, e in analysis["uncovered_regions"] if e - s == 1]
    if single_byte_regions:
        print(f"  {Colors.CYAN}Single-byte unknowns (sector table entry byte 7):{Colors.RESET}")
        values = []
        for start, end in single_byte_regions:
            byte = raw_header[start]
            values.append(f"{Colors.RED}0x{byte:02X}{Colors.RESET}")
        
        # Print in rows of 16
        for i in range(0, len(values), 16):
            row = values[i:i+16]
            print(f"    {' '.join(row)}")
        print()


def main():
    """Run coverage analysis."""
    if len(sys.argv) > 1:
        blb_path = Path(sys.argv[1])
    else:
        blb_path = Path(__file__).parent.parent / "disks" / "blb" / "GAME.BLB"
    
    if not blb_path.exists():
        print(f"Error: BLB file not found: {blb_path}")
        sys.exit(1)
    
    # Header coverage analysis
    analysis = analyze_coverage(blb_path)
    print_report(analysis)
    print_visual_map(analysis)
    print_unknown_hex(analysis)
    
    # Full file coverage analysis
    file_coverage = analyze_file_coverage(blb_path)
    print()
    print("=" * 70)
    print("BLB File Content Coverage (entire file, not just header)")
    print("=" * 70)
    print(f"  File size:      {file_coverage['file_size']:,} bytes ({file_coverage['file_size'] / 1024 / 1024:.2f} MB)")
    print(f"  {Colors.GREEN}Extracted:      {file_coverage['covered_bytes']:,} bytes ({file_coverage['coverage_percent']:.1f}%){Colors.RESET}")
    print(f"  {Colors.YELLOW}Unextracted:    {file_coverage['uncovered_bytes']:,} bytes ({100 - file_coverage['coverage_percent']:.1f}%){Colors.RESET}")
    
    if file_coverage['gaps']:
        print()
        print(f"  Gaps (unextracted regions):")
        for gap_start, gap_end, gap_size in file_coverage['gaps']:
            print(f"    0x{gap_start:08X} - 0x{gap_end:08X} ({gap_size:,} bytes)")


if __name__ == "__main__":
    main()
