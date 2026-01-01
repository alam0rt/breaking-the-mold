#!/usr/bin/env python3
"""
blb.py - Library for parsing Skullmonkeys GAME.BLB archive headers

The BLB (blob) file is the main game archive containing all levels, assets,
and resources. The first 2 sectors (4096 bytes) contain the header with an
index table describing the contents.

Header Structure:
- Offset 0x000: Level metadata table (0x70 bytes per entry)
  - Each entry contains level display info, with the name at +0x5B
- Offset 0xCD0: Sector offset table (0x10 bytes per entry)
  - Contains BLB sector locations for loading data
- Offset 0xF31: Level count (u8) - used by func_8007A9B0
- Offset 0xF32: Asset count (u8) - used by func_8007ACDC
- Offset 0xF33: Sector table entry count (u8) - total entries in sector table

Sector Table Entry Format (16 bytes each, at offset 0xCD0):
- +0x00: u16 - Entry type/flags
- +0x02: u8  - Unknown byte
- +0x03: 5 bytes - Level code (4-char null-terminated, e.g., "PIRA", "MENU")
- +0x08: 4 bytes - Short name (truncated description)
- +0x0C: u16 - Sector offset within BLB file
- +0x0E: u16 - Sector count

Level Metadata Entry Format (0x70 bytes each, at offset 0x000):
- +0x00: 0x5B bytes - Various level metadata (TBD)
- +0x5B: ~20 bytes - Level name, null-terminated (e.g., "Options", "Skullmonkey Gate")

Sector size is standard CD-ROM: 2048 bytes.
"""

import struct
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO, Iterator, Optional

# Constants
SECTOR_SIZE = 2048
HEADER_SIZE = 2 * SECTOR_SIZE  # First 2 sectors (4096 bytes)

# Level metadata table (0x70 bytes per entry, at start of header)
LEVEL_METADATA_OFFSET = 0x000
LEVEL_METADATA_ENTRY_SIZE = 0x70
LEVEL_NAME_OFFSET = 0x5B  # Offset of name within each level metadata entry

# Sector offset table (0x10 bytes per entry)
SECTOR_TABLE_OFFSET = 0xCD0
SECTOR_TABLE_ENTRY_SIZE = 0x10

# Header count fields
LEVEL_COUNT_OFFSET = 0xF31  # Number of level entries (used by func_8007A9B0)
ASSET_COUNT_OFFSET = 0xF32  # Number of asset entries (used by func_8007ACDC)
SECTOR_TABLE_COUNT_OFFSET = 0xF33  # Total sector table entries


@dataclass
class SectorTableEntry:
    """
    Represents a single entry in the BLB sector offset table.
    
    Located at header offset 0xCD0, with 0x10 bytes per entry.
    The number of entries is stored at header offset 0xF33.
    """
    
    index: int
    offset: int  # Offset of this entry within the header
    entry_type: int  # u16 at +0x00, possibly type/flags
    unknown_byte: int  # u8 at +0x02
    code: str  # 4-char level code at +0x03 (e.g., "PIRA", "MENU")
    short_name: str  # Short name at +0x08 (truncated description)
    sector_offset: int  # Sector offset within BLB file
    sector_count: int  # Number of sectors
    
    @property
    def byte_offset(self) -> int:
        """Calculate the byte offset within the BLB file."""
        return self.sector_offset * SECTOR_SIZE
    
    @property
    def byte_size(self) -> int:
        """Calculate the size in bytes."""
        return self.sector_count * SECTOR_SIZE
    
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
    """
    
    index: int
    offset: int  # Offset of this entry within the header
    raw_data: bytes  # Full 0x70 bytes of entry data
    name: str  # Level name at +0x5B (null-terminated)
    
    def __repr__(self) -> str:
        return f"LevelMetadataEntry(idx={self.index}, name='{self.name}')"


# Keep BLBEntry as an alias for backward compatibility
BLBEntry = SectorTableEntry


@dataclass
class BLBHeader:
    """
    Represents the BLB file header.
    
    The header occupies the first 2 sectors (4096 bytes) and contains:
    - Level metadata table at 0x000 (0x70 bytes per entry)
    - Sector offset table at 0xCD0 (0x10 bytes per entry)
    - Count fields at 0xF31-0xF33
    """
    
    raw_data: bytes  # Raw header data (first 4096 bytes)
    sector_entries: list[SectorTableEntry]  # Sector offset table entries
    level_entries: list[LevelMetadataEntry]  # Level metadata entries
    level_count: int  # Number of levels (from offset 0xF31)
    asset_count: int  # Number of assets (from offset 0xF32)
    sector_table_count: int  # Total sector table entries (from offset 0xF33)
    
    # Backward compatibility property
    @property
    def entries(self) -> list[SectorTableEntry]:
        """Alias for sector_entries for backward compatibility."""
        return self.sector_entries
    
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
        
        return cls(
            raw_data=data[:HEADER_SIZE],
            sector_entries=sector_entries,
            level_entries=level_entries,
            level_count=level_count,
            asset_count=asset_count,
            sector_table_count=sector_table_count,
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
        entry_type = struct.unpack('<H', data[0:2])[0]
        unknown_byte = data[2]
        
        # Level code: 4 chars starting at offset 3, null-terminated
        code_bytes = data[3:7]
        code = code_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
        
        # Short name: 4 chars at offset 8
        short_bytes = data[8:12]
        short_name = short_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
        
        sector_offset = struct.unpack('<H', data[12:14])[0]
        sector_count = struct.unpack('<H', data[14:16])[0]
        
        return SectorTableEntry(
            index=index,
            offset=offset,
            entry_type=entry_type,
            unknown_byte=unknown_byte,
            code=code,
            short_name=short_name,
            sector_offset=sector_offset,
            sector_count=sector_count,
        )
    
    @staticmethod
    def _parse_level_entry(index: int, offset: int, data: bytes) -> LevelMetadataEntry:
        """Parse a single 0x70-byte level metadata entry."""
        # Name is at offset 0x5B within the entry, null-terminated
        name_bytes = data[LEVEL_NAME_OFFSET:LEVEL_NAME_OFFSET + 20]
        name = name_bytes.split(b'\x00')[0].decode('ascii', errors='replace')
        
        return LevelMetadataEntry(
            index=index,
            offset=offset,
            raw_data=data,
            name=name,
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
