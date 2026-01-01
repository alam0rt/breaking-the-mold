#!/usr/bin/env python3
"""
Tests for blb.py - BLB header parsing library

Uses SHA256 hash of the BLB file to select expected values,
allowing the same tests to run against different game versions.
"""

import hashlib
import unittest
from pathlib import Path

from blb import (
    BLBHeader,
    BLBFile,
    SectorTableEntry,
    LevelMetadataEntry,
    HEADER_SIZE,
    SECTOR_SIZE,
    SECTOR_TABLE_OFFSET,
    SECTOR_TABLE_ENTRY_SIZE,
    LEVEL_METADATA_OFFSET,
    LEVEL_METADATA_ENTRY_SIZE,
    LEVEL_COUNT_OFFSET,
    ASSET_COUNT_OFFSET,
    SECTOR_TABLE_COUNT_OFFSET,
)


# Expected values keyed by SHA256 hash of the BLB file
EXPECTED_VALUES = {
    # PAL (SLES-01090) / NTSC-U (SLUS-00601)
    "58ed7ec2aee9a5666d2b9db04523cbf3d527b57dfbc8a52c3460f4da91c3f09e": {
        "level_count": 26,
        "asset_count": 13,
        "sector_table_count": 32,
        "first_level_name": "Options",
        "last_level_name": "Evil Engine #9",
        "first_sector_entry": {
            "code": "PIRA",
            "short_name": "Don",
            "sector_offset": 0x000C,
            "sector_count": 0x000A,
        },
        "menu_entry": {
            "index": 3,
            "code": "MENU",
            "short_name": "Opt",
            "sector_offset": 0x0819,
            "sector_count": 0x000A,
        },
        "last_sector_entry": {
            "index": 31,
            "code": "OVER",
            "sector_offset": 0x131F,
            "sector_count": 0x0076,
        },
    },
    # PAL German (SLES-01091)
    "0458ce72c72f5e85352b03b9dd3f664a1bd2a87aecde8fb52149d87a6fb1b7ee": {
        "level_count": 26,
        "asset_count": 13,
        "sector_table_count": 32,
        "first_level_name": "Options",
        "last_level_name": "Evil Engine #9",
        "first_sector_entry": {
            "code": "PIRA",
            "short_name": "Don",
            "sector_offset": 0x000C,
            "sector_count": 0x0009,
        },
        "menu_entry": {
            "index": 3,
            "code": "MENU",
            "short_name": "Opt",
            "sector_offset": 0x0829,
            "sector_count": 0x000A,
        },
        "last_sector_entry": {
            "index": 31,
            "code": "OVER",
            "sector_offset": 0x1349,
            "sector_count": 0x0075,
        },
    },
    # PAL French (SLES-01092)
    "ede0144aabbde758b41ca32d6cd796ddc508403ee1daf403a9b21377b761c0b6": {
        "level_count": 26,
        "asset_count": 13,
        "sector_table_count": 32,
        "first_level_name": "Options",
        "last_level_name": "Evil Engine #9",
        "first_sector_entry": {
            "code": "PIRA",
            "short_name": "Don",
            "sector_offset": 0x000C,
            "sector_count": 0x000A,
        },
        "menu_entry": {
            "index": 3,
            "code": "MENU",
            "short_name": "Opt",
            "sector_offset": 0x082B,
            "sector_count": 0x000A,
        },
        "last_sector_entry": {
            "index": 31,
            "code": "OVER",
            "sector_offset": 0x1348,
            "sector_count": 0x0076,
        },
    },
    # NTSC-J (SLPS-01501) - Demo, only 6 levels, different sector table format
    # Note: Demo uses binary indices in code field, not ASCII codes
    "efa18f96ac823a053234a009efcf20142280e44f227badf80e04ddc7a1762409": {
        "level_count": 6,
        "asset_count": 1,
        "sector_table_count": 4,
        "first_level_name": "Options",
        "last_level_name": "Shriney Guard",
        "first_sector_entry": {
            "code": "",  # All nulls stripped
            "short_name": "ENU",
            "sector_offset": 0x704F,
            "sector_count": 0x0074,
        },
        "menu_entry": None,  # No MENU entry in demo
        "last_sector_entry": {
            "index": 3,
            "code": "\x00\x03",  # Binary index with trailing nulls stripped
            "sector_offset": 0x6F4D,
            "sector_count": 0x006E,
        },
    },
    # NTSC-J Demo (PAPX-90053) - Demo, only 6 levels, different sector table format
    # Note: This file appears to be truncated - sector offsets point beyond file size
    "20e996654c94a88af4159786935c3f28f541d29f6fcc82b2c7842437a1d13aa0": {
        "level_count": 6,
        "asset_count": 1,
        "sector_table_count": 4,
        "first_level_name": "Options",
        "last_level_name": "Shriney Guard",
        "first_sector_entry": {
            "code": "",  # All nulls stripped
            "short_name": "ENU",
            "sector_offset": 0x704F,
            "sector_count": 0x0074,
        },
        "menu_entry": None,  # No MENU entry in demo
        "last_sector_entry": {
            "index": 3,
            "code": "\x00\x03",  # Binary index with trailing nulls stripped
            "sector_offset": 0x6F4D,
            "sector_count": 0x006E,
        },
        "is_truncated": True,  # Sector data not available in this file
    },
    # Beta
    "bab772667336b3b3b92352045746f545729cb4cbc1e8a53b33b593bad049828e": {
        "level_count": 26,
        "asset_count": 13,
        "sector_table_count": 32,
        "first_level_name": "Options",
        "last_level_name": "Evil Engine #9",
        "first_sector_entry": {
            "code": "PIRA",
            "short_name": "Don",
            "sector_offset": 0x000C,
            "sector_count": 0x0009,
        },
        "menu_entry": {
            "index": 3,
            "code": "MENU",
            "short_name": "Opt",
            "sector_offset": 0x0818,
            "sector_count": 0x000A,
        },
        "last_sector_entry": {
            "index": 31,
            "code": "OVER",
            "sector_offset": 0x131E,
            "sector_count": 0x0076,
        },
    },
}

# Default BLB path - can be overridden via environment variable
DEFAULT_BLB_PATH = Path(__file__).parent.parent / "disks" / "blb" / "GAME.BLB"


def get_blb_hash(path: Path) -> str:
    """Calculate SHA256 hash of a BLB file."""
    sha256 = hashlib.sha256()
    with open(path, "rb") as f:
        # Read in chunks to handle large files
        for chunk in iter(lambda: f.read(65536), b""):
            sha256.update(chunk)
    return sha256.hexdigest()


class TestBLBHeader(unittest.TestCase):
    """Tests for BLB header parsing."""
    
    @classmethod
    def setUpClass(cls):
        """Load the BLB file and determine which expected values to use."""
        import os
        cls.blb_path = Path(os.environ.get("BLB_PATH", DEFAULT_BLB_PATH))
        
        if not cls.blb_path.exists():
            raise unittest.SkipTest(f"BLB file not found: {cls.blb_path}")
        
        cls.blb_hash = get_blb_hash(cls.blb_path)
        
        if cls.blb_hash not in EXPECTED_VALUES:
            raise unittest.SkipTest(
                f"Unknown BLB version (hash: {cls.blb_hash}). "
                "Add expected values to EXPECTED_VALUES dict."
            )
        
        cls.expected = EXPECTED_VALUES[cls.blb_hash]
        cls.header = BLBHeader.from_file(cls.blb_path)
        
        print(f"\nTesting with BLB: {cls.blb_path}")
        print(f"SHA256: {cls.blb_hash}")
    
    # -------------------------------------------------------------------------
    # Header count tests
    # -------------------------------------------------------------------------
    
    def test_header_counts(self):
        """Verify the header count fields match expected values."""
        self.assertEqual(
            self.header.level_count,
            self.expected["level_count"],
            "Level count (0xF31) mismatch"
        )
        self.assertEqual(
            self.header.asset_count,
            self.expected["asset_count"],
            "Asset count (0xF32) mismatch"
        )
        self.assertEqual(
            self.header.sector_table_count,
            self.expected["sector_table_count"],
            "Sector table count (0xF33) mismatch"
        )
    
    def test_parsed_entry_counts(self):
        """Verify correct number of entries were parsed."""
        self.assertEqual(
            len(self.header.level_entries),
            self.expected["level_count"],
            "Number of parsed level entries should match level_count"
        )
        self.assertEqual(
            len(self.header.sector_entries),
            self.expected["sector_table_count"],
            "Number of parsed sector entries should match sector_table_count"
        )
    
    # -------------------------------------------------------------------------
    # Level metadata tests
    # -------------------------------------------------------------------------
    
    def test_first_level_entry(self):
        """Verify first level metadata entry."""
        entry = self.header.level_entries[0]
        self.assertIsInstance(entry, LevelMetadataEntry)
        self.assertEqual(entry.index, 0)
        self.assertEqual(entry.name, self.expected["first_level_name"])
    
    def test_last_level_entry(self):
        """Verify last level metadata entry."""
        entry = self.header.level_entries[-1]
        self.assertIsInstance(entry, LevelMetadataEntry)
        self.assertEqual(entry.index, self.expected["level_count"] - 1)
        self.assertEqual(entry.name, self.expected["last_level_name"])
    
    def test_get_level_by_index(self):
        """Test get_level_by_index method."""
        entry = self.header.get_level_by_index(0)
        self.assertIsNotNone(entry)
        self.assertEqual(entry.name, self.expected["first_level_name"])
        
        # Out of bounds should return None
        self.assertIsNone(self.header.get_level_by_index(-1))
        self.assertIsNone(self.header.get_level_by_index(1000))
    
    def test_get_level_by_name(self):
        """Test get_level_by_name method (case-insensitive)."""
        entry = self.header.get_level_by_name(self.expected["first_level_name"])
        self.assertIsNotNone(entry)
        self.assertEqual(entry.index, 0)
        
        # Case-insensitive
        entry_lower = self.header.get_level_by_name(
            self.expected["first_level_name"].lower()
        )
        self.assertIsNotNone(entry_lower)
        self.assertEqual(entry_lower.index, 0)
        
        # Non-existent should return None
        self.assertIsNone(self.header.get_level_by_name("NONEXISTENT_LEVEL"))
    
    # -------------------------------------------------------------------------
    # Sector table tests
    # -------------------------------------------------------------------------
    
    def test_first_sector_entry(self):
        """Verify first sector table entry."""
        entry = self.header.sector_entries[0]
        expected = self.expected["first_sector_entry"]
        
        self.assertIsInstance(entry, SectorTableEntry)
        self.assertEqual(entry.index, 0)
        self.assertEqual(entry.code, expected["code"])
        self.assertEqual(entry.short_name, expected["short_name"])
        self.assertEqual(entry.sector_offset, expected["sector_offset"])
        self.assertEqual(entry.sector_count, expected["sector_count"])
    
    def test_last_sector_entry(self):
        """Verify last sector table entry."""
        entry = self.header.sector_entries[-1]
        expected = self.expected["last_sector_entry"]
        
        self.assertEqual(entry.index, expected["index"])
        self.assertEqual(entry.code, expected["code"])
        self.assertEqual(entry.sector_offset, expected["sector_offset"])
        self.assertEqual(entry.sector_count, expected["sector_count"])
    
    def test_get_sector_entry_by_code(self):
        """Test get_sector_entry_by_code method."""
        expected = self.expected["menu_entry"]
        
        if expected is not None:
            # Full version with MENU entry
            entry = self.header.get_sector_entry_by_code("MENU")
            self.assertIsNotNone(entry)
            self.assertEqual(entry.index, expected["index"])
            self.assertEqual(entry.code, expected["code"])
            self.assertEqual(entry.sector_offset, expected["sector_offset"])
        else:
            # Demo version without MENU entry
            entry = self.header.get_sector_entry_by_code("MENU")
            self.assertIsNone(entry)
        
        # Non-existent should return None
        self.assertIsNone(self.header.get_sector_entry_by_code("XXXX"))
    
    def test_get_sector_entry_by_index(self):
        """Test get_sector_entry_by_index method."""
        entry = self.header.get_sector_entry_by_index(0)
        self.assertIsNotNone(entry)
        self.assertEqual(entry.code, self.expected["first_sector_entry"]["code"])
        
        # Out of bounds should return None
        self.assertIsNone(self.header.get_sector_entry_by_index(-1))
        self.assertIsNone(self.header.get_sector_entry_by_index(1000))
    
    def test_sector_entry_byte_calculations(self):
        """Test byte_offset and byte_size properties."""
        entry = self.header.sector_entries[0]
        expected = self.expected["first_sector_entry"]
        
        self.assertEqual(
            entry.byte_offset,
            expected["sector_offset"] * SECTOR_SIZE
        )
        self.assertEqual(
            entry.byte_size,
            expected["sector_count"] * SECTOR_SIZE
        )
    
    # -------------------------------------------------------------------------
    # Backward compatibility tests
    # -------------------------------------------------------------------------
    
    def test_backward_compat_entries_property(self):
        """Test that 'entries' property returns sector_entries."""
        self.assertIs(self.header.entries, self.header.sector_entries)
    
    def test_backward_compat_get_entry_methods(self):
        """Test backward-compatible get_entry_by_* methods."""
        # get_entry_by_code should work like get_sector_entry_by_code
        entry1 = self.header.get_entry_by_code("MENU")
        entry2 = self.header.get_sector_entry_by_code("MENU")
        self.assertEqual(entry1, entry2)
        
        # get_entry_by_index should work like get_sector_entry_by_index
        entry1 = self.header.get_entry_by_index(0)
        entry2 = self.header.get_sector_entry_by_index(0)
        self.assertEqual(entry1, entry2)
    
    def test_len_and_getitem(self):
        """Test __len__ and __getitem__ on header."""
        self.assertEqual(len(self.header), self.expected["sector_table_count"])
        self.assertEqual(self.header[0], self.header.sector_entries[0])
    
    # -------------------------------------------------------------------------
    # Iterator tests
    # -------------------------------------------------------------------------
    
    def test_iter_entries(self):
        """Test iter_entries generator."""
        entries = list(self.header.iter_entries())
        self.assertEqual(len(entries), self.expected["sector_table_count"])
        self.assertEqual(entries[0], self.header.sector_entries[0])
    
    def test_iter_levels(self):
        """Test iter_levels generator."""
        levels = list(self.header.iter_levels())
        self.assertEqual(len(levels), self.expected["level_count"])
        self.assertEqual(levels[0], self.header.level_entries[0])
    
    # -------------------------------------------------------------------------
    # Raw data tests
    # -------------------------------------------------------------------------
    
    def test_raw_data_size(self):
        """Verify raw_data is correct size."""
        self.assertEqual(len(self.header.raw_data), HEADER_SIZE)
    
    def test_level_entry_raw_data(self):
        """Verify level entries have raw_data of correct size."""
        for entry in self.header.level_entries:
            self.assertEqual(len(entry.raw_data), LEVEL_METADATA_ENTRY_SIZE)


class TestBLBFile(unittest.TestCase):
    """Tests for BLBFile class."""
    
    @classmethod
    def setUpClass(cls):
        """Load the BLB file."""
        import os
        cls.blb_path = Path(os.environ.get("BLB_PATH", DEFAULT_BLB_PATH))
        
        if not cls.blb_path.exists():
            raise unittest.SkipTest(f"BLB file not found: {cls.blb_path}")
        
        cls.blb_hash = get_blb_hash(cls.blb_path)
        
        if cls.blb_hash not in EXPECTED_VALUES:
            raise unittest.SkipTest(f"Unknown BLB version (hash: {cls.blb_hash})")
        
        cls.expected = EXPECTED_VALUES[cls.blb_hash]
    
    def test_context_manager(self):
        """Test BLBFile works as context manager."""
        with BLBFile(self.blb_path) as blb:
            self.assertIsNotNone(blb.header)
            self.assertEqual(
                blb.header.level_count,
                self.expected["level_count"]
            )
    
    def test_read_entry(self):
        """Test reading entry data."""
        if self.expected.get("is_truncated"):
            self.skipTest("File is truncated - sector data not available")
        
        with BLBFile(self.blb_path) as blb:
            # Use first entry (always exists)
            entry = blb.header.sector_entries[0]
            data = blb.read_entry(entry)
            
            self.assertEqual(len(data), entry.byte_size)
    
    def test_read_entry_by_code(self):
        """Test read_entry_by_code method."""
        expected = self.expected["menu_entry"]
        
        with BLBFile(self.blb_path) as blb:
            if expected is not None:
                # Full version with MENU entry
                if self.expected.get("is_truncated"):
                    self.skipTest("File is truncated - sector data not available")
                data = blb.read_entry_by_code("MENU")
                self.assertIsNotNone(data)
                self.assertEqual(
                    len(data),
                    expected["sector_count"] * SECTOR_SIZE
                )
            else:
                # Demo version without MENU entry
                data = blb.read_entry_by_code("MENU")
                self.assertIsNone(data)
            
            # Non-existent should return None
            self.assertIsNone(blb.read_entry_by_code("XXXX"))
    
    def test_read_entry_by_index(self):
        """Test read_entry_by_index method."""
        if self.expected.get("is_truncated"):
            self.skipTest("File is truncated - sector data not available")
        
        with BLBFile(self.blb_path) as blb:
            data = blb.read_entry_by_index(0)
            expected = self.expected["first_sector_entry"]
            
            self.assertIsNotNone(data)
            self.assertEqual(
                len(data),
                expected["sector_count"] * SECTOR_SIZE
            )
            
            # Out of bounds should return None
            self.assertIsNone(blb.read_entry_by_index(1000))
    
    def test_read_sectors(self):
        """Test read_sectors method."""
        with BLBFile(self.blb_path) as blb:
            data = blb.read_sectors(0, 2)
            
            self.assertEqual(len(data), 2 * SECTOR_SIZE)
            # First 2 sectors should match header raw_data
            self.assertEqual(data, blb.header.raw_data)
    
    def test_error_without_context_manager(self):
        """Test that reading without context manager raises error."""
        blb = BLBFile(self.blb_path)
        
        # Use read_entry_by_index which always has data (index 0 always exists)
        with self.assertRaises(RuntimeError):
            blb.read_entry_by_index(0)


class TestBLBHeaderFromBytes(unittest.TestCase):
    """Tests for BLBHeader.from_bytes parsing."""
    
    @classmethod
    def setUpClass(cls):
        """Load raw header bytes."""
        import os
        cls.blb_path = Path(os.environ.get("BLB_PATH", DEFAULT_BLB_PATH))
        
        if not cls.blb_path.exists():
            raise unittest.SkipTest(f"BLB file not found: {cls.blb_path}")
        
        with open(cls.blb_path, "rb") as f:
            cls.header_bytes = f.read(HEADER_SIZE)
    
    def test_from_bytes(self):
        """Test parsing header from bytes."""
        header = BLBHeader.from_bytes(self.header_bytes)
        self.assertIsNotNone(header)
        self.assertGreater(len(header.sector_entries), 0)
        self.assertGreater(len(header.level_entries), 0)
    
    def test_from_bytes_too_short(self):
        """Test that too-short data raises ValueError."""
        with self.assertRaises(ValueError) as ctx:
            BLBHeader.from_bytes(b"too short")
        
        self.assertIn("too short", str(ctx.exception).lower())


if __name__ == "__main__":
    unittest.main(verbosity=2)
