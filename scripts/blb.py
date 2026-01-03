#!/usr/bin/env python3
"""
blb.py - Library for parsing Skullmonkeys GAME.BLB archive headers

The BLB (blob) file is the main game archive containing all levels, assets,
and resources. The first 2 sectors (4096 bytes) contain the header with an
index table describing the contents.

Supported Versions:
===================
    - PAL (SLES-01090, SLES-01091, SLES-01092): 26 levels, 13 movies
    - NTSC-US (SLUS-00601): 26 levels, 13 movies  
    - NTSC-JP Demo (PAPX-90053): 6 levels, 1 movie
    - NTSC-JP Full (SLPS-01501): 6 levels, 1 movie
    - Beta: 26 levels, 13 movies

Header Structure (4096 bytes total):
=====================================
    Offset  Size   Description
    ------  ----   -----------
    0x000   2912   Level metadata table (0x70 bytes × 26 entries max)
    0xB60    364   Movie table (0x1C bytes × 13 entries max)
    0xCD0    512   Sector/loading screen table (0x10 bytes × 32 entries, count at 0xF33)
    0xED0     64   Unknown u32 array (16 entries) - file offsets, purpose TBD
    0xF10     33   Credits sequence table (0x0C bytes × 2 complete entries + overlap)
    0xF31      1   Level count (u8)
    0xF32      1   Asset/movie count (u8)
    0xF33      1   Sector table entry count (u8)
    0xF34    204   Playback Sequence data (mode array at 0xF36, index array at 0xF92)


Playback Sequence (0xF34-0xFFF, 204 bytes):
===========================================
    The game uses a "playback sequence" to define what happens before/during
    level loading (intro movies, credits, etc). The InitializeAndLoadLevel
    function walks through this sequence using AdvancePlaybackSequence.
    paired bytes for mode and index.
    
    Offset  Size  Type    Field               Description
    ------  ----  ----    -----               -----------
    0xF34      2  bytes   unknown_f34         Unknown (padding?)
    0xF36      1  u8      game_mode           Base mode byte (see mode values below)
    0xF37      1  u8      secondary_flag      Secondary state flag
    0xF38     89  bytes   mode_array          Playback mode bytes (0xF36 + stateOffset)
    0xF91      1  u8      unknown_f91         Unknown
    0xF92      1  u8      asset_index         Base index byte for table lookups
    0xF93    109  bytes   index_array         Playback index bytes (0xF92 + stateOffset)
    
    Mode Values (CONFIRMED via decompilation of InitializeAndLoadLevel):
      0 = Invalid/Reset - search for next valid item
      1 = Movie playback mode (uses movie table at 0xB60)
      2 = Credits display mode (uses credits table at 0xF10)
      3 = Level mode - load and play level (uses level table at 0x000)
      4 = Special sector loading mode (uses sector order data)
      5 = Initial sector loading mode (similar to 4)
      6 = End/Special - sequence terminator or special mode
    
    String Markers for Flow Control (CONFIRMED via decompilation):
      "INT1" (0x800A6094) - After playing intro 1, skip next two items
      "CRD1" (0x800A609C) - After credits 1, skip to level
      "CRD2" (0x800A6058) - Credits 2 terminator (used by AdvancePlaybackSequence)
      "END2" (0x800A608C) - Ending 2, skipped under certain conditions
    
    The accessor functions read state using:
      mode = header[0xF36 + stateOffset]
      index = header[0xF92 + stateOffset]
    
    Where stateOffset is stored at GameState+0x60 and increments as the game
    progresses through different loading/playback states.


Credits Sequence Table (0x0C = 12 bytes, at offset 0xF10):
==========================================================
    NOTE: "Credits" is a GUESSED name based on the "CRD1"/"CRD2" codes and
    relationship to the "CRED" sector entry. The actual purpose is not fully
    confirmed - it may control credits sequence playback or something else.
    JP versions don't have this table (garbage data at this offset).

    Offset  Size  Type    Field               Description
    ------  ----  ----    -----               -----------
    0x00       4  str     code                4-char ID (empty for entry 0, "CRD1"/"CRD2" for others)
    0x04       4  bytes   padding             Unused (zeros)
    0x08       2  u16     param_a             Entry 0: level_count (26); CRD1/2: unknown counter
    0x0A       2  u16     param_b             Base sector offset (e.g., 171)

    Sector Calculation (verified):
        CRED.sector_offset = param_b + level_count
        Example: 171 + 26 = 197 = CRED sector entry
    
    Note: Entry 2 (CRD2) at 0xF28 overlaps with count fields at 0xF31-0xF33,
    so its param values are garbage. Only 2 complete entries fit (indices 0 and 1).
    
    Accessed by GetCurrentSectorOffset (0x8007ABCC) and GetCurrentSectorCount
    (0x8007AC54) when game mode == 2, using index from header[0xF92+stateOffset]
    with stride of 12 bytes.


Level Metadata Entry (0x70 = 112 bytes, at offset 0x000):
=========================================================
    Offset  Size  Type    Field               Description
    ------  ----  ----    -----               -----------
    
    # Primary data pointers (0x00-0x0B)
    0x00       2  u16     sector_offset       Primary data sector offset in BLB
    0x02       2  u16     sector_count        Primary data sector count
    0x04       2  u16     unknown_04          Offset into Entry[0] data (purpose TBD)
    0x06       2  u16     unknown_06          Unknown count (7-20 range, purpose TBD)
    0x08       2  u16     entry1_offset_lo    Low 16 bits of Entry[1].offset in primary TOC (CONFIRMED)
    0x0A       2  u16     unknown_0A          Unknown count (0-16 range, purpose TBD)
    
    # Level identification (0x0C-0x0D)
    0x0C       1  u8      level_index         Level asset index (used for asset loading)
    0x0D       1  u8      level_flag          Level flag (0 or 1, purpose TBD)
    
    # Tertiary block configuration (0x0E-0x1B)
    0x0E       2  u16     tert_block_count    Number of active tertiary sub-blocks (1-6)
    0x10       2  u16     tert_data_off_0     Offset within tertiary block 0 to data
    0x12       2  u16     tert_data_off_1     Offset within tertiary block 1 to data
    0x14       2  u16     tert_data_off_2     Offset within tertiary block 2 to data
    0x16       2  u16     tert_data_off_3     Offset within tertiary block 3 to data
    0x18       2  u16     tert_data_off_4     Offset within tertiary block 4 to data
    0x1A       2  u16     tert_data_off_5     Offset within tertiary block 5 to data
    0x1C       2  u16     (padding)           Always 0
    
    # Secondary data structure (0x1E-0x39)
    0x1E       2  u16     secondary_offset    Secondary data sector offset
    0x20       2  u16     sec_sub_off_0       Secondary sub-block 0 sector offset
    0x22       2  u16     sec_sub_off_1       Secondary sub-block 1 sector offset
    0x24       2  u16     sec_sub_off_2       Secondary sub-block 2 sector offset
    0x26       2  u16     sec_sub_off_3       Secondary sub-block 3 sector offset
    0x28       2  u16     sec_sub_off_4       Secondary sub-block 4 sector offset
    0x2A       2  u16     (padding)           Always 0
    0x2C       2  u16     secondary_count     Secondary data sector count
    0x2E       2  u16     sec_sub_cnt_0       Secondary sub-block 0 sector count
    0x30       2  u16     sec_sub_cnt_1       Secondary sub-block 1 sector count
    0x32       2  u16     sec_sub_cnt_2       Secondary sub-block 2 sector count
    0x34       2  u16     sec_sub_cnt_3       Secondary sub-block 3 sector count
    0x36       2  u16     sec_sub_cnt_4       Secondary sub-block 4 sector count
    0x38       2  u16     (padding)           Always 0
    
    # Tertiary data structure (0x3A-0x55)
    0x3A       2  u16     tert_sub_off_0      Tertiary sub-block 0 sector offset
    0x3C       2  u16     tert_sub_off_1      Tertiary sub-block 1 sector offset
    0x3E       2  u16     tert_sub_off_2      Tertiary sub-block 2 sector offset
    0x40       2  u16     tert_sub_off_3      Tertiary sub-block 3 sector offset
    0x42       2  u16     tert_sub_off_4      Tertiary sub-block 4 sector offset
    0x44       2  u16     tert_sub_off_5      Tertiary sub-block 5 sector offset
    0x46       2  u16     (padding)           Always 0
    0x48       2  u16     tert_sub_cnt_0      Tertiary sub-block 0 sector count
    0x4A       2  u16     tert_sub_cnt_1      Tertiary sub-block 1 sector count
    0x4C       2  u16     tert_sub_cnt_2      Tertiary sub-block 2 sector count
    0x4E       2  u16     tert_sub_cnt_3      Tertiary sub-block 3 sector count
    0x50       2  u16     tert_sub_cnt_4      Tertiary sub-block 4 sector count
    0x52       2  u16     tert_sub_cnt_5      Tertiary sub-block 5 sector count
    0x54       2  u16     (padding)           Always 0
    
    # Level strings (0x56-0x6F)
    0x56       5  str     level_id            4-char null-terminated (e.g., "MENU")
    0x5B      21  str     name                Level name, null-terminated
    
    Data Interleaving Pattern:
    --------------------------
    Level data sectors are interleaved: PRIMARY → SECONDARY → TERT[0] → SEC_SUB[0] 
    → TERT[1] → SEC_SUB[1] → ... This creates an alternating pattern where tertiary 
    blocks and secondary sub-blocks occupy adjacent sector ranges.
    
    Example (MENU level): 201-232(PRI) → 233-299(SEC) → 300-787(T0) → 788-849(S0)
    → 850-852(T1) → 853-911(S1) → 912-917(T2) → 918-980(S2) → ...
    
    The tert_data_off_N values are byte offsets WITHIN each tertiary block, pointing
    to specific embedded data structures.


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


GameState Structure (runtime, not in BLB file):
================================================
    The BLB accessor functions receive a context pointer (GameState + 0x84),
    not the raw GameState or header pointer directly.
    
    Key offsets discovered from runtime tracing:
    
    GameState (base: 0x8009DC40 in PAL):
    ------------------------------------
    Offset  Size  Type    Field               Description
    ------  ----  ----    -----               -----------
    0x5C       4  ptr     headerBuffer        Pointer to BLB header in RAM (→ 0x800AE3E0)
    0x60       1  u8      stateOffset         Sliding window offset into state arrays
    0x84       ?  struct  accessorContext     Context passed to accessor functions
    
    PAL runtime addresses (observed):
      - GameState base: 0x8009DC40 (passed to LoadBLBHeader)
      - Accessor context: 0x8009DCC4 (GameState + 0x84, passed to other accessors)
      - BLB header in RAM: 0x800AE3E0 (actual header data)
    
    Sliding Window State Machine:
    -----------------------------
    The game tracks multiple concurrent states using a sliding window pattern.
    Each "slot" uses paired bytes at offsets determined by arrayPosition:
      - Mode byte: header[0xF36 + arrayPosition]
      - Index byte: header[0xF92 + arrayPosition]
    
    VERIFIED FORMULA (via PCSX-Redux MCP):
      arrayPosition = headerOffset - 0x0A
      levelIndex = header[0xF92 + arrayPosition]
      levelEntry = header + (levelIndex * 0x70)
    
    Where headerOffset is read from LevelDataContext at ctx+0x60.
    The base offset 0x0A corresponds to MENU being loaded.
    
    Mode Values (CONFIRMED via decompilation of InitializeAndLoadLevel):
      0 = Invalid/Reset - AdvancePlaybackSequence calls SeekToLevelInSequence
      1 = Movie playback mode (uses movie table at header+0xB60)
      2 = Credits display mode (uses credits table at header+0xF10)
      3 = Level mode - load and play (uses level table at header+0x000)
      4 = Special sector loading mode (uses sector order data at header+0xCD0)
      5 = Initial sector loading mode (similar to 4)
      6 = End/Special - sequence terminator, exits playback loop
    
    Verified examples:
      headerOffset=0x0A, arrayPosition=0, levelIndex=0  → MENU
      headerOffset=0x20, arrayPosition=22, levelIndex=9 → FOOD


LevelDataContext Structure (verified via emulator MCP):
========================================================
    NOTE: These addresses are for PAL version (SLES-01090).
    
    Offset  Size  Type    Field           Description
    ------  ----  ----    -----           -----------
    0x00    0x5C  var     unk00-unk58     Cleared on level load
    0x5C    4     ptr     header          Pointer to BLB header (→ 0x800AE3E0)
    0x60    1     u8      headerOffset    Offset within header state array
    0x61    3     -       pad61           Padding
    0x64    4     ptr     loadCallback    CD load callback function (→ 0x80020848)
    0x68    4     ptr     tocPtr          Pointer to loaded TOC
    0x6C    4     ptr     dataOffset      Pointer to data after TOC
    0x70    4     ptr     asset258        Pointer to geometry data (type 0x258)
    0x74    4     ptr     asset259        Pointer to collision data (type 0x259)
    0x78    4     u32     asset259Size    Size of collision data in bytes
    0x7C    4     ptr     asset25A        Pointer to palette data (type 0x25A)

    Verified runtime example (Science Centre / SCIE, level index 2):
      ctx @ 0x8009DCC4:
        header     = 0x800AE3E0 (BLB header)
        tocPtr     = 0x800AF3E0 (loaded TOC, 3 entries)
        asset258   = 0x800AF408 (524,212 bytes geometry)
        asset259   = 0x8012F3BC (126,256 bytes collision)
        asset25A   = 0x8014E0EC (148 bytes palette)

Known Functions (from trace + decompilation analysis):
=========================================================
    Address     Name                        Description
    -------     ----                        -----------
    
    # Level Loading System (CONFIRMED via Ghidra decompilation)
    0x8007A294  AdvancePlaybackSequence     Advance through playback sequence, return next mode
    0x8007A3AC  SeekToLevelInSequence       Search for level with matching asset index
    0x8007A62C  LevelDataParser             Parse level metadata from header (func_8007A62C)
    0x8007B074  LoadAssetContainer          Load secondary/tertiary container, parse sub-TOC
    0x8007D1D0  InitializeAndLoadLevel      Main level loading orchestrator
    
    # BLB Header Accessors (StateAccessors.c)
    0x8007A9B0  GetLevelCount               Get level count from header[0xF31]
    0x8007A9C4  GetLevelAssetIndex          Get level asset index at level[n]+0x0C
    0x8007A9E8  GetLevelDisplayName         Get level display name ptr at level[n]+0x56
    0x8007AA08  getLevelName                Get level name string at level[n]+0x5B
    0x8007AA28  GetLevelFlagByIndex         Get level flag at level[n]+0x0D
    0x8007AA4C  GetCurrentLevelAssetIndex   Get current level asset index (mode 3)
    0x8007AA90  GetCurrentLevelDisplayName  Get current level display name (mode 3)
    0x8007AAE8  GetCurrentLevelName         Get current level name (modes 3, 6)
    0x8007AB44  GetCurrentSectorOrderUnk1   Get sector order +0xCD1 (modes 4-5)
    0x8007AB88  GetCurrentSectorOrderUnk2   Get sector order +0xCD2 (modes 4-5)
    
    # BLB Header Accessors (CreditsAccessors.c)
    0x8007ABCC  GetCurrentSectorOffset      Get sector offset for current mode
    0x8007AC54  GetCurrentSectorCount       Get sector count for current mode
    0x8007ACDC  GetAssetCount               Get asset count from header[0xF32]
    0x8007ACF0  GetMovieReservedByIndex     Get movie reserved ptr at movie[n]+0x04
    0x8007AD10  GetMovieShortNameByIndex    Get movie short name at movie[n]+0x09
    0x8007AD30  GetCurrentMovieReserved     Get current movie reserved (mode 1)
    0x8007AD7C  GetCurrentMovieShortName    Get current movie short name (mode 1)
    0x8007ADC8  GetCurrentMovieFilename     Get current movie filename at +0xB6C (mode 1)
    
    # BLB Header Accessors (BLBHeaderAccessors.c)
    0x8007AE14  GetMovieUnknown00           Get movie u16 at +0xB60 (mode 1)
    0x8007AE58  GetMovieSectorCount         Get movie sector count at +0xB62 (mode 1)
    0x8007AE9C  GetCurrentModeReservedData  Mode-specific reserved data dispatcher (switch)
    0x8007AFA4  GetCurrentLevelFlag         Get flag byte for current level (mode 3)
    0x8007AFF8  GetCurrentTertiaryDataSize  Calculate tertiary block size
    
    # CD/BLB Loading
    0x800208B0  LoadBLBHeader               Load BLB header into GameState
    0x80038BA0  CdBLB_ReadSectors           Read sectors from CD
    
    # Other
    0x8007CF08  ProcessLevelEntry           Iterate through levels
    0x8008299C  DisplayLevelName            Display level names


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

# State/config data region (0xF34-0x1000, 204 bytes)
# Runtime state fields used by BLB accessor functions
STATE_DATA_OFFSET = 0xF34
STATE_DATA_SIZE = 0x1000 - STATE_DATA_OFFSET  # 204 bytes

# Known field offsets within state data region (relative to 0xF34)
STATE_GAME_MODE_OFFSET = 0xF36  # u8: Game mode/state (1=movie, 2=credits, 4-5=level)
STATE_SECONDARY_FLAG_OFFSET = 0xF37  # u8: Secondary state flag
STATE_UNKNOWN_F91_OFFSET = 0xF91  # u8: Unknown (credits-related?)
STATE_ASSET_INDEX_OFFSET = 0xF92  # u8: Current asset/movie/entry index for table lookups


@dataclass
class StateData:
    """
    Playback Sequence data from header offset 0xF34-0x1000 (204 bytes).
    
    This region defines the "playback sequence" - a list of items (movies,
    credits, levels) that the game walks through before/during gameplay.
    The InitializeAndLoadLevel function (0x8007D1D0) iterates through this
    sequence using AdvancePlaybackSequence (0x8007A294).
    
    Structure: Parallel Arrays (CONFIRMED via decompilation)
    =========================================================
    The region contains two parallel arrays:
      - Mode array at 0xF36-0xF91 (92 bytes): Determines item type
      - Index array at 0xF92-0xFFF (110 bytes): Selects entry in relevant table
    
    For each position n in the sequence:
      mode = header[0xF36 + n]   // What type of item
      index = header[0xF92 + n]  // Which entry in that table
    
    Mode Values (CONFIRMED via decompilation):
      0 = Invalid/Reset - search for next valid item
      1 = Movie - play movie[index] from movie table at 0xB60
      2 = Credits - show credits[index] from credits table at 0xF10
      3 = Level - load level[index] from level table at 0x000
      4 = Special loading (uses sector order data at 0xCD0)
      5 = Initial loading (similar to 4)
      6 = End - sequence terminator, exit playback loop
    
    Example Sequence (typical game start):
      n=0: mode=1, idx=0  → Play DREA (Dreamworks logo)
      n=1: mode=1, idx=1  → Play LOGO
      n=2: mode=1, idx=2  → Play ELEC (EA logo)
      n=3: mode=1, idx=3  → Play INT1 (intro part 1)
      n=4: mode=1, idx=4  → Play INT2 (intro part 2)
      n=5: mode=3, idx=0  → Load MENU level
    
    String Markers for Flow Control:
      - "INT1": After playing, skip next two items
      - "CRD1": After credits 1, skip to level
      - "CRD2": Credits 2 terminator
      - "END2": Ending marker, skipped under conditions
    
    Functions that use this:
      - AdvancePlaybackSequence (0x8007A294): Walk to next item
      - SeekToLevelInSequence (0x8007A3AC): Find level by asset index
      - InitializeAndLoadLevel (0x8007D1D0): Main orchestrator
    """
    
    raw_data: bytes  # Full 204 bytes
    
    # Known fields (absolute offsets in header)
    game_mode: int = None  # u8 at 0xF36: base state (0-6, see mode values above)
    secondary_flag: int = None  # u8 at 0xF37: next state slot
    movie_state: int = None  # u8 at 0xF38: movie accessor state
    unknown_f91: int = unknown("+0x5D: u8, part of index array?")
    asset_index: int = None  # u8 at 0xF92: base index
    asset_index_1: int = None  # u8 at 0xF93: index slot 1
    movie_index: int = None  # u8 at 0xF94: movie index (for movie accessors)
    
    # The bytes form parallel arrays of mode/index pairs (playback sequence)
    unknown_f34_f35: bytes = unknown("0xF34-0xF35: 2 bytes, purpose TBD")
    mode_array: bytes = unknown("0xF36-0xF91: 92 bytes, playback sequence mode values")
    index_array: bytes = unknown("0xF92-0xFFF: 110 bytes, playback sequence index values")
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "StateData":
        """Parse state data from raw bytes (starting at 0xF34)."""
        if len(data) < STATE_DATA_SIZE:
            raise ValueError(f"State data too short: {len(data)} < {STATE_DATA_SIZE}")
        
        return cls(
            raw_data=data[:STATE_DATA_SIZE],
            game_mode=data[0xF36 - STATE_DATA_OFFSET],  # offset 2
            secondary_flag=data[0xF37 - STATE_DATA_OFFSET],  # offset 3
            movie_state=data[0xF38 - STATE_DATA_OFFSET],  # offset 4
            unknown_f91=data[0xF91 - STATE_DATA_OFFSET],  # offset 0x5D
            asset_index=data[0xF92 - STATE_DATA_OFFSET],  # offset 0x5E
            asset_index_1=data[0xF93 - STATE_DATA_OFFSET],  # offset 0x5F
            movie_index=data[0xF94 - STATE_DATA_OFFSET],  # offset 0x60
            unknown_f34_f35=data[0:2],
            mode_array=data[2:0x5E],  # 0xF36 to 0xF91 (92 bytes) - playback sequence modes
            index_array=data[0x5E:],  # 0xF92 to end (110 bytes) - playback sequence indices
        )
    
    def get_sequence_item(self, position: int) -> tuple:
        """Get (mode, index) for a position in the playback sequence.
        
        Args:
            position: Position in sequence (0 = first item at 0xF36/0xF92)
        
        Returns:
            Tuple of (mode, index) for that position
        """
        if position < 0 or position >= len(self.mode_array):
            return (0, 0)
        return (self.mode_array[position], self.index_array[position])
    
    def __repr__(self) -> str:
        return f"StateData(mode={self.game_mode}, idx={self.asset_index})"


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
    
    # Primary data internal pointers (0x04-0x0B)
    unknown_04: int = unknown("+0x04: u16, offset into Entry[0] data, purpose TBD")
    unknown_06: int = unknown("+0x06: u16, count (7-20 range), purpose TBD")
    entry1_offset_lo: int = None  # u16 at +0x08: Low 16 bits of Entry[1].offset in primary TOC (CONFIRMED)
    unknown_0A: int = unknown("+0x0A: u16, count (0-16 range), purpose TBD")
    
    # Level identification (0x0C-0x0D)
    level_index: int = None  # u8 at +0x0C: Level asset index
    level_flag: int = unknown("+0x0D: u8, 0 or 1, purpose TBD")
    
    # Tertiary block configuration (0x0E-0x1B)
    tert_block_count: int = None  # u16 at +0x0E: Number of active tertiary sub-blocks (CONFIRMED)
    tert_data_offsets: list = None  # 6× u16 at +0x10-0x1A: Offsets within each tert block to specific data
    # +0x1C-0x1D: Padding (always 0)
    
    # Secondary data location (0x1E-0x39)
    secondary_offset: int = None  # u16 at +0x1E: Secondary base sector offset
    sec_sub_offsets: list = None  # 5× u16 at +0x20-0x28: Secondary sub-block sector offsets
    # +0x2A-0x2B: Padding (always 0)
    secondary_count: int = None  # u16 at +0x2C: Secondary base sector count
    sec_sub_counts: list = None  # 5× u16 at +0x2E-0x36: Secondary sub-block sector counts
    # +0x38-0x39: Padding (always 0)
    
    # Tertiary data location (0x3A-0x55)
    tert_sub_offsets: list = None  # 6× u16 at +0x3A-0x44: Tertiary sub-block sector offsets
    # +0x46-0x47: Padding (always 0)
    tert_sub_counts: list = None  # 6× u16 at +0x48-0x52: Tertiary sub-block sector counts
    # +0x54-0x55: Padding (always 0)
    
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
        """Calculate first tertiary sub-block byte offset within the BLB file."""
        if self.tert_sub_offsets and self.tert_sub_offsets[0]:
            return self.tert_sub_offsets[0] * SECTOR_SIZE
        return 0
    
    @property
    def tertiary_byte_size(self) -> int:
        """Calculate total tertiary size in bytes (sum of all sub-blocks)."""
        if self.tert_sub_counts:
            return sum(c for c in self.tert_sub_counts if c) * SECTOR_SIZE
        return 0
    
    def get_tert_sub_byte_offset(self, sub_idx: int) -> int:
        """Calculate byte offset for a specific tertiary sub-block."""
        if self.tert_sub_offsets and sub_idx < len(self.tert_sub_offsets):
            return self.tert_sub_offsets[sub_idx] * SECTOR_SIZE
        return 0
    
    def get_tert_sub_byte_size(self, sub_idx: int) -> int:
        """Calculate byte size for a specific tertiary sub-block."""
        if self.tert_sub_counts and sub_idx < len(self.tert_sub_counts):
            return self.tert_sub_counts[sub_idx] * SECTOR_SIZE
        return 0
    
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
    Represents an entry in the credits sequence table (GUESSED NAME).
    
    NOTE: "Credits" is a guessed name based on the "CRD1"/"CRD2" codes found
    in entries and their relationship to the "CRED" sector entry. The actual
    purpose is not fully confirmed. JP versions don't have valid data here.
    
    Located at header offset 0xF10, with 0x0C (12) bytes per entry.
    Only 2 complete entries fit before overlapping with count fields at 0xF31.
    
    Used by GetCurrentSectorOffset (0x8007ABCC) and GetCurrentSectorCount
    (0x8007AC54) when game mode == 2, with index from header[0xF92+stateOffset].
    
    Sector Calculation (verified):
        actual_sector = param_b + level_count
        Example: Entry 0 has param_b=171, level_count=26 → 171+26=197 = CRED sector
    
    Entry 0: Empty code, param_a=level_count, param_b=base_sector (credits header)
    Entry 1 (CRD1): Credits sequence 1
    Entry 2 (CRD2): Overlaps count fields at 0xF31-0xF33 (garbage values)
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
    unknown_ed0: bytes = unknown("0xED0-0xF10: 64 bytes (16 u32 values), purpose TBD")
    
    # State/config data with parsed fields
    state_data: StateData = None
    
    # Keep unknown_f34 as property for backward compatibility
    @property
    def unknown_f34(self) -> bytes:
        """Raw state data bytes for backward compatibility."""
        return self.state_data.raw_data if self.state_data else None
    
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
        # Valid credits entries have zero padding at bytes +4 to +8
        # JP versions have garbage data here (non-zero padding)
        credits_entries = []
        for i in range(CREDITS_MAX_ENTRIES):
            offset = CREDITS_TABLE_OFFSET + (i * CREDITS_ENTRY_SIZE)
            if offset + CREDITS_ENTRY_SIZE > LEVEL_COUNT_OFFSET:
                break
            entry_data = data[offset:offset + CREDITS_ENTRY_SIZE]
            # Check if this is a valid credits entry (padding must be zeros)
            padding = entry_data[0x04:0x08]
            if padding != b'\x00\x00\x00\x00':
                # Invalid entry - stop parsing (JP versions have garbage here)
                break
            credits_entries.append(cls._parse_credits_entry(i, offset, entry_data))
        
        # Read unknown regions as raw bytes
        unknown_ed0 = data[UNKNOWN_ARRAY_OFFSET:CREDITS_TABLE_OFFSET]  # 64 bytes (16 u32 values)
        
        # Parse state/config data region
        state_data = StateData.from_bytes(data[STATE_DATA_OFFSET:0x1000])
        
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
            state_data=state_data,
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
        """Parse a single 16-byte sector table entry.
        
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
        # Detect layout by checking where the code starts
        # PAL: code at byte 3 (uppercase letter), byte 7 is null terminator
        # JP: code at byte 7 (uppercase letter), byte 3 is low byte of count (usually small)
        is_pal_layout = (65 <= data[3] <= 90)  # byte[3] is A-Z
        
        if is_pal_layout:
            # PAL/NTSC-US layout: code at offset 3
            level_index = data[0]
            entry_flags = data[1]
            unknown_byte = data[2]
            code = data[3:8].rstrip(b'\x00').decode('ascii', errors='replace')
            short_name = data[8:12].rstrip(b'\x00').decode('ascii', errors='replace')
            sector_offset = struct.unpack('<H', data[12:14])[0]
            sector_count = struct.unpack('<H', data[14:16])[0]
        else:
            # JP layout: offset/count at front, code at offset 7
            sector_offset = struct.unpack('<H', data[0:2])[0]
            sector_count = struct.unpack('<H', data[2:4])[0]
            level_index = data[4]
            entry_flags = data[5]
            unknown_byte = data[6]
            code = data[7:12].rstrip(b'\x00').decode('ascii', errors='replace')
            short_name = data[12:16].rstrip(b'\x00').decode('ascii', errors='replace')
        
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
        # Primary data location (0x00-0x03)
        sector_offset = struct.unpack('<H', data[0x00:0x02])[0]
        sector_count = struct.unpack('<H', data[0x02:0x04])[0]
        
        # Primary data internal pointers (0x04-0x0B)
        unknown_04 = struct.unpack('<H', data[0x04:0x06])[0]
        unknown_06 = struct.unpack('<H', data[0x06:0x08])[0]
        entry1_offset_lo = struct.unpack('<H', data[0x08:0x0A])[0]
        unknown_0A = struct.unpack('<H', data[0x0A:0x0C])[0]
        
        # Level identification (0x0C-0x0D)
        level_index = data[0x0C]
        level_flag = data[0x0D]
        
        # Tertiary block configuration (0x0E-0x1B)
        tert_block_count = struct.unpack('<H', data[0x0E:0x10])[0]
        tert_data_offsets = [struct.unpack('<H', data[0x10 + i*2:0x12 + i*2])[0] for i in range(6)]
        # 0x1C-0x1D is padding
        
        # Secondary data location (0x1E-0x39)
        secondary_offset = struct.unpack('<H', data[0x1E:0x20])[0]
        sec_sub_offsets = [struct.unpack('<H', data[0x20 + i*2:0x22 + i*2])[0] for i in range(5)]
        # 0x2A-0x2B is padding
        secondary_count = struct.unpack('<H', data[0x2C:0x2E])[0]
        sec_sub_counts = [struct.unpack('<H', data[0x2E + i*2:0x30 + i*2])[0] for i in range(5)]
        # 0x38-0x39 is padding
        
        # Tertiary data location (0x3A-0x55)
        tert_sub_offsets = [struct.unpack('<H', data[0x3A + i*2:0x3C + i*2])[0] for i in range(6)]
        # 0x46-0x47 is padding
        tert_sub_counts = [struct.unpack('<H', data[0x48 + i*2:0x4A + i*2])[0] for i in range(6)]
        # 0x54-0x55 is padding
        
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
            unknown_04=unknown_04,
            unknown_06=unknown_06,
            entry1_offset_lo=entry1_offset_lo,
            unknown_0A=unknown_0A,
            level_index=level_index,
            level_flag=level_flag,
            tert_block_count=tert_block_count,
            tert_data_offsets=tert_data_offsets,
            secondary_offset=secondary_offset,
            sec_sub_offsets=sec_sub_offsets,
            secondary_count=secondary_count,
            sec_sub_counts=sec_sub_counts,
            tert_sub_offsets=tert_sub_offsets,
            tert_sub_counts=tert_sub_counts,
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


# =============================================================================
# Asset Container Parsing
# =============================================================================

@dataclass
class AssetTOCEntry:
    """A single entry in an asset container's TOC."""
    asset_id: int       # Asset type ID (100, 200, 300, 400, etc.)
    size: int           # Size in bytes
    offset: int         # Offset from start of container
    data: bytes = None  # Raw asset data (optional)
    
    @property
    def asset_id_hex(self) -> str:
        return f"0x{self.asset_id:03X}"


@dataclass
class TileHeader:
    """
    Header structure from asset 100 (geometry header).
    
    Contains tile counts used by FUN_8007B53C to calculate total tiles.
    """
    raw_data: bytes
    count_16x16: int  # u16 at offset 0x10: Number of 16×16 tiles
    count_8x8_a: int  # u16 at offset 0x12: First 8×8 tile count
    count_8x8_b: int  # u16 at offset 0x14: Second 8×8 tile count
    
    @property
    def total_tiles(self) -> int:
        return self.count_16x16 + self.count_8x8_a + self.count_8x8_b
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "TileHeader":
        if len(data) < 0x16:
            raise ValueError(f"Header too short: {len(data)} bytes")
        
        return cls(
            raw_data=data,
            count_16x16=struct.unpack('<H', data[0x10:0x12])[0],
            count_8x8_a=struct.unpack('<H', data[0x12:0x14])[0],
            count_8x8_b=struct.unpack('<H', data[0x14:0x16])[0],
        )


def parse_container_toc(data: bytes) -> list[AssetTOCEntry]:
    """
    Parse a container's TOC (Table of Contents).
    
    Container format:
        0x00: u32 entry_count
        0x04: entries[count] - each entry is 12 bytes:
            0x00: u32 asset_id
            0x04: u32 size
            0x08: u32 offset (from container start)
    
    Returns list of AssetTOCEntry with data populated.
    """
    if len(data) < 4:
        return []
    
    count = struct.unpack('<I', data[0:4])[0]
    entries = []
    
    for i in range(count):
        toc_offset = 4 + i * 12
        if toc_offset + 12 > len(data):
            break
        
        asset_id, size, data_offset = struct.unpack('<III', data[toc_offset:toc_offset+12])
        
        # Extract actual data if within bounds
        asset_data = None
        if data_offset + size <= len(data):
            asset_data = data[data_offset:data_offset+size]
        
        entries.append(AssetTOCEntry(
            asset_id=asset_id,
            size=size,
            offset=data_offset,
            data=asset_data,
        ))
    
    return entries


def psx_color_to_rgb(color: int) -> tuple:
    """
    Convert PSX 15-bit color to RGB tuple.
    
    PSX format: ABBBBBGGGGGRRRRR (little-endian u16)
        - Bits 0-4: Red (0-31)
        - Bits 5-9: Green (0-31)
        - Bits 10-14: Blue (0-31)
        - Bit 15: STP (semi-transparency)
    """
    r = (color & 0x1F) * 8
    g = ((color >> 5) & 0x1F) * 8
    b = ((color >> 10) & 0x1F) * 8
    return (r, g, b)


def parse_psx_palette(data: bytes, num_colors: int = 256) -> list[tuple]:
    """
    Parse a PSX CLUT (Color Look-Up Table) to RGB tuples.
    
    Each color is a 15-bit value stored as little-endian u16.
    Standard palettes are 16 colors (32 bytes) or 256 colors (512 bytes).
    """
    colors = []
    max_colors = min(num_colors, len(data) // 2)
    
    for i in range(max_colors):
        color = struct.unpack('<H', data[i*2:i*2+2])[0]
        colors.append(psx_color_to_rgb(color))
    
    return colors


def extract_tile_8bpp(tile_data: bytes, width: int = 16, height: int = 16) -> list[list[int]]:
    """
    Extract an 8bpp tile to a 2D array of palette indices.
    
    8bpp format: Each byte is one pixel index (0-255).
    16×16 tile = 256 bytes.
    """
    pixels = []
    
    for y in range(height):
        row = []
        for x in range(width):
            byte_idx = y * width + x
            if byte_idx < len(tile_data):
                row.append(tile_data[byte_idx])
            else:
                row.append(0)
        pixels.append(row)
    
    return pixels


def extract_tile_4bpp(tile_data: bytes, width: int = 16, height: int = 16) -> list[list[int]]:
    """
    Extract a 4bpp tile to a 2D array of palette indices.
    
    4bpp format: Each byte contains 2 pixels (low nibble first).
    16×16 tile = 128 bytes (8 bytes per row).
    """
    pixels = []
    bytes_per_row = width // 2
    
    for y in range(height):
        row = []
        for x in range(0, width, 2):
            byte_idx = y * bytes_per_row + x // 2
            if byte_idx < len(tile_data):
                byte = tile_data[byte_idx]
                row.append(byte & 0x0F)        # Low nibble = first pixel
                row.append((byte >> 4) & 0x0F) # High nibble = second pixel
            else:
                row.extend([0, 0])
        pixels.append(row)
    
    return pixels


def tile_to_image(pixel_indices: list[list[int]], palette: list[tuple]) -> "Image":
    """
    Convert a 2D array of palette indices to a PIL Image.
    
    Args:
        pixel_indices: 2D array from extract_tile_8bpp or extract_tile_4bpp
        palette: List of RGB tuples from parse_psx_palette
        
    Returns:
        PIL Image in RGB mode
        
    Example:
        >>> indices = extract_tile_8bpp(tile_data)
        >>> palette = parse_psx_palette(palette_data)
        >>> img = tile_to_image(indices, palette)
        >>> img.save("tile.png")
    """
    from PIL import Image
    
    height = len(pixel_indices)
    width = len(pixel_indices[0]) if pixel_indices else 0
    
    img = Image.new('RGB', (width, height))
    pixels = img.load()
    
    for y, row in enumerate(pixel_indices):
        for x, idx in enumerate(row):
            if idx < len(palette):
                pixels[x, y] = palette[idx]
            else:
                pixels[x, y] = (255, 0, 255)  # Magenta for invalid index
    
    return img


def extract_level_tiles(blb: "BLBFile", level_index: int, palette_index: int = 0) -> list["Image"]:
    """
    Extract all tiles from a level's secondary container.
    
    Args:
        blb: Open BLBFile instance
        level_index: Index of the level to extract from
        palette_index: Which palette to use (default: 0)
        
    Returns:
        List of PIL Images, one per tile
        
    Example:
        >>> with BLBFile('game.blb') as blb:
        ...     tiles = extract_level_tiles(blb, level_index=0)
        ...     for i, tile in enumerate(tiles):
        ...         tile.save(f"tile_{i:04d}.png")
    """
    from PIL import Image
    
    level = blb.header.get_level_by_index(level_index)
    sec_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
    entries = parse_container_toc(sec_data)
    
    # Find required assets
    asset100 = next((e for e in entries if e.asset_id == 100), None)
    asset300 = next((e for e in entries if e.asset_id == 300), None)
    asset400 = next((e for e in entries if e.asset_id == 400), None)
    
    if not all([asset100, asset300, asset400]):
        return []
    
    # Parse header for tile counts
    header = TileHeader.from_bytes(asset100.data)
    
    # Parse palettes
    palette_entries = parse_container_toc(asset400.data)
    if palette_index >= len(palette_entries):
        palette_index = 0
    palette = parse_psx_palette(palette_entries[palette_index].data)
    
    tiles = []
    pixel_offset = 0
    
    # Extract 16×16 tiles
    for i in range(header.count_16x16):
        tile_bytes = asset300.data[pixel_offset:pixel_offset + 256]
        indices = extract_tile_8bpp(tile_bytes, 16, 16)
        tiles.append(tile_to_image(indices, palette))
        pixel_offset += 256
    
    # Extract 8×8 tiles
    for i in range(header.count_8x8_a + header.count_8x8_b):
        tile_bytes = asset300.data[pixel_offset:pixel_offset + 64]
        indices = extract_tile_8bpp(tile_bytes, 8, 8)
        tiles.append(tile_to_image(indices, palette))
        pixel_offset += 64
    
    return tiles


if __name__ == "__main__":
    main()
