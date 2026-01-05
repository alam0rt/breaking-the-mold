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
    0x04       4  u32     primary_buffer_size Total buffer size for level (returned by GetPrimaryBufferSize @ 8007a5cc)
    0x08       4  u32     entry1_offset       Offset to Entry[1]/Asset 601 in primary TOC (CONFIRMED)
    
    # Level identification (0x0C-0x0D)
    0x0C       1  u8      level_index         Level asset index (used for asset loading)
    0x0D       1  u8      level_flag          Password-selectable flag (1=selectable via password, 0=not)
                                              8 levels have flag=1: SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED
                                              Used by InitGameState to build password level list
    
    # Stage count and tertiary data offsets (0x0E-0x1D)
    0x0E       2  u16     stage_count         Number of stages in this level (1-6)
    0x10      12  u16[6]  tert_data_offsets   Per-stage tertiary data offsets (value << 5 = size)
    0x1C       2  u16     (padding)           Always 0
    
    # Secondary sector locations (0x1E-0x39) - 6 stages as parallel arrays
    0x1E      12  u16[6]  sec_sector_offsets  Per-stage secondary sector offsets
    0x2A       2  u16     (padding)           Always 0
    0x2C      12  u16[6]  sec_sector_counts   Per-stage secondary sector counts
    0x38       2  u16     (padding)           Always 0
    
    # Tertiary sector locations (0x3A-0x55) - 6 stages as parallel arrays
    0x3A      12  u16[6]  tert_sector_offsets Per-stage tertiary sector offsets
    0x46       2  u16     (padding)           Always 0
    0x48      12  u16[6]  tert_sector_counts  Per-stage tertiary sector counts
    0x54       2  u16     (padding)           Always 0
    
    # Level strings (0x56-0x6F)
    0x56       5  str     level_id            4-char null-terminated (e.g., "MENU")
    0x5B      21  str     name                Level name, null-terminated
    
    Stage Data Organization:
    -----------------------
    Each level contains 1-6 stages. The sector arrays are parallel:
    - Stage i uses: sec_sector_offsets[i], sec_sector_counts[i] for secondary
    - Stage i uses: tert_sector_offsets[i], tert_sector_counts[i] for tertiary
    
    Data Interleaving Pattern:
    --------------------------
    Level data sectors are interleaved: PRIMARY → SECONDARY → TERT[0] → SEC_SUB[0] 
    → TERT[1] → SEC_SUB[1] → ... This creates an alternating pattern where tertiary 
    blocks and secondary sub-blocks occupy adjacent sector ranges.
    
    Example (MENU level): 201-232(PRI) → 233-299(SEC) → 300-787(T0) → 788-849(S0)
    → 850-852(T1) → 853-911(S1) → 912-917(T2) → 918-980(S2) → ...
    
    The tert_data_off_N values are byte offsets WITHIN each tertiary block, pointing
    to specific embedded data structures.
    
    Secondary/Tertiary Pairing (CRITICAL):
    --------------------------------------
    Each tertiary sub-block uses tiles from the secondary that PRECEDES it in the
    sector layout, NOT from its corresponding stage-indexed secondary. This is because
    the game loads data sequentially and the preceding secondary is in memory when
    the tertiary is processed.
    
    Sector order:  BASE_SEC → TERT[0] → SEC_SUB[0] → TERT[1] → SEC_SUB[1] → ...
    Pairing:       TERT[0] uses BASE_SEC
                   TERT[1] uses SEC_SUB[0]
                   TERT[2] uses SEC_SUB[1]
                   TERT[N] uses SEC_SUB[N-1] (for N > 0)
    
    Example (BOIL level):
        Base secondary (sectors 8053-8200) → 974 tiles
        Stage 0 tertiary (8201-8382) uses base secondary tiles
        Stage 0 secondary (8383-8524) → 783 tiles  
        Stage 1 tertiary (8525-8727) uses stage 0 secondary tiles
        ...
        Stage 5 tertiary (9881-9897) uses stage 4 secondary tiles


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
    See docs/blb-data-format.md for the COMPLETE 128-byte structure with all 32 fields.
    
    Summary of key fields:
    
    Offset  Size  Type    Field                   Description
    ------  ----  ----    -----                   -----------
    0x00-0x58     var     Asset pointers          Populated by LoadAssetContainer from sub-TOC
                                                  (IDs 100-700 map to specific offsets)
    0x5C    4     ptr     blbHeaderBuffer         Pointer to BLB header (→ 0x800AE3E0)
    0x60    1     u8      slidingWindowIndex      Playback sequence position (init 0xFF)
    0x64    4     ptr     loaderCallback          CD load callback (→ 0x80020848)
    0x68    4     ptr     primaryDataBuffer       Primary TOC buffer pointer
    0x6C    4     ptr     secondaryDataBuffer     Secondary container buffer
    0x70    4     ptr     primaryLevel600         Primary TOC ID 600 pointer (geometry)
    0x74    4     ptr     primaryCollision601     Primary TOC ID 601 pointer (collision)
    0x78    4     u32     primaryCollision601Size Primary collision size in bytes
    0x7C    4     ptr     primaryPalette602       Primary TOC ID 602 pointer (palette)
    
    Total structure size: 0x80 (128) bytes

    Verified runtime example (Science Centre / SCIE, level index 2):
      ctx @ 0x8009DCC4:
        blbHeaderBuffer = 0x800AE3E0 (BLB header)
        primaryDataBuffer = 0x800AF3E0 (loaded TOC, 3 entries)
        primaryLevel600 = 0x800AF408 (524,212 bytes geometry)
        primaryCollision601 = 0x8012F3BC (126,256 bytes collision)
        primaryPalette602 = 0x8014E0EC (148 bytes palette)

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


Available Classes and Types:
============================

    Enums & Flags:
    --------------
    AssetID         - IntEnum of all known asset type IDs (0x064=Header, 0x258=Sprites, etc.)
    TileFlags       - IntFlag for tile properties (SIZE_8X8, SEMI_TRANSPARENT, SKIP)
    TileFlagBits    - Constants class with same tile flag values
    SegmentType     - String constants for PRIMARY, SECONDARY, TERTIARY

    Header Classes:
    ---------------
    BLBHeader              - Main BLB file header with level table access
    LevelMetadataEntry     - Single level entry (0x70 bytes) from header
    MovieEntry             - Single movie entry (0x1C bytes) from header  
    SectorTableEntry       - Sector/loading screen entry from header
    CreditsEntry           - Credits sequence entry from header
    StateData              - Playback sequence state data

    Asset Container Classes:
    ------------------------
    SpriteContainer        - Container for sprites from Asset 600 (0x258)
    TilemapContainer       - Container for tilemaps from Asset 200 (0xC8)
    PaletteContainer       - Container for palettes from Asset 400 (0x190)
    TileData               - Combined tile data from Assets 100, 300-302, 400

    Sprite Classes:
    ---------------
    Sprite                 - Complete sprite with header, animations, frames
    SpriteFrameMetadata    - Frame positioning and size metadata (36 bytes)
    AnimationEntry         - Animation definition (12 bytes)
    SpriteHeader           - Sprite header with counts and offsets (12 bytes)
    RLECommand             - Parsed RLE decompression command

    Tile & Layer Classes:
    ---------------------
    TileHeader             - Tile header from Asset 100 (36 bytes)
    Tilemap                - Layer tilemap (array of tile indices)
    TilemapEntry           - Single tilemap entry (u16 with 11-bit index)
    LayerEntry             - Layer definition from Asset 201 (92 bytes)

    Color Classes:
    --------------
    Palette                - 256-color CLUT from Asset 400
    PSXColor               - Single 15-bit PSX color (XBGR1555)

    TOC Classes:
    ------------
    AssetTOCEntry          - TOC entry with asset_id, size, offset, data


Parsing Functions:
==================
    parse_container_toc()      - Parse container sub-TOC
    parse_layer_entries()      - Parse Asset 201 layer entries
    parse_tilemap_container()  - Parse Asset 200 tilemaps (deprecated, use TilemapContainer)
    parse_tilemap()            - Parse raw tilemap u16 array
    get_tile_index()           - Extract 0-based tile index from u16 value
    parse_palette_rgba()       - Parse PSX CLUT to RGBA tuples
    parse_palette_container()  - Parse Asset 400 palettes (deprecated, use PaletteContainer)
    extract_tiles_rgba()       - Extract tiles as PIL Images
    psx_color_to_rgb()         - Convert 15-bit color to RGB tuple
    parse_psx_palette()        - Parse CLUT to RGB tuples

"""

import struct
from dataclasses import dataclass, field
from enum import IntEnum, IntFlag
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


# =============================================================================
# Asset ID Constants
# =============================================================================
# These constants define the known asset type IDs found in BLB containers.
# Each segment type (Primary, Secondary, Tertiary) uses different subsets.

class AssetID:
    """
    Known asset type IDs in BLB containers.
    
    Asset IDs are u32 values that identify the type of data in a TOC entry.
    Different segments use different subsets of these IDs.
    """
    # Secondary Segment - Tile Data (VERIFIED)
    TILE_HEADER = 100       # 0x064 - Tile counts and level dimensions (36 bytes)
    TILE_HEADER_101 = 101   # 0x065 - Unknown geometry (12 bytes, sparse)
    TILEMAP_CONTAINER = 200 # 0x0C8 - Tilemap sub-TOC with layer data
    LAYER_ENTRIES = 201     # 0x0C9 - Layer definitions (92 bytes each)
    TILE_PIXELS = 300       # 0x12C - 8bpp indexed pixel data
    PALETTE_INDICES = 301   # 0x12D - Palette index per tile (1 byte each)
    TILE_FLAGS = 302        # 0x12E - Tile size/rendering flags (1 byte each)
    ANIMATED_TILES = 303    # 0x12F - Animated tile lookup table
    PALETTE_CONTAINER = 400 # 0x190 - Palette sub-TOC with 256-color CLUTs
    PALETTE_ANIM = 401      # 0x191 - Palette animation data
    
    # Primary Segment - Level Geometry (VERIFIED)
    GEOMETRY = 600          # 0x258 - Level geometry/RLE sprites container
    COLLISION = 601         # 0x259 - Collision/layout data container
    LEVEL_PALETTE = 602     # 0x25A - Level palette (15-bit PSX colors)
    
    # Tertiary Segment - Audio & Sprites
    # Note: Asset 100, 200, 201, 302, 401 may also appear in tertiary
    SPRITE_METADATA = 500   # 0x1F4 - Sprite metadata
    SPRITE_RLE = 501        # 0x1F5 - Sprite RLE pixel data
    AUDIO_502 = 502         # 0x1F6 - Audio configuration
    AUDIO_503 = 503         # 0x1F7 - Audio configuration
    SPU_SAMPLES = 700       # 0x2BC - SPU audio samples (ADPCM)
    
    # Hex equivalents for reference
    _HEX_MAP = {
        0x064: 'TILE_HEADER',
        0x065: 'TILE_HEADER_101',
        0x0C8: 'TILEMAP_CONTAINER',
        0x0C9: 'LAYER_ENTRIES',
        0x12C: 'TILE_PIXELS',
        0x12D: 'PALETTE_INDICES',
        0x12E: 'TILE_FLAGS',
        0x12F: 'ANIMATED_TILES',
        0x190: 'PALETTE_CONTAINER',
        0x191: 'PALETTE_ANIM',
        0x1F4: 'SPRITE_METADATA',
        0x1F5: 'SPRITE_RLE',
        0x1F6: 'AUDIO_502',
        0x1F7: 'AUDIO_503',
        0x258: 'GEOMETRY',
        0x259: 'COLLISION',
        0x25A: 'LEVEL_PALETTE',
        0x2BC: 'SPU_SAMPLES',
    }
    
    @classmethod
    def name(cls, asset_id: int) -> str:
        """Get the name for an asset ID."""
        return cls._HEX_MAP.get(asset_id, f'UNKNOWN_{asset_id:03X}')


class TileFlagBits:
    """
    Bit flags for tile rendering properties (Asset 302) - constant class.
    
    Verified via Ghidra decompilation of LoadTileDataToVRAM (0x80025240).
    Use TileFlags IntFlag for actual flag operations.
    """
    SEMI_TRANSPARENT = 0x01  # Bit 0: Enable GPU alpha blending
    SIZE_8X8 = 0x02          # Bit 1: Tile is 8×8 (not 16×16)
    SKIP = 0x04              # Bit 2: Don't load/render this tile


class TileFlags(IntFlag):
    """
    IntFlag version of tile rendering properties for flag operations.
    
    Verified via Ghidra decompilation of LoadTileDataToVRAM (0x80025240).
    """
    NONE = 0x00              # No flags
    SEMI_TRANSPARENT = 0x01  # Bit 0: Enable GPU alpha blending
    SIZE_8X8 = 0x02          # Bit 1: Tile is 8×8 (not 16×16)
    SKIP = 0x04              # Bit 2: Don't load/render this tile


class SegmentType:
    """
    BLB data segment types.
    
    Each level consists of three segments loaded from different sectors.
    """
    PRIMARY = 'primary'      # Level geometry, collision, palette
    SECONDARY = 'secondary'  # Tiles, tilemaps, layer definitions
    TERTIARY = 'tertiary'    # Sprites, audio, music


# =============================================================================
# Sprite Data Structures
# =============================================================================

@dataclass
class SpriteFrameMetadata:
    """
    Sprite frame metadata from Asset 600 sprite container.
    
    Each frame is 36 bytes (0x24), containing position/size info and RLE offset.
    Verified via Ghidra analysis of GetFrameMetadata (0x8007bebc).
    """
    flags: int              # u16 at +0x04: Frame flags (1 or 2)
    render_x: int           # s16 at +0x06: Render X offset (signed)
    render_y: int           # s16 at +0x08: Render Y offset (signed)
    render_width: int       # u16 at +0x0A: Visible width
    render_height: int      # u16 at +0x0C: Visible height
    anchor_x: int           # s16 at +0x12: Anchor X offset (signed)
    anchor_y: int           # s16 at +0x14: Anchor Y offset (signed)
    clip_width: int         # u16 at +0x16: Clip width
    clip_height: int        # u16 at +0x18: Clip height
    rle_offset: int         # u32 at +0x20: RLE data offset from sprite's RLE base
    raw_data: bytes = None
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "SpriteFrameMetadata":
        """Parse a 36-byte frame metadata entry."""
        if len(data) < 36:
            raise ValueError(f"Frame metadata too short: {len(data)} < 36")
        
        return cls(
            flags=struct.unpack('<H', data[0x04:0x06])[0],
            render_x=struct.unpack('<h', data[0x06:0x08])[0],  # Signed
            render_y=struct.unpack('<h', data[0x08:0x0A])[0],  # Signed
            render_width=struct.unpack('<H', data[0x0A:0x0C])[0],
            render_height=struct.unpack('<H', data[0x0C:0x0E])[0],
            anchor_x=struct.unpack('<h', data[0x12:0x14])[0],  # Signed
            anchor_y=struct.unpack('<h', data[0x14:0x16])[0],  # Signed
            clip_width=struct.unpack('<H', data[0x16:0x18])[0],
            clip_height=struct.unpack('<H', data[0x18:0x1A])[0],
            rle_offset=struct.unpack('<I', data[0x20:0x24])[0],
            raw_data=data[:36],
        )


@dataclass
class AnimationEntry:
    """
    Animation definition from Asset 600 sprite container.
    
    Each animation is 12 bytes, defining a group of frames.
    Located at sprite offset 0x0C + anim_index × 12.
    """
    animation_id: int       # u32 at +0x00: Animation type identifier
    frame_count: int        # u16 at +0x04: Number of frames
    frame_offset: int       # u16 at +0x06: Index into frame metadata table
    flags: int              # u16 at +0x08: Animation properties
    extra: int              # u16 at +0x0A: Unknown (often 0)
    raw_data: bytes = None
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "AnimationEntry":
        """Parse a 12-byte animation entry."""
        if len(data) < 12:
            raise ValueError(f"Animation entry too short: {len(data)} < 12")
        
        return cls(
            animation_id=struct.unpack('<I', data[0x00:0x04])[0],
            frame_count=struct.unpack('<H', data[0x04:0x06])[0],
            frame_offset=struct.unpack('<H', data[0x06:0x08])[0],
            flags=struct.unpack('<H', data[0x08:0x0A])[0],
            extra=struct.unpack('<H', data[0x0A:0x0C])[0],
            raw_data=data[:12],
        )


@dataclass
class SpriteHeader:
    """
    Sprite definition header from Asset 600 container.
    
    The first 12 bytes of each sprite define its structure.
    Verified via Ghidra analysis of InitSpriteContext (0x8007bc3c).
    """
    animation_count: int    # u16 at +0x00: Number of animation groups
    frame_meta_offset: int  # u16 at +0x02: Offset to frame metadata table
    rle_offset: int         # u32 at +0x04: Offset to RLE pixel data
    palette_offset: int     # u32 at +0x08: Offset to embedded 256-color palette
    raw_data: bytes = None
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "SpriteHeader":
        """Parse a 12-byte sprite header."""
        if len(data) < 12:
            raise ValueError(f"Sprite header too short: {len(data)} < 12")
        
        return cls(
            animation_count=struct.unpack('<H', data[0x00:0x02])[0],
            frame_meta_offset=struct.unpack('<H', data[0x02:0x04])[0],
            rle_offset=struct.unpack('<I', data[0x04:0x08])[0],
            palette_offset=struct.unpack('<I', data[0x08:0x0C])[0],
            raw_data=data[:12],
        )


@dataclass
class Sprite:
    """
    Complete sprite definition including header, animations, and frames.
    
    Represents a single sprite entry from Asset 600 (primary or tertiary).
    """
    sprite_id: int                          # Sprite ID from TOC entry
    header: SpriteHeader                    # Sprite header (12 bytes)
    animations: list[AnimationEntry]        # Animation definitions
    frames: list[SpriteFrameMetadata]       # Frame metadata entries
    palette: list[tuple] = None             # 256-color RGBA palette
    raw_data: bytes = None                  # Full sprite data
    
    @classmethod
    def from_bytes(cls, sprite_id: int, data: bytes) -> "Sprite":
        """Parse a complete sprite from raw data."""
        header = SpriteHeader.from_bytes(data)
        
        # Parse animations (starting at offset 0x0C)
        animations = []
        for i in range(header.animation_count):
            anim_offset = 0x0C + i * 12
            if anim_offset + 12 > len(data):
                break
            animations.append(AnimationEntry.from_bytes(data[anim_offset:anim_offset + 12]))
        
        # Parse frame metadata
        frames = []
        if animations:
            # Calculate total frame count from animations
            max_frame = max(a.frame_offset + a.frame_count for a in animations)
            for i in range(max_frame):
                frame_offset = header.frame_meta_offset + i * 36
                if frame_offset + 36 > len(data):
                    break
                frames.append(SpriteFrameMetadata.from_bytes(data[frame_offset:frame_offset + 36]))
        
        # Parse embedded palette (512 bytes at palette_offset)
        palette = None
        if header.palette_offset + 512 <= len(data):
            palette_data = data[header.palette_offset:header.palette_offset + 512]
            palette = parse_palette_rgba(palette_data)
        
        return cls(
            sprite_id=sprite_id,
            header=header,
            animations=animations,
            frames=frames,
            palette=palette,
            raw_data=data,
        )
    
    @property
    def total_frames(self) -> int:
        """Total number of frames across all animations."""
        return len(self.frames)


@dataclass
class RLECommand:
    """
    RLE command for sprite decompression.
    
    Each command is a u16 with the following bit layout:
    - Bit 15: New line flag (advance to next row)
    - Bits 14-8: Skip count (transparent pixels)
    - Bits 7-0: Copy count (literal pixels to copy)
    """
    new_line: bool          # Bit 15: Start new row
    skip_count: int         # Bits 14-8: Transparent pixel count
    copy_count: int         # Bits 7-0: Literal pixel count
    
    @classmethod
    def from_u16(cls, value: int) -> "RLECommand":
        """Parse a u16 RLE command."""
        return cls(
            new_line=bool(value & 0x8000),
            skip_count=(value >> 8) & 0x7F,
            copy_count=value & 0xFF,
        )


@dataclass
class SpriteTOCEntry:
    """
    Sprite entry from Asset 600 sub-TOC.
    
    Each entry is 12 bytes:
    - u32: Sprite ID (hash-like lookup key)
    - u32: Data size in bytes
    - u32: Data offset from asset start
    """
    sprite_id: int
    size: int
    offset: int
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "SpriteTOCEntry":
        """Parse a 12-byte sprite TOC entry."""
        if len(data) < 12:
            raise ValueError(f"Sprite TOC entry too short: {len(data)} < 12")
        return cls(
            sprite_id=struct.unpack('<I', data[0:4])[0],
            size=struct.unpack('<I', data[4:8])[0],
            offset=struct.unpack('<I', data[8:12])[0],
        )


@dataclass
class SpriteContainer:
    """
    Asset 600 (0x258) sprite container from Primary or Tertiary segments.
    
    Contains RLE-encoded sprites with embedded palettes.
    Verified via Ghidra analysis of FindSpriteInTOC (0x8007b968).
    
    Container Structure:
        0x00    u32     Sprite count (N)
        0x04    N×12    Sprite header table
        var             Sprite data blocks
    """
    count: int
    entries: list[SpriteTOCEntry]
    sprites: dict[int, Sprite] = None  # Keyed by sprite_id
    raw_data: bytes = None
    
    @classmethod
    def from_bytes(cls, data: bytes, parse_sprites: bool = False) -> "SpriteContainer":
        """
        Parse a sprite container.
        
        Args:
            data: Raw asset data
            parse_sprites: If True, also parse all sprite definitions
        """
        if len(data) < 4:
            return cls(count=0, entries=[], raw_data=data)
        
        count = struct.unpack('<I', data[0:4])[0]
        
        # Sanity check
        if count > 1000 or count * 12 + 4 > len(data):
            return cls(count=0, entries=[], raw_data=data)
        
        entries = []
        for i in range(count):
            entry_offset = 4 + i * 12
            entries.append(SpriteTOCEntry.from_bytes(data[entry_offset:entry_offset + 12]))
        
        sprites = None
        if parse_sprites:
            sprites = {}
            for entry in entries:
                if entry.offset + entry.size <= len(data):
                    sprite_data = data[entry.offset:entry.offset + entry.size]
                    try:
                        sprites[entry.sprite_id] = Sprite.from_bytes(entry.sprite_id, sprite_data)
                    except Exception:
                        pass  # Skip malformed sprites
        
        return cls(count=count, entries=entries, sprites=sprites, raw_data=data)
    
    def get_sprite(self, sprite_id: int) -> Optional[Sprite]:
        """Get a sprite by its ID."""
        if self.sprites and sprite_id in self.sprites:
            return self.sprites[sprite_id]
        
        # Parse on demand if not pre-parsed
        for entry in self.entries:
            if entry.sprite_id == sprite_id:
                if self.raw_data and entry.offset + entry.size <= len(self.raw_data):
                    sprite_data = self.raw_data[entry.offset:entry.offset + entry.size]
                    return Sprite.from_bytes(sprite_id, sprite_data)
        return None
    
    def __len__(self) -> int:
        return self.count
    
    def __iter__(self):
        return iter(self.entries)


@dataclass
class PSXColor:
    """
    PSX 15-bit color (XBGR1555 format).
    
    Stored as u16 little-endian:
    - Bits 0-4: Red (0-31)
    - Bits 5-9: Green (0-31)
    - Bits 10-14: Blue (0-31)
    - Bit 15: STP (semi-transparency processing)
    """
    r: int  # Red (0-31, multiply by 8 for 8-bit)
    g: int  # Green (0-31)
    b: int  # Blue (0-31)
    stp: bool  # Semi-transparency flag
    
    @classmethod
    def from_u16(cls, value: int) -> "PSXColor":
        """Parse a 15-bit PSX color from u16."""
        return cls(
            r=value & 0x1F,
            g=(value >> 5) & 0x1F,
            b=(value >> 10) & 0x1F,
            stp=bool(value & 0x8000),
        )
    
    def to_rgb(self) -> tuple:
        """Convert to 8-bit RGB tuple."""
        return (self.r * 8, self.g * 8, self.b * 8)
    
    def to_rgba(self, transparent_if_zero: bool = True) -> tuple:
        """
        Convert to 8-bit RGBA tuple.
        
        Args:
            transparent_if_zero: If True, color 0x0000 is treated as transparent
        """
        # More accurate 5-bit to 8-bit conversion
        r = (self.r * 255) // 31 if self.r else 0
        g = (self.g * 255) // 31 if self.g else 0
        b = (self.b * 255) // 31 if self.b else 0
        
        # Determine alpha
        is_black = (self.r == 0 and self.g == 0 and self.b == 0)
        if transparent_if_zero and is_black and not self.stp:
            a = 0
        else:
            a = 255
        
        return (r, g, b, a)


@dataclass
class Palette:
    """
    256-color palette (CLUT) from Asset 400 container.
    
    Each palette is 512 bytes (256 × u16 PSX colors).
    Color 0 is typically transparent.
    """
    colors: list[PSXColor]
    raw_data: bytes = None
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "Palette":
        """Parse a 512-byte palette."""
        colors = []
        for i in range(min(256, len(data) // 2)):
            value = struct.unpack('<H', data[i*2:i*2+2])[0]
            colors.append(PSXColor.from_u16(value))
        
        # Pad to 256 if needed
        while len(colors) < 256:
            colors.append(PSXColor(0, 0, 0, False))
        
        return cls(colors=colors, raw_data=data[:512] if len(data) >= 512 else data)
    
    def to_rgba_list(self) -> list[tuple]:
        """Convert to list of RGBA tuples (for PIL)."""
        return [c.to_rgba() for c in self.colors]
    
    def __getitem__(self, index: int) -> PSXColor:
        return self.colors[index]
    
    def __len__(self) -> int:
        return len(self.colors)


# =============================================================================
# Tilemap Structures
# =============================================================================

@dataclass
class TilemapEntry:
    """
    Single tilemap entry (u16 value) from Asset 200.
    
    Tile Index Format (16 bits):
    - Bits 0-10: Tile index (11 bits, 1-based, 0 = transparent)
    - Bits 11-15: Unused (tiles are not flipped in game)
    """
    raw_value: int
    
    @property
    def tile_index(self) -> int:
        """Get 0-based tile index, or -1 for empty."""
        idx = self.raw_value & 0x7FF
        return idx - 1 if idx > 0 else -1
    
    @property
    def is_empty(self) -> bool:
        """Check if this is an empty/transparent tile."""
        return (self.raw_value & 0x7FF) == 0
    
    @property
    def is_entity(self) -> bool:
        """Check if this might be an entity spawn marker (bit 12+ set)."""
        return (self.raw_value & 0xF800) != 0


@dataclass
class Tilemap:
    """
    Layer tilemap data from Asset 200 container.
    
    A tilemap is an array of u16 tile indices defining tile placement for one layer.
    """
    layer_index: int
    entries: list[TilemapEntry]
    raw_data: bytes = None
    
    @classmethod
    def from_bytes(cls, layer_index: int, data: bytes) -> "Tilemap":
        """Parse tilemap data."""
        entries = []
        for i in range(len(data) // 2):
            value = struct.unpack('<H', data[i*2:i*2+2])[0]
            entries.append(TilemapEntry(value))
        return cls(layer_index=layer_index, entries=entries, raw_data=data)
    
    def get_tile_at(self, x: int, y: int, width: int) -> TilemapEntry:
        """Get tile entry at grid position."""
        idx = y * width + x
        if 0 <= idx < len(self.entries):
            return self.entries[idx]
        return TilemapEntry(0)
    
    def __len__(self) -> int:
        return len(self.entries)
    
    def __getitem__(self, index: int) -> TilemapEntry:
        return self.entries[index]


@dataclass
class TilemapContainer:
    """
    Container for all layer tilemaps from Asset 200 (0xC8).
    
    The container has an internal sub-TOC with one entry per layer.
    Each tilemap is an array of u16 tile indices for that layer.
    """
    tilemaps: list[Tilemap]
    count: int
    raw_data: bytes = None
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "TilemapContainer":
        """Parse tilemap container from Asset 200 data."""
        if len(data) < 4:
            return cls(tilemaps=[], count=0, raw_data=data)
        
        count = struct.unpack('<I', data[0:4])[0]
        tilemaps = []
        
        for i in range(count):
            entry_offset = 4 + i * 12
            if entry_offset + 12 > len(data):
                break
            
            # Sub-TOC entry: layer_idx (u32), size (u32), offset (u32)
            _, size, data_offset = struct.unpack('<III', 
                data[entry_offset:entry_offset + 12])
            
            if data_offset + size <= len(data):
                tilemap_data = data[data_offset:data_offset + size]
                tilemaps.append(Tilemap.from_bytes(i, tilemap_data))
            else:
                tilemaps.append(Tilemap(layer_index=i, entries=[], raw_data=b''))
        
        return cls(tilemaps=tilemaps, count=count, raw_data=data)
    
    def __len__(self) -> int:
        return len(self.tilemaps)
    
    def __getitem__(self, index: int) -> Tilemap:
        return self.tilemaps[index]
    
    def __iter__(self):
        return iter(self.tilemaps)


@dataclass
class PaletteContainer:
    """
    Container for all palettes from Asset 400 (0x190).
    
    The container has an internal sub-TOC with one entry per palette.
    Each palette is 512 bytes (256 × 16-bit PSX colors).
    """
    palettes: list[Palette]
    count: int
    raw_data: bytes = None
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "PaletteContainer":
        """Parse palette container from Asset 400 data."""
        entries = parse_container_toc(data)
        palettes = []
        
        for entry in entries:
            if entry.data and len(entry.data) >= 512:
                palettes.append(Palette.from_bytes(entry.data))
            else:
                # Empty palette
                palettes.append(Palette(colors=[PSXColor(0, 0, 0, False)] * 256))
        
        return cls(palettes=palettes, count=len(palettes), raw_data=data)
    
    def __len__(self) -> int:
        return len(self.palettes)
    
    def __getitem__(self, index: int) -> Palette:
        if 0 <= index < len(self.palettes):
            return self.palettes[index]
        # Return grayscale fallback
        return Palette(colors=[PSXColor(i, i, i, i > 0) for i in range(256)])
    
    def __iter__(self):
        return iter(self.palettes)
    
    def to_rgba_lists(self) -> list[list[tuple]]:
        """Convert all palettes to RGBA tuple lists."""
        return [p.to_rgba_list() for p in self.palettes]


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
    
    # Primary level data location (0x00-0x03)
    sector_offset: int = None  # u16 at +0x00, sector offset in BLB
    sector_count: int = None  # u16 at +0x02, sector count
    
    # Primary buffer parameters (0x04-0x0B) - CONFIRMED as u32s
    primary_buffer_size: int = None  # u32 at +0x04: Total buffer size for level (returned by GetPrimaryBufferSize @ 8007a5cc)
    entry1_offset: int = None  # u32 at +0x08: Offset to Entry[1]/Asset 601 in primary TOC (CONFIRMED)
    
    # Level identification (0x0C-0x0D)
    level_index: int = None  # u8 at +0x0C: Level asset index
    level_flag: int = unknown("+0x0D: u8, 0 or 1, purpose TBD")
    
    # Stage count and tertiary data offsets (0x0E-0x1D)
    # Stage count (tert_block_count) indicates how many stages this level has (1-6)
    # tert_data_offsets[i] << 5 gives tertiary data size for stage i (used by GetCurrentTertiaryDataSize)
    stage_count: int = None  # u16 at +0x0E: Number of stages in this level (1-6)
    tert_data_offsets: list = None  # 6× u16 at +0x10-0x1A: Per-stage tertiary data offsets
    # +0x1C-0x1D: Padding (always 0)
    
    # Secondary sector locations (0x1E-0x39) - 6 stages as parallel arrays
    # sec_sector_offsets[i] and sec_sector_counts[i] define stage i's secondary data
    sec_sector_offsets: list = None  # 6× u16 at +0x1E-0x28: Per-stage secondary sector offsets
    # +0x2A-0x2B: Padding (always 0)
    sec_sector_counts: list = None  # 6× u16 at +0x2C-0x36: Per-stage secondary sector counts
    # +0x38-0x39: Padding (always 0)
    
    # Tertiary sector locations (0x3A-0x55) - 6 stages as parallel arrays
    tert_sector_offsets: list = None  # 6× u16 at +0x3A-0x44: Per-stage tertiary sector offsets
    # +0x46-0x47: Padding (always 0)
    tert_sector_counts: list = None  # 6× u16 at +0x48-0x52: Per-stage tertiary sector counts
    # +0x54-0x55: Padding (always 0)
    
    # Identification (0x56-0x6F)
    level_id: str = None  # 5 bytes at +0x56 (4-char null-terminated)
    name: str = None  # 21 bytes at +0x5B (null-terminated)
    
    # =========================================================================
    # Legacy aliases for backward compatibility
    # =========================================================================
    
    @property
    def tert_block_count(self) -> int:
        """Legacy alias - use stage_count instead."""
        return self.stage_count
    
    @property
    def secondary_offset(self) -> int:
        """Legacy alias - use sec_sector_offsets[0] instead."""
        return self.sec_sector_offsets[0] if self.sec_sector_offsets else 0
    
    @property
    def sec_sub_offsets(self) -> list:
        """Legacy alias - use sec_sector_offsets[1:6] instead."""
        return self.sec_sector_offsets[1:6] if self.sec_sector_offsets else [0, 0, 0, 0, 0]
    
    @property
    def secondary_count(self) -> int:
        """Legacy alias - use sec_sector_counts[0] instead."""
        return self.sec_sector_counts[0] if self.sec_sector_counts else 0
    
    @property
    def sec_sub_counts(self) -> list:
        """Legacy alias - use sec_sector_counts[1:6] instead."""
        return self.sec_sector_counts[1:6] if self.sec_sector_counts else [0, 0, 0, 0, 0]
    
    @property
    def tert_sub_offsets(self) -> list:
        """Legacy alias - use tert_sector_offsets instead."""
        return self.tert_sector_offsets
    
    @property
    def tert_sub_counts(self) -> list:
        """Legacy alias - use tert_sector_counts instead."""
        return self.tert_sector_counts
    
    @property
    def unknown_04(self) -> int:
        """Legacy alias - use primary_buffer_size instead."""
        return self.primary_buffer_size & 0xFFFF
    
    @property
    def unknown_06(self) -> int:
        """Legacy alias - use primary_buffer_size instead."""
        return (self.primary_buffer_size >> 16) & 0xFFFF
    
    @property
    def entry1_offset_lo(self) -> int:
        """Legacy alias - use entry1_offset instead."""
        return self.entry1_offset & 0xFFFF
    
    @property
    def unknown_0A(self) -> int:
        """Legacy alias - use entry1_offset instead."""
        return (self.entry1_offset >> 16) & 0xFFFF
    
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
        
        # Primary buffer parameters (0x04-0x0B) - u32 values
        primary_buffer_size = struct.unpack('<I', data[0x04:0x08])[0]  # Total buffer size for level
        entry1_offset = struct.unpack('<I', data[0x08:0x0C])[0]  # Offset to Entry[1]/Asset 601
        
        # Level identification (0x0C-0x0D)
        level_index = data[0x0C]
        level_flag = data[0x0D]
        
        # Stage count and tertiary data offsets (0x0E-0x1D)
        stage_count = struct.unpack('<H', data[0x0E:0x10])[0]
        tert_data_offsets = [struct.unpack('<H', data[0x10 + i*2:0x12 + i*2])[0] for i in range(6)]
        # 0x1C-0x1D is padding
        
        # Secondary sector locations (0x1E-0x39) - 6-element arrays
        sec_sector_offsets = [struct.unpack('<H', data[0x1E + i*2:0x20 + i*2])[0] for i in range(6)]
        # 0x2A-0x2B is padding
        sec_sector_counts = [struct.unpack('<H', data[0x2C + i*2:0x2E + i*2])[0] for i in range(6)]
        # 0x38-0x39 is padding
        
        # Tertiary sector locations (0x3A-0x55) - 6-element arrays
        tert_sector_offsets = [struct.unpack('<H', data[0x3A + i*2:0x3C + i*2])[0] for i in range(6)]
        # 0x46-0x47 is padding
        tert_sector_counts = [struct.unpack('<H', data[0x48 + i*2:0x4A + i*2])[0] for i in range(6)]
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
            primary_buffer_size=primary_buffer_size,
            entry1_offset=entry1_offset,
            level_index=level_index,
            level_flag=level_flag,
            stage_count=stage_count,
            tert_data_offsets=tert_data_offsets,
            sec_sector_offsets=sec_sector_offsets,
            sec_sector_counts=sec_sector_counts,
            tert_sector_offsets=tert_sector_offsets,
            tert_sector_counts=tert_sector_counts,
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
    Tile header from Asset 100 (0x064), 36 bytes.
    
    Contains level dimensions, tile counts, and background colors.
    Verified via Ghidra decompilation of CopyTilePixelData (0x8007b588)
    and GetTotalTileCount (0x8007b53c).
    
    Offset  Size  Type   Description
    ------  ----  ----   -----------
    0x00    3     u8[3]  Background RGB color
    0x03    1     u8     Padding
    0x04    3     u8[3]  Secondary RGB color  
    0x07    1     u8     Padding
    0x08    2     u16    Level width in tiles
    0x0A    2     u16    Level height in tiles
    0x0C    4     -      Unknown
    0x10    2     u16    16×16 tile count
    0x12    2     u16    8×8 tile count (primary)
    0x14    2     u16    8×8 tile count (secondary, often 0)
    0x16    6     -      Unknown
    0x1C    2     u16    Unknown field (read by GetAsset100Field1C)
    0x1E    2     -      Remaining header data
    """
    raw_data: bytes
    bg_r: int = 0           # Background red (0-255)
    bg_g: int = 0           # Background green
    bg_b: int = 0           # Background blue
    secondary_r: int = 0    # Secondary color red
    secondary_g: int = 0    # Secondary color green
    secondary_b: int = 0    # Secondary color blue
    level_width: int = 0    # Level width in tiles
    level_height: int = 0   # Level height in tiles
    count_16x16: int = 0    # Number of 16×16 tiles
    count_8x8_a: int = 0    # Primary 8×8 tile count
    count_8x8_b: int = 0    # Secondary 8×8 tile count (often 0)
    field_1c: int = 0       # Unknown field at 0x1C
    
    @property
    def total_tiles(self) -> int:
        """Total tile count (used by GetTotalTileCount)."""
        return self.count_16x16 + self.count_8x8_a + self.count_8x8_b
    
    @property
    def level_width_pixels(self) -> int:
        """Level width in pixels."""
        return self.level_width * 16
    
    @property
    def level_height_pixels(self) -> int:
        """Level height in pixels."""
        return self.level_height * 16
    
    @property
    def background_color(self) -> tuple:
        """Background color as RGB tuple."""
        return (self.bg_r, self.bg_g, self.bg_b)
    
    @property
    def secondary_color(self) -> tuple:
        """Secondary color as RGB tuple."""
        return (self.secondary_r, self.secondary_g, self.secondary_b)
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "TileHeader":
        if len(data) < 0x1E:
            raise ValueError(f"Header too short: {len(data)} bytes (need 30)")
        
        return cls(
            raw_data=data,
            bg_r=data[0x00],
            bg_g=data[0x01],
            bg_b=data[0x02],
            secondary_r=data[0x04],
            secondary_g=data[0x05],
            secondary_b=data[0x06],
            level_width=struct.unpack('<H', data[0x08:0x0A])[0],
            level_height=struct.unpack('<H', data[0x0A:0x0C])[0],
            count_16x16=struct.unpack('<H', data[0x10:0x12])[0],
            count_8x8_a=struct.unpack('<H', data[0x12:0x14])[0],
            count_8x8_b=struct.unpack('<H', data[0x14:0x16])[0],
            field_1c=struct.unpack('<H', data[0x1C:0x1E])[0] if len(data) >= 0x1E else 0,
        )


@dataclass
class TileData:
    """
    Combined tile data from Secondary segment assets.
    
    Combines:
    - Asset 100 (0x064): TileHeader - tile counts, dimensions, colors
    - Asset 300 (0x12C): Pixel data - raw 8bpp tile pixels
    - Asset 301 (0x12D): Palette indices - which palette each tile uses
    - Asset 302 (0x12E): Flags - tile properties (8x8, transparent, etc.)
    - Asset 400 (0x190): Palettes - color lookup tables
    
    This class provides convenience methods to extract rendered tiles.
    """
    header: TileHeader
    pixel_data: bytes       # Asset 300: raw 8bpp pixels
    palette_indices: bytes  # Asset 301: palette assignments per tile
    flags: bytes            # Asset 302: tile flags
    palettes: PaletteContainer
    
    @property
    def total_tiles(self) -> int:
        """Total number of tiles."""
        return self.header.total_tiles
    
    @property
    def count_16x16(self) -> int:
        """Number of 16×16 tiles."""
        return self.header.count_16x16
    
    @property
    def count_8x8(self) -> int:
        """Number of 8×8 tiles."""
        return self.header.count_8x8_a + self.header.count_8x8_b
    
    def get_tile_flags(self, tile_index: int) -> TileFlags:
        """Get flags for a specific tile."""
        if 0 <= tile_index < len(self.flags):
            return TileFlags(self.flags[tile_index])
        return TileFlags(0)
    
    def get_tile_palette_index(self, tile_index: int) -> int:
        """Get palette index for a specific tile."""
        if 0 <= tile_index < len(self.palette_indices):
            return self.palette_indices[tile_index]
        return 0
    
    def is_8x8_tile(self, tile_index: int) -> bool:
        """Check if tile is 8×8 (vs 16×16)."""
        flags = self.get_tile_flags(tile_index)
        return bool(flags & TileFlags.SIZE_8X8)
    
    @classmethod
    def from_assets(cls, assets: dict) -> "TileData":
        """
        Create TileData from asset dictionary.
        
        Args:
            assets: Dict with keys AssetID.HEADER, AssetID.TILE_PIXELS, etc.
                    or numeric keys 0x064, 0x12C, etc.
        """
        # Handle both AssetID enum and int keys
        def get_asset(key):
            if isinstance(key, AssetID):
                return assets.get(key) or assets.get(key.value)
            return assets.get(key)
        
        header_data = get_asset(AssetID.HEADER) or b''
        pixel_data = get_asset(AssetID.TILE_PIXELS) or b''
        palette_indices = get_asset(AssetID.PALETTE_INDICES) or b''
        flags = get_asset(AssetID.TILE_FLAGS) or b''
        palette_data = get_asset(AssetID.PALETTE_CONTAINER) or b''
        
        return cls(
            header=TileHeader.from_bytes(header_data) if len(header_data) >= 0x1E else TileHeader(raw_data=b''),
            pixel_data=pixel_data,
            palette_indices=palette_indices,
            flags=flags,
            palettes=PaletteContainer.from_bytes(palette_data) if palette_data else PaletteContainer([], 0),
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


# =============================================================================
# Layer and Tilemap Parsing
# =============================================================================

@dataclass
class LayerEntry:
    """
    Layer definition from Asset 201 (0xC9), 92 bytes each.
    
    Verified via Ghidra analysis and extraction testing.
    
    Offset  Size   Type   Description
    ------  ----   ----   -----------
    0x00    2      u16    X offset (in tiles)
    0x02    2      u16    Y offset (in tiles)
    0x04    2      u16    Layer width (in tiles)
    0x06    2      u16    Layer height (in tiles)
    0x08    2      u16    Level width (in tiles, from Asset 100)
    0x0A    2      u16    Level height (in tiles)
    0x0C    4      -      Unknown
    0x10    4      u32    Scroll factor X (0x10000 = 1.0, 0x8000 = 0.5)
    0x14    4      u32    Scroll factor Y
    0x18-0x25      -      Unknown
    0x26    1      u8     Layer type (0=normal, 2=foreground?)
    0x27-0x2B      -      Unknown
    0x2C    1      u8     Background R
    0x2D    1      u8     Background G
    0x2E    1      u8     Background B
    0x2F-0x5B      -      Padding/unknown
    """
    x_offset: int       # X position in tiles
    y_offset: int       # Y position in tiles
    width: int          # Layer width in tiles
    height: int         # Layer height in tiles
    level_width: int    # Level width in tiles (from Asset 100)
    level_height: int   # Level height in tiles
    scroll_x: int       # X scroll factor (0x10000 = 1.0)
    scroll_y: int       # Y scroll factor
    layer_type: int     # Layer type (0=normal, 2=foreground?)
    bg_r: int           # Background R
    bg_g: int           # Background G
    bg_b: int           # Background B
    raw_data: bytes     # Full 92 bytes for debugging
    
    @property
    def scroll_factor_x(self) -> float:
        """Get X scroll factor as float (1.0 = same speed as camera)."""
        return self.scroll_x / 0x10000 if self.scroll_x else 0.0
    
    @property
    def scroll_factor_y(self) -> float:
        """Get Y scroll factor as float (1.0 = same speed as camera)."""
        return self.scroll_y / 0x10000 if self.scroll_y else 0.0
    
    @property
    def is_parallax(self) -> bool:
        """Check if this is a parallax layer (scroll != 1.0)."""
        return self.scroll_x != 0x10000 or self.scroll_y != 0x10000
    
    @property
    def background_color(self) -> tuple:
        """Background color as RGB tuple."""
        return (self.bg_r, self.bg_g, self.bg_b)
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "LayerEntry":
        if len(data) < 92:
            raise ValueError(f"Layer entry too short: {len(data)} bytes")
        return cls(
            x_offset=struct.unpack('<H', data[0x00:0x02])[0],
            y_offset=struct.unpack('<H', data[0x02:0x04])[0],
            width=struct.unpack('<H', data[0x04:0x06])[0],
            height=struct.unpack('<H', data[0x06:0x08])[0],
            level_width=struct.unpack('<H', data[0x08:0x0A])[0],
            level_height=struct.unpack('<H', data[0x0A:0x0C])[0],
            scroll_x=struct.unpack('<I', data[0x10:0x14])[0],
            scroll_y=struct.unpack('<I', data[0x14:0x18])[0],
            layer_type=data[0x26] if len(data) > 0x26 else 0,
            bg_r=data[0x2C] if len(data) > 0x2C else 0,
            bg_g=data[0x2D] if len(data) > 0x2D else 0,
            bg_b=data[0x2E] if len(data) > 0x2E else 0,
            raw_data=data[:92],
        )


def parse_layer_entries(asset201_data: bytes) -> list[LayerEntry]:
    """
    Parse layer entries from Asset 201 (0xC9).
    
    Each layer entry is 92 bytes.
    """
    entries = []
    num_layers = len(asset201_data) // 92
    
    for i in range(num_layers):
        offset = i * 92
        entry = LayerEntry.from_bytes(asset201_data[offset:offset + 92])
        entries.append(entry)
    
    return entries


def parse_tilemap_container(asset200_data: bytes) -> list[bytes]:
    """
    Parse tilemap container from Asset 200 (0xC8).
    
    Returns list of raw tilemap data (array of u16 tile indices).
    """
    if len(asset200_data) < 4:
        return []
    
    count = struct.unpack('<I', asset200_data[0:4])[0]
    tilemaps = []
    
    for i in range(count):
        entry_offset = 4 + i * 12
        if entry_offset + 12 > len(asset200_data):
            break
        
        # Sub-TOC entry: layer_idx (u32), size (u32), offset (u32)
        _, size, data_offset = struct.unpack('<III', 
            asset200_data[entry_offset:entry_offset + 12])
        
        if data_offset + size <= len(asset200_data):
            tilemaps.append(asset200_data[data_offset:data_offset + size])
        else:
            tilemaps.append(b'')
    
    return tilemaps


def parse_tilemap(tilemap_data: bytes) -> list[int]:
    """
    Parse raw tilemap data into list of tile indices.
    
    Each tile is a u16 value where bits 0-10 are the tile index (1-based, 0=empty).
    Bits 11-15 are unused (tiles are not flipped in game).
    """
    tiles = []
    for i in range(len(tilemap_data) // 2):
        tile_val = struct.unpack('<H', tilemap_data[i*2:i*2+2])[0]
        tiles.append(tile_val)
    return tiles


def get_tile_index(tile_val: int) -> int:
    """
    Extract tile index from tilemap u16 value.
    
    Returns 0-based index, or -1 for empty tiles.
    """
    idx = tile_val & 0x7FF  # Lower 11 bits, 1-based
    return idx - 1 if idx > 0 else -1


# =============================================================================
# Palette Parsing with RGBA Support
# =============================================================================

def parse_palette_rgba(data: bytes, num_colors: int = 256) -> list[tuple]:
    """
    Parse a PSX CLUT to RGBA tuples with transparency support.
    
    Color 0 is treated as transparent. Other colors are fully opaque.
    Uses proper 255-scale conversion from 5-bit components.
    
    Returns:
        List of (R, G, B, A) tuples
    """
    colors = []
    max_colors = min(num_colors, len(data) // 2)
    
    for i in range(max_colors):
        color = struct.unpack('<H', data[i*2:i*2+2])[0]
        r = ((color & 0x1F) * 255) // 31
        g = (((color >> 5) & 0x1F) * 255) // 31
        b = (((color >> 10) & 0x1F) * 255) // 31
        # Color 0 is transparent unless STP bit is set
        a = 255 if i > 0 or (color & 0x8000) else 0
        colors.append((r, g, b, a))
    
    # Pad to requested size
    while len(colors) < num_colors:
        colors.append((0, 0, 0, 0))
    
    return colors


def parse_palette_container(asset400_data: bytes) -> list[list[tuple]]:
    """
    Parse palette container from Asset 400 (0x190).
    
    Returns list of RGBA palettes (256 colors each).
    """
    entries = parse_container_toc(asset400_data)
    palettes = []
    
    for entry in entries:
        if entry.data and len(entry.data) >= 512:
            palettes.append(parse_palette_rgba(entry.data))
        else:
            # Fallback to grayscale
            palettes.append([(i, i, i, 255 if i > 0 else 0) for i in range(256)])
    
    return palettes


# =============================================================================
# Tile Extraction with Size Flag Support
# =============================================================================

def extract_tiles_rgba(pixel_data: bytes, palette_indices: bytes, palettes: list[list[tuple]],
                       flags_data: bytes, count_16x16: int, count_8x8: int,
                       scale_8x8: bool = True) -> list["Image"]:
    """
    Extract all tiles as RGBA PIL Images with proper size handling.
    
    Args:
        pixel_data: Asset 300 raw pixel data (8bpp)
        palette_indices: Asset 301 palette assignments (1 byte per tile)
        palettes: List of RGBA palettes from parse_palette_container()
        flags_data: Asset 302 tile size flags (1 byte per tile)
        count_16x16: Number of 16×16 tiles
        count_8x8: Number of 8×8 tiles
        scale_8x8: If True, scale 8×8 tiles to 16×16 for consistent grid (default: True)
        
    Returns:
        List of PIL Images (all 16×16 if scale_8x8=True)
    """
    from PIL import Image
    
    tiles = []
    total_tiles = count_16x16 + count_8x8
    
    for i in range(total_tiles):
        # Determine tile size from flags or position
        if flags_data and i < len(flags_data):
            is_8x8 = (flags_data[i] & 0x02) != 0
        else:
            is_8x8 = (i >= count_16x16)
        
        # Get palette for this tile
        pal_idx = palette_indices[i] if i < len(palette_indices) else 0
        palette = palettes[pal_idx] if pal_idx < len(palettes) else palettes[0] if palettes else [(i, i, i, 255) for i in range(256)]
        
        if is_8x8:
            # 8×8 tiles: 128 bytes each (8 rows × 16-byte stride)
            tile_offset = count_16x16 * 256 + (i - count_16x16) * 128
            tile_size = 8
            stride = 16
        else:
            # 16×16 tiles: 256 bytes each
            tile_offset = i * 256
            tile_size = 16
            stride = 16
        
        # Create tile image at native size
        native_img = Image.new('RGBA', (tile_size, tile_size), (0, 0, 0, 0))
        
        for y in range(tile_size):
            for x in range(tile_size):
                pix_off = tile_offset + y * stride + x
                if pix_off < len(pixel_data):
                    color_idx = pixel_data[pix_off]
                    if color_idx < len(palette):
                        native_img.putpixel((x, y), palette[color_idx])
        
        # Scale 8×8 tiles to 16×16 for consistent grid placement
        if scale_8x8 and tile_size == 8:
            tile_img = native_img.resize((16, 16), Image.NEAREST)
        else:
            tile_img = native_img
        
        tiles.append(tile_img)
    
    return tiles


if __name__ == "__main__":
    main()
