#!/usr/bin/env python3
"""
blb.py - Library for parsing Skullmonkeys GAME.BLB archive headers

The BLB (blob) file is the main game archive containing all levels, assets,
and resources. The first 2 sectors (4096 bytes) contain the header with an
index table describing the contents.

Header Structure (4096 bytes total):
=====================================
    Offset  Size   Description
    ------  ----   -----------
    0x000   2912   Level metadata table (0x70 bytes × 26 entries max)
    0xB60    364   Movie table (0x1C bytes × 13 entries max)
    0xCD0    512   Sector/loading screen table (0x10 bytes × 32 entries max)
    0xED0     64   Unknown u32 array (16 entries) - per-world data, checksums/seeds?
    0xF10     33   Credits sequence table (0x0C bytes × 2-3 entries, overlaps counts?)
    0xF31      1   Level count (u8)
    0xF32      1   Asset/movie count (u8)
    0xF33      1   Sector table entry count (u8)
    0xF34    204   State/config data region


Credits Sequence Table (0x0C = 12 bytes, at offset 0xF10):
==========================================================
    Offset  Size  Type    Field               Description
    ------  ----  ----    -----               -----------
    0x00       4  str     code                4-char ID (empty for entry 0, "CRD1"/"CRD2" for others)
    0x04       4  bytes   padding             Unused (zeros)
    0x08       2  u16     param_a             Entry 0: level_count; CRD1/2: unknown counter
    0x0A       2  u16     param_b             Base sector offset (CRED.sector - level_count)

    Note: Entry 2 (CRD2) overlaps with count fields at 0xF31-0xF33.
    Only 2 complete entries fit (indices 0 and 1).
    
    Accessed by func_8007ABCC and func_8007AC54 when state (0xF36) == 2,
    using index from 0xF92 with stride of 12 bytes.


Level Metadata Entry (0x70 = 112 bytes, at offset 0x000):
=========================================================
    Offset  Size  Type    Field               Description
    ------  ----  ----    -----               -----------
    0x00       2  u16     sector_offset       Primary data sector offset in BLB
    0x02       2  u16     sector_count        Primary data sector count
    0x04       8  bytes   static_data         Unknown static data
    0x0C       1  u8      level_index         Level index number
    0x0D       1  u8      flag                Unknown flag
    0x0E      14  bytes   unknown_0e          Unknown
    0x1C       2  bytes   unknown_1c          Unknown
    0x1E       2  u16     secondary_offset    Secondary data sector offset
    0x20       2  bytes   unknown_20          Unknown
    0x22      10  bytes   dynamic_data_1      Unknown, may contain pointers
    0x2C       2  u16     secondary_count     Secondary data sector count
    0x2E      16  bytes   unknown_2e          Unknown
    0x3E       2  bytes   unknown_3e          Unknown
    0x40       2  u16     tertiary_offset     Tertiary data sector offset
    0x42       2  bytes   unknown_42          Unknown
    0x44      10  bytes   dynamic_data_2      Unknown, may contain pointers
    0x4E       2  u16     tertiary_count      Tertiary data sector count
    0x50       6  bytes   unknown_50          Unknown
    0x56       5  str     level_id            4-char null-terminated (e.g., "MENU")
    0x5B      21  str     name                Level name, null-terminated


Movie Entry (0x1C = 28 bytes, at offset 0xB60):
===============================================
    Offset  Size  Type    Field               Description
    ------  ----  ----    -----               -----------
    0x00       2  u16     unknown_00          Reserved/unused (always 0)
    0x02       2  u16     sector_count        Movie size in sectors
    0x04       5  str     movie_id            4-char null-terminated (e.g., "LOGO")
    0x09       3  str     short_name          2-char null-terminated
    0x0C      16  str     filename            CD path (e.g., "\\MVLOGO.STR;1")

    Note: Movies are external .STR files on the CD, not embedded in GAME.BLB.


Sector Table Entry (0x10 = 16 bytes, at offset 0xCD0):
======================================================
    Offset  Size  Type    Field               Description
    ------  ----  ----    -----               -----------
    0x00       1  u8      level_index         Level index (0-25) when hi_byte=0
    0x01       1  u8      entry_flags         Entry type flags (0x00=level, 0x03=game over, 0x05=loading)
    0x02       1  u8      unknown_byte        Unknown (0x0A for loading screens, 0x63 for game over)
    0x03       5  str     code                4-char null-terminated (e.g., "PIRA")
    0x08       4  str     short_name          Truncated description
    0x0C       2  u16     sector_offset       Sector offset in BLB
    0x0E       2  u16     sector_count        Sector count

    Entry type patterns observed in PAL version:
      - entry_flags=0x00: Level loading screens (level_index = 0-25)
      - entry_flags=0x05: Special loading screens (PIRA=0x37, LEGL=0x32)
      - entry_flags=0x03: Game over screens (unknown_byte=0x63)
      - entry_flags=0x00, level_index=0x35: Credits screen


Constants:
==========
    SECTOR_SIZE = 2048 bytes (standard CD-ROM sector)
    HEADER_SIZE = 4096 bytes (2 sectors)

File Coverage (PAL version):
============================
    Header parsing covers 100% of header bytes (4,096 / 4,096).
    Of those, ~45% are understood fields, ~55% are unknown/TBD.
    
    Extractable data covers ~43% of total file size.
    Remaining ~57% may be padding, unreferenced data, or additional tables.
"""

import struct
from dataclasses import dataclass, field
from pathlib import Path
from typing import BinaryIO, Iterator, Optional


# Helper for marking fields as unknown/not fully understood
def unknown(description: str = "") -> field:
    """Mark a dataclass field as unknown/not fully understood purpose.
    
    Usage:
        unknown_field: bytes = unknown("Purpose TBD")
    
    The coverage tool will exclude these fields from the "understood" count.
    """
    return field(default=None, metadata={"unknown": True, "description": description})


def is_unknown_field(cls, field_name: str) -> bool:
    """Check if a field is marked as unknown."""
    from dataclasses import fields
    for f in fields(cls):
        if f.name == field_name:
            return f.metadata.get("unknown", False)
    return False

# Constants
SECTOR_SIZE = 2048
HEADER_SIZE = 2 * SECTOR_SIZE  # First 2 sectors (4096 bytes)

# Level metadata table (0x70 bytes per entry, at start of header)
LEVEL_METADATA_OFFSET = 0x000
LEVEL_METADATA_ENTRY_SIZE = 0x70
LEVEL_NAME_OFFSET = 0x5B  # Offset of name within each level metadata entry

# Movie table (0x1C bytes per entry, between level metadata and sector table)
MOVIE_TABLE_OFFSET = 0xB60
MOVIE_ENTRY_SIZE = 0x1C

# Sector offset table (0x10 bytes per entry)
SECTOR_TABLE_OFFSET = 0xCD0
SECTOR_TABLE_ENTRY_SIZE = 0x10

# Unknown u32 array (16 entries between sector table and credits table)
UNKNOWN_ARRAY_OFFSET = 0xED0
UNKNOWN_ARRAY_COUNT = 16

# Credits sequence table (0x0C bytes per entry)
CREDITS_TABLE_OFFSET = 0xF10
CREDITS_ENTRY_SIZE = 0x0C
CREDITS_MAX_ENTRIES = 2  # Only 2 complete entries fit before count fields

# Header count fields
LEVEL_COUNT_OFFSET = 0xF31  # Number of level entries (used by func_8007A9B0)
ASSET_COUNT_OFFSET = 0xF32  # Number of asset entries (used by func_8007ACDC)
SECTOR_TABLE_COUNT_OFFSET = 0xF33  # Total sector table entries

# State/config data region
STATE_DATA_OFFSET = 0xF34


@dataclass
class SectorTableEntry:
    """
    Represents a single entry in the BLB sector offset table (loading screens).
    
    Located at header offset 0xCD0, with 0x10 bytes per entry.
    The number of entries is stored at header offset 0xF33.
    
    Entry type patterns:
      - entry_flags=0x00, level_index=0-25: Level loading screens
      - entry_flags=0x05: Special loading screens (level_index=0x37 or 0x32)
      - entry_flags=0x03: Game over screens (unknown_byte=0x63)
      - entry_flags=0x00, level_index=0x35: Credits screen
    """
    
    index: int
    offset: int  # Offset of this entry within the header
    level_index: int = None  # u8 at +0x00: Level index when entry_flags=0x00
    entry_flags: int = None  # u8 at +0x01: Entry type (0x00=level, 0x03=game over, 0x05=loading)
    unknown_byte: int = unknown("+0x02: u8, 0x0A for loading screens, 0x63 for game over")
    code: str = None  # 5-byte level code at +0x03 (4-char null-terminated)
    short_name: str = None  # Short name at +0x08 (4 bytes, truncated description)
    sector_offset: int = None  # Sector offset within BLB file
    sector_count: int = None  # Number of sectors
    raw_data: bytes = None  # Full 16 bytes of entry data
    
    # Keep entry_type for backward compatibility (combines level_index and entry_flags)
    @property
    def entry_type(self) -> int:
        """Combined u16 of level_index (lo) and entry_flags (hi) for compatibility."""
        return (self.entry_flags << 8) | self.level_index
    
    @property
    def byte_offset(self) -> int:
        """Calculate the byte offset within the BLB file."""
        return self.sector_offset * SECTOR_SIZE
    
    @property
    def byte_size(self) -> int:
        """Calculate the size in bytes."""
        return self.sector_count * SECTOR_SIZE
    
    @property
    def is_level_screen(self) -> bool:
        """Check if this is a standard level loading screen."""
        return self.entry_flags == 0x00 and self.level_index < 0x20
    
    @property
    def is_special_loading(self) -> bool:
        """Check if this is a special loading screen (PIRA/LEGL intro)."""
        return self.entry_flags == 0x05
    
    @property
    def is_game_over(self) -> bool:
        """Check if this is a game over screen."""
        return self.entry_flags == 0x03
    
    @property
    def is_credits(self) -> bool:
        """Check if this is a credits screen."""
        return self.entry_flags == 0x00 and self.level_index == 0x35
    
    def __repr__(self) -> str:
        return (
            f"SectorTableEntry(idx={self.index}, code='{self.code}', "
            f"short='{self.short_name}', offset=0x{self.byte_offset:08X}, "
            f"size=0x{self.byte_size:X})"
        )


@dataclass
class LevelMetadataEntry:
    """
    Represents a single entry in the level metadata table.
    
    Located at header offset 0x000, with 0x70 bytes per entry.
    The number of entries is stored at header offset 0xF31.
    
    Contains level data locations (primary, secondary, tertiary) and metadata.
    """
    
    index: int
    offset: int  # Offset of this entry within the header
    raw_data: bytes  # Full 0x70 bytes of entry data
    
    # Primary level data location
    sector_offset: int = None  # u16 at +0x00, sector offset in BLB
    sector_count: int = None  # u16 at +0x02, sector count
    
    # Static and index data
    static_data: bytes = unknown("+0x04: 8 bytes, purpose TBD")
    level_index: int = None  # u8 at +0x0C
    flag: int = unknown("+0x0D: u8, purpose TBD")
    unknown_0e: bytes = unknown("+0x0E: 14 bytes")
    
    # Secondary data location (loading screen?)
    unknown_1c: bytes = unknown("+0x1C: 2 bytes")
    secondary_offset: int = None  # u16 at +0x1E
    unknown_20: bytes = unknown("+0x20: 2 bytes")
    dynamic_data_1: bytes = unknown("+0x22: 10 bytes, contains pointers?")
    secondary_count: int = None  # u16 at +0x2C
    unknown_2e: bytes = unknown("+0x2E: 16 bytes")
    
    # Tertiary data location
    unknown_3e: bytes = unknown("+0x3E: 2 bytes")
    tertiary_offset: int = None  # u16 at +0x40
    unknown_42: bytes = unknown("+0x42: 2 bytes")
    dynamic_data_2: bytes = unknown("+0x44: 10 bytes, contains pointers?")
    tertiary_count: int = None  # u16 at +0x4E
    unknown_50: bytes = unknown("+0x50: 6 bytes")
    
    # Identification
    level_id: str = None  # 5 bytes at +0x56 (4-char null-terminated)
    name: str = None  # 21 bytes at +0x5B (null-terminated)
    
    @property
    def byte_offset(self) -> int:
        """Calculate primary byte offset within the BLB file."""
        return self.sector_offset * SECTOR_SIZE
    
    @property
    def byte_size(self) -> int:
        """Calculate primary size in bytes."""
        return self.sector_count * SECTOR_SIZE
    
    @property
    def secondary_byte_offset(self) -> int:
        """Calculate secondary byte offset within the BLB file."""
        return self.secondary_offset * SECTOR_SIZE
    
    @property
    def secondary_byte_size(self) -> int:
        """Calculate secondary size in bytes."""
        return self.secondary_count * SECTOR_SIZE
    
    @property
    def tertiary_byte_offset(self) -> int:
        """Calculate tertiary byte offset within the BLB file."""
        return self.tertiary_offset * SECTOR_SIZE
    
    @property
    def tertiary_byte_size(self) -> int:
        """Calculate tertiary size in bytes."""
        return self.tertiary_count * SECTOR_SIZE
    
    def __repr__(self) -> str:
        return f"LevelMetadataEntry(idx={self.index}, id='{self.level_id}', name='{self.name}')"


@dataclass
class MovieEntry:
    """
    Represents a single entry in the movie table.
    
    Located at header offset 0xB60, with 0x1C bytes per entry.
    The number of entries matches the asset_count at header offset 0xF32.
    
    Movies are FMV sequences stored as .STR files on the CD.
    
    The unknown_00 field is accessed by func_8007AE14 as a u16, but is always 0
    in all known versions. It may have been intended as a start_sector offset,
    but movies are loaded by filename from the filesystem instead.
    """
    
    index: int
    offset: int  # Offset of this entry within the header
    raw_data: bytes = None  # Full 0x1C bytes of entry data (for debugging)
    
    # Fields
    unknown_00: int = unknown("+0x00: u16, always 0 - reserved/unused (accessed by func_8007AE14)")
    sector_count: int = None  # u16 at +0x02
    movie_id: str = None  # 5 bytes at +0x04 (4-char null-terminated)
    short_name: str = None  # 3 bytes at +0x09 (2-char null-terminated)
    filename: str = None  # 16 bytes at +0x0C (CD path)
    
    @property
    def byte_size(self) -> int:
        """Calculate the size in bytes."""
        return self.sector_count * SECTOR_SIZE
    
    def __repr__(self) -> str:
        return f"MovieEntry(idx={self.index}, id='{self.movie_id}', file='{self.filename}')"


@dataclass
class CreditsEntry:
    """
    Represents an entry in the credits sequence table.
    
    Located at header offset 0xF10, with 0x0C (12) bytes per entry.
    Only 2 complete entries fit before overlapping with count fields at 0xF31.
    
    Used by func_8007ABCC and func_8007AC54 when game state (0xF36) == 2.
    The index is read from 0xF92 and multiplied by 12 for table lookup.
    
    Entry 0 appears to be a default/header with param_a = level_count.
    Entry 1 (CRD1) and partial Entry 2 (CRD2) are for credits sequences.
    """
    
    index: int
    offset: int  # Offset of this entry within the header
    raw_data: bytes = None  # Full 0x0C bytes of entry data
    
    code: str = None  # 4-char code at +0x00 (empty for entry 0)
    padding: bytes = None  # 4 bytes at +0x04 (zeros)
    param_a: int = None  # u16 at +0x08 (level_count for entry 0)
    param_b: int = None  # u16 at +0x0A (base_sector = CRED.sector - level_count)
    
    def __repr__(self) -> str:
        code_str = self.code if self.code else "(empty)"
        return f"CreditsEntry(idx={self.index}, code='{code_str}', a=0x{self.param_a:04X}, b=0x{self.param_b:04X})"


# Keep BLBEntry as an alias for backward compatibility
BLBEntry = SectorTableEntry


@dataclass
class BLBHeader:
    """
    Represents the BLB file header.
    
    The header occupies the first 2 sectors (4096 bytes) and contains:
    - Level metadata table at 0x000 (0x70 bytes per entry)
    - Movie table at 0xB60 (0x1C bytes per entry)
    - Sector offset table at 0xCD0 (0x10 bytes per entry)
    - Unknown u32 array at 0xED0 (16 entries, 64 bytes)
    - Credits sequence table at 0xF10 (0x0C bytes per entry)
    - Count fields at 0xF31-0xF33
    - State/config data at 0xF34 (204 bytes)
    """
    
    raw_data: bytes  # Raw header data (first 4096 bytes)
    sector_entries: list[SectorTableEntry]  # Sector offset table entries
    level_entries: list[LevelMetadataEntry]  # Level metadata entries
    movie_entries: list[MovieEntry]  # Movie table entries
    credits_entries: list[CreditsEntry]  # Credits sequence table entries
    level_count: int  # Number of levels (from offset 0xF31)
    asset_count: int  # Number of assets/movies (from offset 0xF32)
    sector_table_count: int  # Total sector table entries (from offset 0xF33)
    
    # Unknown regions stored as raw bytes
    unknown_ed0: bytes = unknown("0xED0-0xF31: 97 bytes, purpose TBD")
    unknown_f34: bytes = unknown("0xF34-0x1000: 204 bytes, state data/padding")
    
    # Backward compatibility property
    @property
    def entries(self) -> list[SectorTableEntry]:
        """Alias for sector_entries for backward compatibility."""
        return self.sector_entries
    
    @property
    def movies(self) -> list[MovieEntry]:
        """Alias for movie_entries."""
        return self.movie_entries
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "BLBHeader":
        """
        Parse a BLB header from raw bytes.
        
        Args:
            data: Raw header data (at least 4096 bytes)
            
        Returns:
            Parsed BLBHeader instance
        """
        if len(data) < HEADER_SIZE:
            raise ValueError(f"Header data too short: {len(data)} < {HEADER_SIZE}")
        
        # Read count fields from header
        level_count = data[LEVEL_COUNT_OFFSET]
        asset_count = data[ASSET_COUNT_OFFSET]
        sector_table_count = data[SECTOR_TABLE_COUNT_OFFSET]
        
        # Parse sector offset table (count from 0xF33)
        sector_entries = []
        for i in range(sector_table_count):
            offset = SECTOR_TABLE_OFFSET + (i * SECTOR_TABLE_ENTRY_SIZE)
            if offset + SECTOR_TABLE_ENTRY_SIZE > len(data):
                break
            entry_data = data[offset:offset + SECTOR_TABLE_ENTRY_SIZE]
            sector_entries.append(cls._parse_sector_entry(i, offset, entry_data))
        
        # Parse level metadata table (count from 0xF31)
        level_entries = []
        for i in range(level_count):
            offset = LEVEL_METADATA_OFFSET + (i * LEVEL_METADATA_ENTRY_SIZE)
            if offset + LEVEL_METADATA_ENTRY_SIZE > len(data):
                break
            entry_data = data[offset:offset + LEVEL_METADATA_ENTRY_SIZE]
            level_entries.append(cls._parse_level_entry(i, offset, entry_data))
        
        # Parse movie table (count from 0xF32, dynamically verified)
        movie_entries = []
        for i in range(asset_count):
            offset = MOVIE_TABLE_OFFSET + (i * MOVIE_ENTRY_SIZE)
            if offset + MOVIE_ENTRY_SIZE > SECTOR_TABLE_OFFSET:
                # Don't read past sector table
                break
            entry_data = data[offset:offset + MOVIE_ENTRY_SIZE]
            # Verify this is a valid movie entry (first 2 bytes should be 0x0000)
            if entry_data[0:2] != b'\x00\x00':
                break
            movie_entries.append(cls._parse_movie_entry(i, offset, entry_data))
        
        # Parse credits sequence table (max 2 entries before count fields)
        credits_entries = []
        for i in range(CREDITS_MAX_ENTRIES):
            offset = CREDITS_TABLE_OFFSET + (i * CREDITS_ENTRY_SIZE)
            if offset + CREDITS_ENTRY_SIZE > LEVEL_COUNT_OFFSET:
                break
            entry_data = data[offset:offset + CREDITS_ENTRY_SIZE]
            credits_entries.append(cls._parse_credits_entry(i, offset, entry_data))
        
        # Read unknown regions as raw bytes
        unknown_ed0 = data[UNKNOWN_ARRAY_OFFSET:CREDITS_TABLE_OFFSET]  # 64 bytes (16 u32 values)
        unknown_f34 = data[STATE_DATA_OFFSET:0x1000]  # 204 bytes after count fields
        
        return cls(
            raw_data=data[:HEADER_SIZE],
            sector_entries=sector_entries,
            level_entries=level_entries,
            movie_entries=movie_entries,
            credits_entries=credits_entries,
            level_count=level_count,
            asset_count=asset_count,
            sector_table_count=sector_table_count,
            unknown_ed0=unknown_ed0,
            unknown_f34=unknown_f34,
        )
    
    @classmethod
    def from_file(cls, path: Path | str) -> "BLBHeader":
        """
        Load and parse a BLB header from a file.
        
        Args:
            path: Path to the BLB file
            
        Returns:
            Parsed BLBHeader instance
        """
        with open(path, 'rb') as f:
            data = f.read(HEADER_SIZE)
        return cls.from_bytes(data)
    
    @staticmethod
    def _parse_sector_entry(index: int, offset: int, data: bytes) -> SectorTableEntry:
        """Parse a single 16-byte sector table entry."""
        level_index = data[0]  # u8 at +0x00
        entry_flags = data[1]  # u8 at +0x01
        unknown_byte = data[2]  # u8 at +0x02
        
        # Level code: 5 bytes at offset 3 (4-char null-terminated)
        code_bytes = data[3:8]
        code = code_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
        
        # Short name: 4 bytes at offset 8
        short_bytes = data[8:12]
        short_name = short_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
        
        sector_offset = struct.unpack('<H', data[12:14])[0]
        sector_count = struct.unpack('<H', data[14:16])[0]
        
        return SectorTableEntry(
            index=index,
            offset=offset,
            level_index=level_index,
            entry_flags=entry_flags,
            unknown_byte=unknown_byte,
            code=code,
            short_name=short_name,
            sector_offset=sector_offset,
            sector_count=sector_count,
            raw_data=data,
        )
    
    @staticmethod
    def _parse_level_entry(index: int, offset: int, data: bytes) -> LevelMetadataEntry:
        """Parse a single 0x70-byte level metadata entry."""
        # Primary data location
        sector_offset = struct.unpack('<H', data[0x00:0x02])[0]
        sector_count = struct.unpack('<H', data[0x02:0x04])[0]
        
        # Static and index data
        static_data = data[0x04:0x0C]
        level_index = data[0x0C]
        flag = data[0x0D]
        unknown_0e = data[0x0E:0x1C]
        
        # Secondary data location
        unknown_1c = data[0x1C:0x1E]
        secondary_offset = struct.unpack('<H', data[0x1E:0x20])[0]
        unknown_20 = data[0x20:0x22]
        dynamic_data_1 = data[0x22:0x2C]
        secondary_count = struct.unpack('<H', data[0x2C:0x2E])[0]
        unknown_2e = data[0x2E:0x3E]
        
        # Tertiary data location
        unknown_3e = data[0x3E:0x40]
        tertiary_offset = struct.unpack('<H', data[0x40:0x42])[0]
        unknown_42 = data[0x42:0x44]
        dynamic_data_2 = data[0x44:0x4E]
        tertiary_count = struct.unpack('<H', data[0x4E:0x50])[0]
        unknown_50 = data[0x50:0x56]
        
        # Identification
        level_id_bytes = data[0x56:0x5B]
        level_id = level_id_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
        
        name_bytes = data[0x5B:0x70]
        name = name_bytes.split(b'\x00')[0].decode('ascii', errors='replace')
        
        return LevelMetadataEntry(
            index=index,
            offset=offset,
            raw_data=data,
            sector_offset=sector_offset,
            sector_count=sector_count,
            static_data=static_data,
            level_index=level_index,
            flag=flag,
            unknown_0e=unknown_0e,
            unknown_1c=unknown_1c,
            secondary_offset=secondary_offset,
            unknown_20=unknown_20,
            dynamic_data_1=dynamic_data_1,
            secondary_count=secondary_count,
            unknown_2e=unknown_2e,
            unknown_3e=unknown_3e,
            tertiary_offset=tertiary_offset,
            unknown_42=unknown_42,
            dynamic_data_2=dynamic_data_2,
            tertiary_count=tertiary_count,
            unknown_50=unknown_50,
            level_id=level_id,
            name=name,
        )
    
    @staticmethod
    def _parse_movie_entry(index: int, offset: int, data: bytes) -> MovieEntry:
        """Parse a single 0x1C-byte movie table entry."""
        # unknown_00: u16 at +0x00 (always 0 in all known versions)
        unknown_00 = struct.unpack('<H', data[0x00:0x02])[0]
        sector_count = struct.unpack('<H', data[0x02:0x04])[0]
        
        # Movie ID: 5 bytes at +0x04 (4-char null-terminated)
        movie_id_bytes = data[0x04:0x09]
        movie_id = movie_id_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
        
        # Short name: 3 bytes at +0x09 (2-char null-terminated)
        short_name_bytes = data[0x09:0x0C]
        short_name = short_name_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
        
        # Filename: 16 bytes at +0x0C
        filename_bytes = data[0x0C:0x1C]
        filename = filename_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
        
        return MovieEntry(
            index=index,
            offset=offset,
            raw_data=data,
            unknown_00=unknown_00,
            sector_count=sector_count,
            movie_id=movie_id,
            short_name=short_name,
            filename=filename,
        )
    
    @staticmethod
    def _parse_credits_entry(index: int, offset: int, data: bytes) -> CreditsEntry:
        """Parse a single 0x0C-byte credits sequence table entry."""
        # Code: 4 bytes at +0x00 (4-char, may be empty for entry 0)
        code_bytes = data[0x00:0x04]
        code = code_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
        
        # Padding: 4 bytes at +0x04 (zeros)
        padding = data[0x04:0x08]
        
        # Parameters: u16 at +0x08 and +0x0A
        param_a = struct.unpack('<H', data[0x08:0x0A])[0]
        param_b = struct.unpack('<H', data[0x0A:0x0C])[0]
        
        return CreditsEntry(
            index=index,
            offset=offset,
            raw_data=data,
            code=code,
            padding=padding,
            param_a=param_a,
            param_b=param_b,
        )
    
    def get_sector_entry_by_code(self, code: str) -> Optional[SectorTableEntry]:
        """Find a sector table entry by its level code."""
        for entry in self.sector_entries:
            if entry.code == code:
                return entry
        return None
    
    def get_sector_entry_by_index(self, index: int) -> Optional[SectorTableEntry]:
        """Get a sector table entry by its index."""
        if 0 <= index < len(self.sector_entries):
            return self.sector_entries[index]
        return None
    
    def get_level_by_index(self, index: int) -> Optional[LevelMetadataEntry]:
        """Get a level metadata entry by its index."""
        if 0 <= index < len(self.level_entries):
            return self.level_entries[index]
        return None
    
    def get_level_by_name(self, name: str) -> Optional[LevelMetadataEntry]:
        """Find a level metadata entry by its name (case-insensitive)."""
        name_lower = name.lower()
        for entry in self.level_entries:
            if entry.name.lower() == name_lower:
                return entry
        return None
    
    # Backward compatibility methods
    def get_entry_by_code(self, code: str) -> Optional[SectorTableEntry]:
        """Alias for get_sector_entry_by_code."""
        return self.get_sector_entry_by_code(code)
    
    def get_entry_by_index(self, index: int) -> Optional[SectorTableEntry]:
        """Alias for get_sector_entry_by_index."""
        return self.get_sector_entry_by_index(index)
    
    def iter_entries(self) -> Iterator[SectorTableEntry]:
        """Iterate over all sector table entries."""
        yield from self.sector_entries
    
    def iter_levels(self) -> Iterator[LevelMetadataEntry]:
        """Iterate over all level metadata entries."""
        yield from self.level_entries
    
    def __len__(self) -> int:
        return len(self.sector_entries)
    
    def __getitem__(self, index: int) -> SectorTableEntry:
        return self.sector_entries[index]
    
    def print_table(self) -> None:
        """Print a formatted table of all entries."""
        print("=" * 80)
        print("BLB Header Summary")
        print("=" * 80)
        print(f"Level count (0xF31):        {self.level_count}")
        print(f"Asset count (0xF32):        {self.asset_count}")
        print(f"Sector table count (0xF33): {self.sector_table_count}")
        print()
        
        print("=" * 80)
        print("Level Metadata Table (0x70 bytes each, at offset 0x000)")
        print("=" * 80)
        for entry in self.level_entries:
            print(f"  [{entry.index:2d}] {entry.name}")
        print()
        
        print("=" * 80)
        print("Sector Offset Table (0x10 bytes each, at offset 0xCD0)")
        print("=" * 80)
        print(f"{'Idx':>3} {'Offset':>8} {'Code':>6} {'Short':>6} "
              f"{'SecOff':>8} {'SecCnt':>8} {'ByteOff':>12} {'Size':>10}")
        print("-" * 80)
        
        for entry in self.sector_entries:
            print(
                f"{entry.index:3d} 0x{entry.offset:04X} "
                f"{entry.code:>6} {entry.short_name:>6} "
                f"0x{entry.sector_offset:04X}   0x{entry.sector_count:04X}   "
                f"0x{entry.byte_offset:08X} 0x{entry.byte_size:06X}"
            )



class BLBFile:
    """
    High-level interface for reading from a BLB file.
    
    Provides methods to read data based on header entries.
    """
    
    def __init__(self, path: Path | str):
        """
        Open a BLB file and parse its header.
        
        Args:
            path: Path to the BLB file
        """
        self.path = Path(path)
        self._file: Optional[BinaryIO] = None
        self.header = BLBHeader.from_file(self.path)
    
    def __enter__(self) -> "BLBFile":
        self._file = open(self.path, 'rb')
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        if self._file:
            self._file.close()
            self._file = None
    
    def _ensure_open(self) -> BinaryIO:
        """Ensure the file is open for reading."""
        if self._file is None:
            raise RuntimeError("BLBFile must be used as a context manager (with statement)")
        return self._file
    
    def read_entry(self, entry: BLBEntry) -> bytes:
        """
        Read the raw data for an index entry.
        
        Args:
            entry: The BLBEntry to read data for
            
        Returns:
            Raw bytes for this entry
        """
        f = self._ensure_open()
        f.seek(entry.byte_offset)
        return f.read(entry.byte_size)
    
    def read_entry_by_code(self, code: str) -> Optional[bytes]:
        """
        Read data for an entry by its level code.
        
        Args:
            code: The 4-character level code
            
        Returns:
            Raw bytes for the entry, or None if not found
        """
        entry = self.header.get_entry_by_code(code)
        if entry is None:
            return None
        return self.read_entry(entry)
    
    def read_entry_by_index(self, index: int) -> Optional[bytes]:
        """
        Read data for an entry by its index.
        
        Args:
            index: The entry index
            
        Returns:
            Raw bytes for the entry, or None if not found
        """
        entry = self.header.get_entry_by_index(index)
        if entry is None:
            return None
        return self.read_entry(entry)
    
    def read_sectors(self, sector_offset: int, sector_count: int) -> bytes:
        """
        Read raw sectors from the BLB file.
        
        Args:
            sector_offset: Starting sector number
            sector_count: Number of sectors to read
            
        Returns:
            Raw sector data
        """
        f = self._ensure_open()
        f.seek(sector_offset * SECTOR_SIZE)
        return f.read(sector_count * SECTOR_SIZE)


def main():
    """Example usage - parse and display a BLB header."""
    import sys
    
    if len(sys.argv) > 1:
        blb_path = sys.argv[1]
    else:
        # Default path for this project
        blb_path = Path(__file__).parent.parent / "disks" / "blb" / "GAME.BLB"
    
    print(f"Parsing: {blb_path}")
    print()
    
    header = BLBHeader.from_file(blb_path)
    header.print_table()
    
    print()
    print("=" * 80)
    print("Example: Reading entries by code")
    print("=" * 80)
    
    # Try to find some known level codes
    for code in ["MENU", "SC01", "SC02", "SH01"]:
        entry = header.get_entry_by_code(code)
        if entry:
            print(f"Found '{code}': {entry}")


if __name__ == "__main__":
    main()
