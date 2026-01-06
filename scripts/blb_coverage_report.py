#!/usr/bin/env python3
"""
BLB Coverage Report Generator

Generates a comprehensive sector coverage report for all BLB files based on
the structure defined in blb.hexpat template.

Template Reference (blb.hexpat):
- Header:             0x0000 - 0x0FFF  (4 KB, 2 sectors)
- Level Table:        0x0000, 26 entries × 0x70 bytes
- Movie Table:        0x0B60, 13 entries × 0x1C bytes  
- Sector Table:       0x0CD0, 32 entries × 0x10 bytes
- Password Table:     0x0ECC, 17 entries × 4 bytes (entry 0 overlaps sectors[31])
- Credits Table:      0x0F10, 2 entries × 0x0C bytes
- Counts:             0x0F31 (levels), 0x0F32 (movies), 0x0F33 (sectors)
- Playback Sequence:  0x0F34 - 0x0FFF
"""

import struct
import os
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Tuple, Optional

# Template-defined constants
SECTOR_SIZE = 2048
HEADER_SIZE = 0x1000  # 4096 bytes = 2 sectors

# Template-defined header offsets
LEVEL_TABLE_OFFSET = 0x0000
LEVEL_ENTRY_SIZE = 0x70  # 112 bytes
MAX_LEVELS = 26

MOVIE_TABLE_OFFSET = 0x0B60
MOVIE_ENTRY_SIZE = 0x1C  # 28 bytes
MAX_MOVIES = 13

SECTOR_TABLE_OFFSET = 0x0CD0
SECTOR_ENTRY_SIZE = 0x10  # 16 bytes
MAX_SECTORS = 32

PASSWORD_TABLE_OFFSET = 0x0ECC  # Entry[0] overlaps with sectors[31]
PASSWORD_ENTRY_SIZE = 4  # u16 sector_offset, u16 sector_count
MAX_PASSWORDS = 17  # Entry 0 at 0xECC, entries 1-16 at 0xED0-0xF0F

CREDITS_TABLE_OFFSET = 0x0F10
CREDITS_ENTRY_SIZE = 0x0C  # 12 bytes
MAX_CREDITS = 2

# Count field offsets
LEVEL_COUNT_OFFSET = 0x0F31
MOVIE_COUNT_OFFSET = 0x0F32
SECTOR_COUNT_OFFSET = 0x0F33


@dataclass
class Region:
    """Represents a contiguous region of sectors"""
    start_sector: int
    sector_count: int
    name: str
    category: str  # header, level, loading, password, credits, hidden, gameover
    
    @property
    def end_sector(self) -> int:
        return self.start_sector + self.sector_count - 1
    
    @property
    def start_offset(self) -> int:
        return self.start_sector * SECTOR_SIZE
    
    @property
    def end_offset(self) -> int:
        return (self.start_sector + self.sector_count) * SECTOR_SIZE - 1
    
    @property
    def size_bytes(self) -> int:
        return self.sector_count * SECTOR_SIZE


@dataclass
class LevelEntry:
    """Level metadata from template LevelEntry struct (0x70 bytes)"""
    index: int
    sector_offset: int
    sector_count: int
    primary_buffer_size: int
    entry1_offset: int
    level_index: int
    level_flag: int
    stage_count: int
    tert_data_off: List[int]  # 6 values
    sec_sector_off: List[int]  # 6 values
    sec_sector_cnt: List[int]  # 6 values
    tert_sector_off: List[int]  # 6 values
    tert_sector_cnt: List[int]  # 6 values
    level_id: str
    name: str


@dataclass
class SectorEntry:
    """Sector table entry from template SectorEntry struct (0x10 bytes)"""
    level_index: int
    entry_flags: int
    unknown_byte: int
    code: str
    short_name: str
    sector_offset: int
    sector_count: int


@dataclass
class PasswordEntry:
    """Password screen entry from template PasswordScreenEntry struct (4 bytes)"""
    sector_offset: int
    sector_count: int


@dataclass 
class CreditsEntry:
    """Credits entry from template CreditsEntry struct (0x0C bytes)"""
    code: str
    param_a: int
    param_b: int


@dataclass
class BLBHeader:
    """Parsed BLB header based on template BLBHeader struct"""
    level_count: int
    movie_count: int
    sector_count: int
    levels: List[LevelEntry]
    sectors: List[SectorEntry]
    passwords: List[PasswordEntry]
    credits: List[CreditsEntry]


def parse_level_entry(data: bytes, index: int) -> LevelEntry:
    """Parse a single level entry (0x70 bytes) per template LevelEntry struct"""
    offset = LEVEL_TABLE_OFFSET + index * LEVEL_ENTRY_SIZE
    entry = data[offset:offset + LEVEL_ENTRY_SIZE]
    
    # Primary data pointers (0x00-0x0B)
    sector_offset, sector_count = struct.unpack_from('<HH', entry, 0x00)
    primary_buffer_size = struct.unpack_from('<I', entry, 0x04)[0]
    entry1_offset = struct.unpack_from('<I', entry, 0x08)[0]
    
    # Level identification (0x0C-0x0D)
    level_index, level_flag = struct.unpack_from('<BB', entry, 0x0C)
    
    # Stage count (0x0E)
    stage_count = struct.unpack_from('<H', entry, 0x0E)[0]
    
    # Tertiary data offsets (0x10-0x1B)
    tert_data_off = list(struct.unpack_from('<6H', entry, 0x10))
    
    # Secondary sector locations (0x1E-0x39)
    sec_sector_off = list(struct.unpack_from('<6H', entry, 0x1E))
    sec_sector_cnt = list(struct.unpack_from('<6H', entry, 0x2C))
    
    # Tertiary sector locations (0x3A-0x55)
    tert_sector_off = list(struct.unpack_from('<6H', entry, 0x3A))
    tert_sector_cnt = list(struct.unpack_from('<6H', entry, 0x48))
    
    # Level strings (0x56-0x6F)
    level_id = entry[0x56:0x5B].rstrip(b'\x00').decode('ascii', errors='replace')
    name = entry[0x5B:0x70].rstrip(b'\x00').decode('ascii', errors='replace')
    
    return LevelEntry(
        index=index,
        sector_offset=sector_offset,
        sector_count=sector_count,
        primary_buffer_size=primary_buffer_size,
        entry1_offset=entry1_offset,
        level_index=level_index,
        level_flag=level_flag,
        stage_count=stage_count,
        tert_data_off=tert_data_off,
        sec_sector_off=sec_sector_off,
        sec_sector_cnt=sec_sector_cnt,
        tert_sector_off=tert_sector_off,
        tert_sector_cnt=tert_sector_cnt,
        level_id=level_id,
        name=name
    )


def parse_sector_entry(data: bytes, index: int) -> SectorEntry:
    """Parse a single sector entry (0x10 bytes) per template SectorEntry struct
    
    Two layout variants exist (same fields, different byte order):
    
    PAL/NTSC-US Layout:
        0x00: u8  level_index
        0x01: u8  entry_flags  
        0x02: u8  unknown_byte
        0x03: char[5] code (4-char null-terminated)
        0x08: char[4] short_name
        0x0C: u16 sector_offset
        0x0E: u16 sector_count
        
    JP Layout (offset/count moved to front):
        0x00: u16 sector_offset
        0x02: u16 sector_count
        0x04: u8  level_index
        0x05: u8  entry_flags
        0x06: u8  unknown_byte
        0x07: char[5] code (4-char null-terminated)
        0x0C: char[4] short_name
    
    Detection: In PAL, byte[3] is uppercase A-Z (code start), byte[7] is 0x00.
               In JP, byte[3] is 0x00 (count low byte), byte[7] is uppercase A-Z.
    """
    offset = SECTOR_TABLE_OFFSET + index * SECTOR_ENTRY_SIZE
    entry = data[offset:offset + SECTOR_ENTRY_SIZE]
    
    # Detect layout by checking where the code starts
    # PAL: code at byte 3 (uppercase letter), byte 7 is null terminator
    # JP: code at byte 7 (uppercase letter), byte 3 is low byte of count (usually small)
    is_pal_layout = (65 <= entry[3] <= 90)  # byte[3] is A-Z
    
    if is_pal_layout:
        # PAL/NTSC-US layout: code at offset 3
        level_index = entry[0]
        entry_flags = entry[1]
        unknown_byte = entry[2]
        code = entry[3:8].rstrip(b'\x00').decode('ascii', errors='replace')
        short_name = entry[8:12].rstrip(b'\x00').decode('ascii', errors='replace')
        sector_offset, sector_count = struct.unpack_from('<HH', entry, 0x0C)
    else:
        # JP layout: offset/count at front, code at offset 7
        sector_offset, sector_count = struct.unpack_from('<HH', entry, 0x00)
        level_index = entry[4]
        entry_flags = entry[5]
        unknown_byte = entry[6]
        code = entry[7:12].rstrip(b'\x00').decode('ascii', errors='replace')
        short_name = entry[12:16].rstrip(b'\x00').decode('ascii', errors='replace')
    
    return SectorEntry(
        level_index=level_index,
        entry_flags=entry_flags,
        unknown_byte=unknown_byte,
        code=code,
        short_name=short_name,
        sector_offset=sector_offset,
        sector_count=sector_count
    )


def parse_password_entry(data: bytes, index: int) -> PasswordEntry:
    """Parse a single password entry (4 bytes) per template PasswordScreenEntry struct"""
    # Entry[0] is at 0xECC (overlaps sectors[31]), entries 1-16 at 0xED0+
    offset = PASSWORD_TABLE_OFFSET + index * PASSWORD_ENTRY_SIZE
    sector_offset, sector_count = struct.unpack_from('<HH', data, offset)
    return PasswordEntry(sector_offset=sector_offset, sector_count=sector_count)


def parse_credits_entry(data: bytes, index: int) -> CreditsEntry:
    """Parse a single credits entry (0x0C bytes) per template CreditsEntry struct"""
    offset = CREDITS_TABLE_OFFSET + index * CREDITS_ENTRY_SIZE
    entry = data[offset:offset + CREDITS_ENTRY_SIZE]
    
    code = entry[0:4].rstrip(b'\x00').decode('ascii', errors='replace')
    # reserved bytes 4-7
    param_a, param_b = struct.unpack_from('<HH', entry, 0x08)
    
    return CreditsEntry(code=code, param_a=param_a, param_b=param_b)


def parse_header(data: bytes) -> BLBHeader:
    """Parse BLB header per template BLBHeader struct"""
    # Read counts
    level_count = data[LEVEL_COUNT_OFFSET]
    movie_count = data[MOVIE_COUNT_OFFSET]
    sector_count = data[SECTOR_COUNT_OFFSET]
    
    # Parse all entries
    levels = [parse_level_entry(data, i) for i in range(min(level_count, MAX_LEVELS))]
    sectors = [parse_sector_entry(data, i) for i in range(min(sector_count, MAX_SECTORS))]
    passwords = [parse_password_entry(data, i) for i in range(MAX_PASSWORDS)]
    credits = [parse_credits_entry(data, i) for i in range(MAX_CREDITS)]
    
    return BLBHeader(
        level_count=level_count,
        movie_count=movie_count,
        sector_count=sector_count,
        levels=levels,
        sectors=sectors,
        passwords=passwords,
        credits=credits
    )


def collect_regions(header: BLBHeader, file_size: int) -> List[Region]:
    """Collect all referenced regions from header tables"""
    regions = []
    total_sectors = file_size // SECTOR_SIZE
    
    # 1. Header (always sectors 0-1)
    regions.append(Region(0, 2, "Header", "header"))
    
    # Handle unusual files (prototypes, demos) with few levels
    if header.level_count < 10:
        # Simplified handling for demo/prototype builds
        # Just collect level data and sector table entries
        for level in header.levels:
            if level.sector_count > 0:
                regions.append(Region(
                    level.sector_offset, 
                    level.sector_count,
                    f"Level {level.index} Primary: {level.level_id}",
                    "level"
                ))
            for stage in range(min(level.stage_count, 6)):
                if level.sec_sector_cnt[stage] > 0 and level.sec_sector_off[stage] > 0:
                    regions.append(Region(
                        level.sec_sector_off[stage],
                        level.sec_sector_cnt[stage],
                        f"Level {level.index} Secondary[{stage}]: {level.level_id}",
                        "level"
                    ))
                if level.tert_sector_cnt[stage] > 0 and level.tert_sector_off[stage] > 0:
                    regions.append(Region(
                        level.tert_sector_off[stage],
                        level.tert_sector_cnt[stage],
                        f"Level {level.index} Tertiary[{stage}]: {level.level_id}",
                        "level"
                    ))
        
        for i, sec in enumerate(header.sectors):
            if sec.sector_count > 0 and sec.sector_offset > 0:
                regions.append(Region(sec.sector_offset, sec.sector_count, f"Sector[{i}]: {sec.code}", "loading"))
        
        return regions
    
    # Full game handling (26 levels)
    
    # 2. Hidden legal screen (sectors 2-11) - unreferenced but known
    # Check if sector table entry 0 starts after sector 11
    if header.sectors and header.sectors[0].sector_offset > 11:
        regions.append(Region(2, 10, "Hidden Legal (unreferenced)", "hidden"))
    
    # 3. Sector table entries (loading screens, title, legal, game over)
    for i, sec in enumerate(header.sectors):
        if sec.sector_count > 0 and sec.sector_offset > 0:
            if sec.entry_flags == 0x05:
                category = "loading"
                name = f"Boot: {sec.code}"
            elif sec.entry_flags == 0x03:
                category = "gameover"
                name = f"Game Over: {sec.code}"
            elif i >= 2 and i <= 27:
                category = "loading"
                name = f"Loading: {sec.code}"
            elif i == 28:
                category = "loading"
                name = f"End Loading: {sec.code}"
            else:
                category = "loading"
                name = f"Sector[{i}]: {sec.code}"
            
            regions.append(Region(sec.sector_offset, sec.sector_count, name, category))
    
    # 4. Level data (primary, secondary, tertiary segments)
    for level in header.levels:
        # Primary segment
        if level.sector_count > 0:
            regions.append(Region(
                level.sector_offset, 
                level.sector_count,
                f"Level {level.index} Primary: {level.level_id}",
                "level"
            ))
        
        # Secondary segments (per-stage)
        for stage in range(level.stage_count):
            if level.sec_sector_cnt[stage] > 0 and level.sec_sector_off[stage] > 0:
                regions.append(Region(
                    level.sec_sector_off[stage],
                    level.sec_sector_cnt[stage],
                    f"Level {level.index} Secondary[{stage}]: {level.level_id}",
                    "level"
                ))
        
        # Tertiary segments (per-stage)
        for stage in range(level.stage_count):
            if level.tert_sector_cnt[stage] > 0 and level.tert_sector_off[stage] > 0:
                regions.append(Region(
                    level.tert_sector_off[stage],
                    level.tert_sector_cnt[stage],
                    f"Level {level.index} Tertiary[{stage}]: {level.level_id}",
                    "level"
                ))
    
    # 5. Password screens (from password table at 0xECC)
    for i, pw in enumerate(header.passwords):
        if pw.sector_count > 0 and pw.sector_offset > 0:
            if i == 16:
                name = "Victory Screen (YOU WIN)"
            else:
                name = f"Password Screen #{i}"
            regions.append(Region(pw.sector_offset, pw.sector_count, name, "password"))
    
    # 6. Credits containers
    # Per template: Credits container #1 is at sectors 26-196 (0xD000)
    # Credits container #2 is a duplicate near end of file for CD seek optimization
    # The credits table at 0xF10 has 2 entries that reference these
    
    # First credits container: always at sector 26 after boot screens
    # Size is 171 sectors (verified in template)
    regions.append(Region(26, 171, "Credits Container #1", "credits"))
    
    # Second credits container: near end of file
    # Find the loading screen that comes before game over (CRED or similar)
    # The credits container #2 follows that loading screen
    credits2_start = 0
    for sec in header.sectors:
        # Look for CRED entry which marks end of credits container #2
        if sec.code == 'CRED' and sec.sector_offset > 30000:
            # Credits container #2 is the 171 sectors BEFORE this CRED loading screen
            credits2_start = sec.sector_offset - 171
            break
    
    if credits2_start == 0:
        # Fallback: look for highest loading screen before game over
        for sec in header.sectors:
            if sec.entry_flags == 0x00 and sec.sector_offset > 30000:
                # Found a loading screen near end, credits container is after it
                end_sector = sec.sector_offset + sec.sector_count
                if end_sector > credits2_start:
                    credits2_start = end_sector
    
    if credits2_start > 30000:  # Sanity check
        regions.append(Region(credits2_start, 171, "Credits Container #2 (duplicate)", "credits"))
    
    return regions


def compute_coverage(regions: List[Region], file_size: int) -> Tuple[int, int, List[Tuple[int, int]]]:
    """Compute total coverage and find gaps"""
    total_sectors = file_size // SECTOR_SIZE
    
    # Create a coverage bitmap
    covered = [False] * total_sectors
    
    for region in regions:
        for s in range(region.start_sector, region.start_sector + region.sector_count):
            if 0 <= s < total_sectors:
                covered[s] = True
    
    # Count covered sectors
    covered_count = sum(covered)
    
    # Find gaps
    gaps = []
    in_gap = False
    gap_start = 0
    
    for i, is_covered in enumerate(covered):
        if not is_covered and not in_gap:
            in_gap = True
            gap_start = i
        elif is_covered and in_gap:
            in_gap = False
            gaps.append((gap_start, i - gap_start))
    
    if in_gap:
        gaps.append((gap_start, total_sectors - gap_start))
    
    return covered_count, total_sectors, gaps


def generate_report(blb_path: str) -> str:
    """Generate coverage report for a single BLB file"""
    file_size = os.path.getsize(blb_path)
    total_sectors = file_size // SECTOR_SIZE
    
    with open(blb_path, 'rb') as f:
        header_data = f.read(HEADER_SIZE)
    
    header = parse_header(header_data)
    regions = collect_regions(header, file_size)
    covered_count, total_count, gaps = compute_coverage(regions, file_size)
    
    # Sort regions by start sector
    regions.sort(key=lambda r: r.start_sector)
    
    # Build report
    lines = []
    filename = os.path.basename(blb_path)
    
    lines.append("=" * 80)
    lines.append(f"BLB COVERAGE REPORT: {filename}")
    lines.append("=" * 80)
    lines.append("")
    lines.append("FILE STATISTICS")
    lines.append("-" * 40)
    lines.append(f"  File Size:     {file_size:,} bytes ({file_size / 1024 / 1024:.2f} MB)")
    lines.append(f"  Total Sectors: {total_count:,}")
    lines.append(f"  Level Count:   {header.level_count}")
    lines.append(f"  Movie Count:   {header.movie_count}")
    lines.append(f"  Sector Entries:{header.sector_count}")
    lines.append("")
    
    # Category breakdown
    category_sectors = {}
    for region in regions:
        category_sectors[region.category] = category_sectors.get(region.category, 0) + region.sector_count
    
    lines.append("COVERAGE BY CATEGORY")
    lines.append("-" * 40)
    for cat in ['header', 'level', 'loading', 'password', 'credits', 'hidden', 'gameover']:
        if cat in category_sectors:
            count = category_sectors[cat]
            pct = count / total_count * 100
            lines.append(f"  {cat.capitalize():12s}: {count:6,} sectors ({pct:5.2f}%)")
    
    lines.append(f"  {'TOTAL':12s}: {covered_count:6,} sectors ({covered_count/total_count*100:5.2f}%)")
    lines.append("")
    
    # Coverage status
    if covered_count == total_count:
        lines.append("✅ COVERAGE: 100% - All sectors accounted for!")
    else:
        uncovered = total_count - covered_count
        lines.append(f"⚠️  COVERAGE: {covered_count/total_count*100:.2f}% - {uncovered} sectors unaccounted")
    lines.append("")
    
    # Gaps
    if gaps:
        lines.append("UNCOVERED GAPS")
        lines.append("-" * 40)
        for start, count in gaps:
            end = start + count - 1
            offset = start * SECTOR_SIZE
            lines.append(f"  Sectors {start:5d}-{end:5d} ({count:4d} sectors) @ 0x{offset:08X}")
        lines.append("")
    
    # Region details (summarized)
    lines.append("REGION SUMMARY")
    lines.append("-" * 40)
    
    # Group by category
    for cat in ['header', 'hidden', 'loading', 'credits', 'level', 'password', 'gameover']:
        cat_regions = [r for r in regions if r.category == cat]
        if cat_regions:
            lines.append(f"\n  [{cat.upper()}] ({len(cat_regions)} regions)")
            for r in cat_regions[:20]:  # Limit to first 20
                lines.append(f"    {r.start_sector:5d}-{r.end_sector:5d} ({r.sector_count:4d}): {r.name}")
            if len(cat_regions) > 20:
                lines.append(f"    ... and {len(cat_regions) - 20} more")
    
    lines.append("")
    lines.append("=" * 80)
    
    return "\n".join(lines)


def main():
    """Generate coverage reports for all BLB files"""
    blb_dir = Path(__file__).parent.parent / "disks" / "blb"
    
    if not blb_dir.exists():
        print(f"Error: BLB directory not found: {blb_dir}")
        return
    
    blb_files = sorted(blb_dir.glob("*.blb")) + sorted(blb_dir.glob("*.BLB"))
    
    if not blb_files:
        print(f"No BLB files found in {blb_dir}")
        return
    
    print(f"Generating coverage reports for {len(blb_files)} BLB files...\n")
    
    all_reports = []
    summary = []
    
    for blb_path in blb_files:
        try:
            report = generate_report(str(blb_path))
            all_reports.append(report)
            
            # Extract summary line
            file_size = os.path.getsize(blb_path)
            with open(blb_path, 'rb') as f:
                header_data = f.read(HEADER_SIZE)
            header = parse_header(header_data)
            regions = collect_regions(header, file_size)
            covered, total, gaps = compute_coverage(regions, file_size)
            pct = covered / total * 100
            status = "✅" if covered == total else "⚠️"
            summary.append((blb_path.name, pct, covered, total, len(gaps), header.level_count))
            
        except Exception as e:
            all_reports.append(f"Error processing {blb_path.name}: {e}")
            summary.append((blb_path.name, 0, 0, 0, -1, 0))
    
    # Print summary table first
    print("=" * 80)
    print("BLB COVERAGE SUMMARY - ALL FILES")
    print("=" * 80)
    print(f"{'File':<25} {'Coverage':>10} {'Covered':>10} {'Total':>10} {'Gaps':>6} {'Levels':>7}")
    print("-" * 80)
    
    for name, pct, covered, total, gap_count, levels in summary:
        status = "✅" if pct == 100 else "⚠️"
        gaps_str = str(gap_count) if gap_count >= 0 else "ERR"
        print(f"{name:<25} {pct:>9.2f}% {covered:>10,} {total:>10,} {gaps_str:>6} {levels:>7}")
    
    print("-" * 80)
    print("")
    
    # Print individual reports
    for report in all_reports:
        print(report)
        print("")


if __name__ == "__main__":
    main()
