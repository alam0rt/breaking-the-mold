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
    MovieEntry,
    StateData,
    HEADER_SIZE,
    SECTOR_SIZE,
    SECTOR_TABLE_OFFSET,
    SECTOR_TABLE_ENTRY_SIZE,
    LEVEL_METADATA_OFFSET,
    LEVEL_METADATA_ENTRY_SIZE,
    LEVEL_COUNT_OFFSET,
    ASSET_COUNT_OFFSET,
    SECTOR_TABLE_COUNT_OFFSET,
    is_unknown_field,
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
        # First level metadata entry details
        "first_level_entry": {
            "index": 0,
            "level_id": "MENU",
            "name": "Options",
            "sector_offset": 0x00C9,
            "sector_count": 0x0020,
            "secondary_offset": 0x00E9,
            "secondary_count": 0x0043,
            "tertiary_offset": 0x03D5,
            "tertiary_count": 0x0002,
            "level_index": 0,
        },
        # Last level metadata entry
        "last_level_entry": {
            "index": 25,
            "level_id": "EVIL",
            "name": "Evil Engine #9",
            "level_index": 25,
        },
        # First sector table entry (PIRA - special loading screen)
        "first_sector_entry": {
            "code": "PIRA",
            "short_name": "Don",
            "sector_offset": 0x000C,
            "sector_count": 0x000A,
            "level_index": 0x37,
            "entry_flags": 0x05,
            "unknown_byte": 0x0A,
        },
        # MENU sector entry (standard level loading screen)
        "menu_entry": {
            "index": 3,
            "code": "MENU",
            "short_name": "Opt",
            "sector_offset": 0x0819,
            "sector_count": 0x000A,
            "level_index": 0x00,
            "entry_flags": 0x00,
            "unknown_byte": 0x0A,
        },
        # Last sector entry (OVER - game over screen)
        "last_sector_entry": {
            "index": 31,
            "code": "OVER",
            "sector_offset": 0x131F,
            "sector_count": 0x0076,
            "level_index": 0x36,
            "entry_flags": 0x03,
            "unknown_byte": 0x63,  # 'c' for game over
        },
        # Credits sector entry (first one at index 2)
        "credits_entry": {
            "index": 2,
            "code": "CRED",
            "level_index": 0x35,
            "entry_flags": 0x00,
        },
        # First movie entry
        "first_movie_entry": {
            "index": 0,
            "movie_id": "DREA",
            "short_name": "DW",
            "filename": "\\MVDWI.STR;1",
            "sector_count": 0x004F,
        },
        # Last movie entry
        "last_movie_entry": {
            "index": 12,
            "movie_id": "END2",
            "filename": "\\MVWIN.STR;1",
        },
        # State/config data at 0xF34
        "state_data": {
            "game_mode": 5,
            "secondary_flag": 5,
            "unknown_f91": 0,
            "asset_index": 0,
        },
    },
    # PAL German (SLES-01091)
    "0458ce72c72f5e85352b03b9dd3f664a1bd2a87aecde8fb52149d87a6fb1b7ee": {
        "level_count": 26,
        "asset_count": 13,
        "sector_table_count": 32,
        "first_level_name": "Options",
        "last_level_name": "Evil Engine #9",
        "first_level_entry": {
            "index": 0,
            "level_id": "MENU",
            "name": "Options",
            "level_index": 0,
        },
        "last_level_entry": {
            "index": 25,
            "level_id": "EVIL",
            "name": "Evil Engine #9",
            "level_index": 25,
        },
        "first_sector_entry": {
            "code": "PIRA",
            "short_name": "Don",
            "sector_offset": 0x000C,
            "sector_count": 0x0009,
            "level_index": 0x37,
            "entry_flags": 0x05,
            "unknown_byte": 0x0A,
        },
        "menu_entry": {
            "index": 3,
            "code": "MENU",
            "short_name": "Opt",
            "sector_offset": 0x0829,
            "sector_count": 0x000A,
            "level_index": 0x00,
            "entry_flags": 0x00,
            "unknown_byte": 0x0A,
        },
        "last_sector_entry": {
            "index": 31,
            "code": "OVER",
            "sector_offset": 0x1349,
            "sector_count": 0x0075,
            "level_index": 0x36,
            "entry_flags": 0x03,
            "unknown_byte": 0x63,
        },
        "credits_entry": {
            "index": 2,
            "code": "CRED",
            "level_index": 0x35,
            "entry_flags": 0x00,
        },
        "first_movie_entry": {
            "index": 0,
            "movie_id": "DREA",
            "short_name": "DW",
            "filename": "\\MVDWI.STR;1",
        },
        "last_movie_entry": {
            "index": 12,
            "movie_id": "END2",
            "filename": "\\MVWIN.STR;1",
        },
        # State/config data at 0xF34
        "state_data": {
            "game_mode": 5,
            "secondary_flag": 5,
            "unknown_f91": 0,
            "asset_index": 0,
        },
    },
    # PAL French (SLES-01092)
    "ede0144aabbde758b41ca32d6cd796ddc508403ee1daf403a9b21377b761c0b6": {
        "level_count": 26,
        "asset_count": 13,
        "sector_table_count": 32,
        "first_level_name": "Options",
        "last_level_name": "Evil Engine #9",
        "first_level_entry": {
            "index": 0,
            "level_id": "MENU",
            "name": "Options",
            "level_index": 0,
        },
        "last_level_entry": {
            "index": 25,
            "level_id": "EVIL",
            "name": "Evil Engine #9",
            "level_index": 25,
        },
        "first_sector_entry": {
            "code": "PIRA",
            "short_name": "Don",
            "sector_offset": 0x000C,
            "sector_count": 0x000A,
            "level_index": 0x37,
            "entry_flags": 0x05,
            "unknown_byte": 0x0A,
        },
        "menu_entry": {
            "index": 3,
            "code": "MENU",
            "short_name": "Opt",
            "sector_offset": 0x082B,
            "sector_count": 0x000A,
            "level_index": 0x00,
            "entry_flags": 0x00,
            "unknown_byte": 0x0A,
        },
        "last_sector_entry": {
            "index": 31,
            "code": "OVER",
            "sector_offset": 0x1348,
            "sector_count": 0x0076,
            "level_index": 0x36,
            "entry_flags": 0x03,
            "unknown_byte": 0x63,
        },
        "credits_entry": {
            "index": 2,
            "code": "CRED",
            "level_index": 0x35,
            "entry_flags": 0x00,
        },
        "first_movie_entry": {
            "index": 0,
            "movie_id": "DREA",
            "short_name": "DW",
            "filename": "\\MVDWI.STR;1",
        },
        "last_movie_entry": {
            "index": 12,
            "movie_id": "END2",
            "filename": "\\MVWIN.STR;1",
        },
        # State/config data at 0xF34
        "state_data": {
            "game_mode": 5,
            "secondary_flag": 5,
            "unknown_f91": 0,
            "asset_index": 0,
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
        # State/config data at 0xF34 - Japanese has different values
        "state_data": {
            "game_mode": 4,
            "secondary_flag": 3,
            "unknown_f91": 7,
            "asset_index": 3,
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
        # State/config data at 0xF34 - PAPX demo values
        "state_data": {
            "game_mode": 5,
            "secondary_flag": 5,
            "unknown_f91": 7,
            "asset_index": 3,
        },
    },
    # Beta
    "bab772667336b3b3b92352045746f545729cb4cbc1e8a53b33b593bad049828e": {
        "level_count": 26,
        "asset_count": 13,
        "sector_table_count": 32,
        "first_level_name": "Options",
        "last_level_name": "Evil Engine #9",
        "first_level_entry": {
            "index": 0,
            "level_id": "MENU",
            "name": "Options",
            "level_index": 0,
        },
        "last_level_entry": {
            "index": 25,
            "level_id": "EVIL",
            "name": "Evil Engine #9",
            "level_index": 25,
        },
        "first_sector_entry": {
            "code": "PIRA",
            "short_name": "Don",
            "sector_offset": 0x000C,
            "sector_count": 0x0009,
            "level_index": 0x37,
            "entry_flags": 0x05,
            "unknown_byte": 0x0A,
        },
        "menu_entry": {
            "index": 3,
            "code": "MENU",
            "short_name": "Opt",
            "sector_offset": 0x0818,
            "sector_count": 0x000A,
            "level_index": 0x00,
            "entry_flags": 0x00,
            "unknown_byte": 0x0A,
        },
        "last_sector_entry": {
            "index": 31,
            "code": "OVER",
            "sector_offset": 0x131E,
            "sector_count": 0x0076,
            "level_index": 0x36,
            "entry_flags": 0x03,
            "unknown_byte": 0x63,
        },
        "credits_entry": {
            "index": 2,
            "code": "CRED",
            "level_index": 0x35,
            "entry_flags": 0x00,
        },
        "first_movie_entry": {
            "index": 0,
            "movie_id": "DREA",
            "short_name": "DW",
            "filename": "\\MVDWI.STR;1",
        },
        "last_movie_entry": {
            "index": 12,
            "movie_id": "END2",
            "filename": "\\MVWIN.STR;1",
        },
        # State/config data at 0xF34 - Beta has same values as release
        "state_data": {
            "game_mode": 5,
            "secondary_flag": 5,
            "unknown_f91": 0,
            "asset_index": 0,
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
    
    def test_sector_entry_new_fields(self):
        """Test level_index, entry_flags, unknown_byte fields."""
        entry = self.header.sector_entries[0]
        expected = self.expected["first_sector_entry"]
        
        if "level_index" in expected:
            self.assertEqual(entry.level_index, expected["level_index"])
        if "entry_flags" in expected:
            self.assertEqual(entry.entry_flags, expected["entry_flags"])
        if "unknown_byte" in expected:
            self.assertEqual(entry.unknown_byte, expected["unknown_byte"])
    
    def test_credits_entry(self):
        """Test credits entry identification."""
        expected = self.expected.get("credits_entry")
        if expected is None:
            self.skipTest("No credits entry in this BLB version")
        
        entry = self.header.get_sector_entry_by_code(expected["code"])
        self.assertIsNotNone(entry)
        self.assertEqual(entry.index, expected["index"])
        self.assertEqual(entry.level_index, expected["level_index"])
        self.assertEqual(entry.entry_flags, expected["entry_flags"])
        self.assertTrue(entry.is_credits)
    
    # -------------------------------------------------------------------------
    # Level metadata detailed tests
    # -------------------------------------------------------------------------
    
    def test_first_level_entry_details(self):
        """Verify first level metadata entry with full details."""
        expected = self.expected.get("first_level_entry")
        if expected is None:
            self.skipTest("No first_level_entry in this BLB version")
        
        entry = self.header.level_entries[0]
        self.assertEqual(entry.index, expected["index"])
        self.assertEqual(entry.level_id, expected["level_id"])
        self.assertEqual(entry.name, expected["name"])
        
        if "sector_offset" in expected:
            self.assertEqual(entry.sector_offset, expected["sector_offset"])
        if "sector_count" in expected:
            self.assertEqual(entry.sector_count, expected["sector_count"])
        if "secondary_offset" in expected:
            self.assertEqual(entry.secondary_offset, expected["secondary_offset"])
        if "secondary_count" in expected:
            self.assertEqual(entry.secondary_count, expected["secondary_count"])
        if "tertiary_offset" in expected:
            self.assertEqual(entry.tertiary_offset, expected["tertiary_offset"])
        if "tertiary_count" in expected:
            self.assertEqual(entry.tertiary_count, expected["tertiary_count"])
        if "level_index" in expected:
            self.assertEqual(entry.level_index, expected["level_index"])
    
    def test_last_level_entry_details(self):
        """Verify last level metadata entry with full details."""
        expected = self.expected.get("last_level_entry")
        if expected is None:
            self.skipTest("No last_level_entry in this BLB version")
        
        entry = self.header.level_entries[-1]
        self.assertEqual(entry.index, expected["index"])
        self.assertEqual(entry.level_id, expected["level_id"])
        self.assertEqual(entry.name, expected["name"])
        if "level_index" in expected:
            self.assertEqual(entry.level_index, expected["level_index"])
    
    # -------------------------------------------------------------------------
    # Movie entry tests
    # -------------------------------------------------------------------------
    
    def test_first_movie_entry(self):
        """Verify first movie entry."""
        expected = self.expected.get("first_movie_entry")
        if expected is None:
            self.skipTest("No first_movie_entry in this BLB version")
        
        if len(self.header.movie_entries) == 0:
            self.skipTest("No movie entries in header")
        
        entry = self.header.movie_entries[0]
        self.assertEqual(entry.index, expected["index"])
        self.assertEqual(entry.movie_id, expected["movie_id"])
        if "short_name" in expected:
            self.assertEqual(entry.short_name, expected["short_name"])
        if "filename" in expected:
            self.assertEqual(entry.filename, expected["filename"])
        if "sector_count" in expected:
            self.assertEqual(entry.sector_count, expected["sector_count"])
    
    def test_last_movie_entry(self):
        """Verify last movie entry."""
        expected = self.expected.get("last_movie_entry")
        if expected is None:
            self.skipTest("No last_movie_entry in this BLB version")
        
        if len(self.header.movie_entries) == 0:
            self.skipTest("No movie entries in header")
        
        entry = self.header.movie_entries[-1]
        self.assertEqual(entry.index, expected["index"])
        self.assertEqual(entry.movie_id, expected["movie_id"])
        if "filename" in expected:
            self.assertEqual(entry.filename, expected["filename"])
    
    def test_movie_count_matches_asset_count(self):
        """Verify movie entry count matches asset_count."""
        self.assertEqual(
            len(self.header.movie_entries),
            self.expected["asset_count"],
            "Movie entry count should match asset_count"
        )
    
    # -------------------------------------------------------------------------
    # State data tests
    # -------------------------------------------------------------------------
    
    def test_state_data_exists(self):
        """Verify state_data is parsed."""
        self.assertIsNotNone(self.header.state_data)
        self.assertIsInstance(self.header.state_data, StateData)
    
    def test_state_data_raw_data_size(self):
        """Verify state_data raw_data is 204 bytes."""
        self.assertEqual(len(self.header.state_data.raw_data), 204)
    
    def test_state_data_game_mode(self):
        """Verify state_data game_mode field."""
        if "state_data" not in self.expected:
            self.skipTest("No state_data expected values for this version")
        expected = self.expected["state_data"]
        self.assertEqual(
            self.header.state_data.game_mode,
            expected["game_mode"],
            "game_mode should match expected value"
        )
    
    def test_state_data_secondary_flag(self):
        """Verify state_data secondary_flag field."""
        if "state_data" not in self.expected:
            self.skipTest("No state_data expected values for this version")
        expected = self.expected["state_data"]
        self.assertEqual(
            self.header.state_data.secondary_flag,
            expected["secondary_flag"],
            "secondary_flag should match expected value"
        )
    
    def test_state_data_asset_index(self):
        """Verify state_data asset_index field."""
        if "state_data" not in self.expected:
            self.skipTest("No state_data expected values for this version")
        expected = self.expected["state_data"]
        self.assertEqual(
            self.header.state_data.asset_index,
            expected["asset_index"],
            "asset_index should match expected value"
        )
    
    def test_state_data_unknown_f91(self):
        """Verify state_data unknown_f91 field."""
        if "state_data" not in self.expected:
            self.skipTest("No state_data expected values for this version")
        expected = self.expected["state_data"]
        self.assertEqual(
            self.header.state_data.unknown_f91,
            expected["unknown_f91"],
            "unknown_f91 should match expected value"
        )
    
    def test_state_data_backward_compat_unknown_f34(self):
        """Verify unknown_f34 property returns state_data.raw_data."""
        self.assertEqual(
            self.header.unknown_f34,
            self.header.state_data.raw_data,
            "unknown_f34 should return state_data.raw_data"
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


class TestSectorTableEntryProperties(unittest.TestCase):
    """Tests for SectorTableEntry helper properties."""
    
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
        cls.header = BLBHeader.from_file(cls.blb_path)
    
    def test_entry_type_property(self):
        """Test entry_type backward-compat property."""
        # Entry 0 in PAL is PIRA with entry_flags=0x05, level_index=0x37
        entry = self.header.sector_entries[0]
        expected_type = (entry.entry_flags << 8) | entry.level_index
        self.assertEqual(entry.entry_type, expected_type)
    
    def test_is_level_screen(self):
        """Test is_level_screen property."""
        # Find a level screen entry (entry_flags=0x00, level_index < 0x20)
        # In PAL, MENU is at index 3 with entry_flags=0x00, level_index=0x00
        menu_entry = self.header.get_sector_entry_by_code("MENU")
        if menu_entry:
            self.assertTrue(menu_entry.is_level_screen)
        
        # PIRA at index 0 is NOT a level screen (entry_flags=0x05)
        pira_entry = self.header.sector_entries[0]
        if pira_entry.entry_flags == 0x05:
            self.assertFalse(pira_entry.is_level_screen)
    
    def test_is_special_loading(self):
        """Test is_special_loading property."""
        # In PAL, PIRA (index 0) has entry_flags=0x05
        pira_entry = self.header.sector_entries[0]
        if pira_entry.entry_flags == 0x05:
            self.assertTrue(pira_entry.is_special_loading)
        
        # MENU should NOT be special loading
        menu_entry = self.header.get_sector_entry_by_code("MENU")
        if menu_entry:
            self.assertFalse(menu_entry.is_special_loading)
    
    def test_is_game_over(self):
        """Test is_game_over property."""
        # In PAL, OVER entries (index 30, 31) have entry_flags=0x03
        if len(self.header.sector_entries) > 30:
            over_entry = self.header.sector_entries[30]
            if over_entry.entry_flags == 0x03:
                self.assertTrue(over_entry.is_game_over)
        
        # MENU should NOT be game over
        menu_entry = self.header.get_sector_entry_by_code("MENU")
        if menu_entry:
            self.assertFalse(menu_entry.is_game_over)
    
    def test_is_credits(self):
        """Test is_credits property."""
        # In PAL, CRED entries have entry_flags=0x00, level_index=0x35
        cred_entry = self.header.get_sector_entry_by_code("CRED")
        if cred_entry and cred_entry.level_index == 0x35:
            self.assertTrue(cred_entry.is_credits)
        
        # MENU should NOT be credits
        menu_entry = self.header.get_sector_entry_by_code("MENU")
        if menu_entry:
            self.assertFalse(menu_entry.is_credits)
    
    def test_sector_entry_repr(self):
        """Test SectorTableEntry __repr__."""
        entry = self.header.sector_entries[0]
        repr_str = repr(entry)
        self.assertIn("SectorTableEntry", repr_str)
        self.assertIn(entry.code, repr_str)


class TestLevelMetadataEntryProperties(unittest.TestCase):
    """Tests for LevelMetadataEntry helper properties."""
    
    @classmethod
    def setUpClass(cls):
        """Load the BLB file."""
        import os
        cls.blb_path = Path(os.environ.get("BLB_PATH", DEFAULT_BLB_PATH))
        
        if not cls.blb_path.exists():
            raise unittest.SkipTest(f"BLB file not found: {cls.blb_path}")
        
        cls.header = BLBHeader.from_file(cls.blb_path)
    
    def test_secondary_byte_offset(self):
        """Test secondary_byte_offset property."""
        entry = self.header.level_entries[0]
        expected = entry.secondary_offset * SECTOR_SIZE
        self.assertEqual(entry.secondary_byte_offset, expected)
    
    def test_secondary_byte_size(self):
        """Test secondary_byte_size property."""
        entry = self.header.level_entries[0]
        expected = entry.secondary_count * SECTOR_SIZE
        self.assertEqual(entry.secondary_byte_size, expected)
    
    def test_tertiary_byte_offset(self):
        """Test tertiary_byte_offset property."""
        entry = self.header.level_entries[0]
        expected = entry.tertiary_offset * SECTOR_SIZE
        self.assertEqual(entry.tertiary_byte_offset, expected)
    
    def test_tertiary_byte_size(self):
        """Test tertiary_byte_size property."""
        entry = self.header.level_entries[0]
        expected = entry.tertiary_count * SECTOR_SIZE
        self.assertEqual(entry.tertiary_byte_size, expected)
    
    def test_level_entry_repr(self):
        """Test LevelMetadataEntry __repr__."""
        entry = self.header.level_entries[0]
        repr_str = repr(entry)
        self.assertIn("LevelMetadataEntry", repr_str)
        self.assertIn(entry.level_id, repr_str)
        self.assertIn(entry.name, repr_str)


class TestMovieEntryProperties(unittest.TestCase):
    """Tests for MovieEntry helper properties."""
    
    @classmethod
    def setUpClass(cls):
        """Load the BLB file."""
        import os
        cls.blb_path = Path(os.environ.get("BLB_PATH", DEFAULT_BLB_PATH))
        
        if not cls.blb_path.exists():
            raise unittest.SkipTest(f"BLB file not found: {cls.blb_path}")
        
        cls.header = BLBHeader.from_file(cls.blb_path)
    
    def test_movie_byte_size(self):
        """Test MovieEntry byte_size property."""
        if len(self.header.movie_entries) == 0:
            self.skipTest("No movie entries in this BLB version")
        
        movie = self.header.movie_entries[0]
        expected = movie.sector_count * SECTOR_SIZE
        self.assertEqual(movie.byte_size, expected)
    
    def test_movie_repr(self):
        """Test MovieEntry __repr__."""
        if len(self.header.movie_entries) == 0:
            self.skipTest("No movie entries in this BLB version")
        
        movie = self.header.movie_entries[0]
        repr_str = repr(movie)
        self.assertIn("MovieEntry", repr_str)
        self.assertIn(movie.movie_id, repr_str)
    
    def test_movies_property(self):
        """Test BLBHeader.movies property alias."""
        self.assertIs(self.header.movies, self.header.movie_entries)


class TestIsUnknownField(unittest.TestCase):
    """Tests for is_unknown_field helper function."""
    
    def test_unknown_field_returns_true(self):
        """Test is_unknown_field returns True for unknown fields."""
        # SectorTableEntry.unknown_byte is marked as unknown
        result = is_unknown_field(SectorTableEntry, "unknown_byte")
        self.assertTrue(result)
    
    def test_known_field_returns_false(self):
        """Test is_unknown_field returns False for known fields."""
        # SectorTableEntry.code is NOT marked as unknown
        result = is_unknown_field(SectorTableEntry, "code")
        self.assertFalse(result)
    
    def test_nonexistent_field_returns_false(self):
        """Test is_unknown_field returns False for nonexistent fields."""
        result = is_unknown_field(SectorTableEntry, "nonexistent_field")
        self.assertFalse(result)


class TestPrintTable(unittest.TestCase):
    """Tests for BLBHeader.print_table method."""
    
    @classmethod
    def setUpClass(cls):
        """Load the BLB file."""
        import os
        cls.blb_path = Path(os.environ.get("BLB_PATH", DEFAULT_BLB_PATH))
        
        if not cls.blb_path.exists():
            raise unittest.SkipTest(f"BLB file not found: {cls.blb_path}")
        
        cls.header = BLBHeader.from_file(cls.blb_path)
    
    def test_print_table_runs(self):
        """Test that print_table executes without error."""
        import io
        import sys
        
        # Capture stdout
        captured = io.StringIO()
        old_stdout = sys.stdout
        sys.stdout = captured
        
        try:
            self.header.print_table()
        finally:
            sys.stdout = old_stdout
        
        output = captured.getvalue()
        
        # Verify output contains expected content
        self.assertIn("BLB Header Summary", output)
        self.assertIn("Level Metadata Table", output)
        self.assertIn("Sector Offset Table", output)
        self.assertIn(str(self.header.level_count), output)


if __name__ == "__main__":
    unittest.main(verbosity=2)
