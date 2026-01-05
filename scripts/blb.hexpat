// GAME.BLB Pattern File for Skullmonkeys PSX
// Skullmonkeys (PAL: SLES-01090, NTSC: SLUS-00601)
//
// The BLB file is the main game archive containing all levels, assets,
// and resources. The first 0x1000 bytes (4096) contain the header.
//
// Reference: docs/blb-data-format.md, docs/blb-asset-handling.md

#pragma description Skullmonkeys GAME.BLB Archive
#pragma endian little

import std.mem;
import std.string;

// =============================================================================
// Constants
// =============================================================================

#define SECTOR_SIZE 2048
#define HEADER_SIZE 4096

// =============================================================================
// Formatting Functions
// =============================================================================

// Format RGB color as hex string for display
fn format_rgb(auto bg) {
    return std::format("#{:02X}{:02X}{:02X}", bg.bg_r, bg.bg_g, bg.bg_b);
};

// Format PSX 15-bit color as 24-bit RGB
fn format_psx_color(auto c) {
    u8 r8 = c.r << 3;
    u8 g8 = c.g << 3;
    u8 b8 = c.b << 3;
    return std::format("#{:02X}{:02X}{:02X} (STP:{})", r8, g8, b8, c.stp);
};

// Format level name with null trimming
fn format_level_name(auto entry) {
    return std::format("{} (Lv{})", entry.name, entry.level_index);
};

// Format movie entry info
fn format_movie(auto entry) {
    return std::format("{}: {} ({} sectors)", entry.movie_id, entry.filename, entry.sector_count);
};

// Format sector offset as file position
fn format_sector_offset(auto entry) {
    u32 file_offset = entry.sector_offset * SECTOR_SIZE;
    return std::format("Sector {} (0x{:06X})", entry.sector_offset, file_offset);
};

// =============================================================================
// PSX Color Format
// =============================================================================

// PSX 15-bit color (XBGR1555 format)
bitfield PSXColor {
    r : 5 [[comment("Red 0-31, multiply by 8 for 8-bit")]];
    g : 5 [[comment("Green 0-31")]];
    b : 5 [[comment("Blue 0-31")]];
    stp : 1 [[comment("Semi-transparency processing flag")]];
} [[format("format_psx_color"), color("8080FF")]];

// 256-color palette (CLUT) - 512 bytes
struct Palette256 {
    PSXColor colors[256];
} [[name("CLUT Palette"), color("FF80FF"), comment("256-entry color lookup table (512 bytes)")]];

// =============================================================================
// Sub-TOC Entry (used by Container Assets)
// =============================================================================

// Generic sub-TOC entry (12 bytes)
// Used in Asset 400 (palettes), Asset 600 (sprites), etc.
struct SubTOCEntry {
    u32 id [[name("ID/Flags"), comment("Entry ID or flags")]];
    u32 size [[name("Size"), comment("Data size in bytes")]];
    u32 offset [[name("Offset"), comment("Offset from container start")]];
} [[static, color("80FFFF")]];

// =============================================================================
// Asset TOC Entry (used by Data Segments)
// =============================================================================

// Each data segment (Primary, Secondary, Tertiary) has a TOC
struct AssetTOCEntry {
    u32 asset_id [[name("Asset ID"), comment("Asset type ID (e.g., 0x258=600, 0x259=601)")]];
    u32 size [[name("Size"), comment("Asset size in bytes")]];
    u32 offset [[name("Offset"), comment("Offset from segment start")]];
} [[static, color("FFFF80")]];

// =============================================================================
// Tile Header - Asset 100 (0x064) - 36 bytes
// =============================================================================

struct TileHeader {
    u8 bg_r [[name("BG Red"), color("FF0000")]];
    u8 bg_g [[name("BG Green"), color("00FF00")]];
    u8 bg_b [[name("BG Blue"), color("0000FF")]];
    u8 pad_03 [[hidden]];
    u8 secondary_r [[name("Secondary R"), color("FF4040")]];
    u8 secondary_g [[name("Secondary G"), color("40FF40")]];
    u8 secondary_b [[name("Secondary B"), color("4040FF")]];
    u8 pad_07 [[hidden]];
    u16 level_width [[name("Level Width"), comment("Level width in tiles")]];
    u16 level_height [[name("Level Height"), comment("Level height in tiles")]];
    u16 unknown_0c [[hidden]];
    u16 unknown_0e [[hidden]];
    u16 tile_count_16x16 [[name("16x16 Tiles"), comment("Number of 16x16 pixel tiles")]];
    u16 tile_count_8x8 [[name("8x8 Tiles"), comment("Number of 8x8 pixel tiles")]];
    u16 tile_count_extra [[name("Extra Tiles")]];
    u8 unknown_16[6] [[hidden]];
    u16 field_1c [[hidden]];
    u8 remaining[6] [[hidden]];
} [[name("Tile Header"), color("80FF80"), static]];

// =============================================================================
// Layer Entry - Asset 201 (0x0C9) - 92 bytes
// =============================================================================

struct LayerEntry {
    u32 position_x [[name("Position X")]];
    u32 position_y [[name("Position Y")]];
    u16 width [[name("Width"), comment("Width in tiles")]];
    u16 height [[name("Height"), comment("Height in tiles")]];
    u16 unknown_0c [[hidden]];
    u16 priority [[name("Priority"), comment("Depth value for layer ordering")]];
    u32 velocity_x [[name("Velocity X"), comment("Parallax scroll factor (0x8000 = 1.0)")]];
    u32 velocity_y [[name("Velocity Y"), comment("Parallax scroll factor")]];
    u32 unknown_18 [[hidden]];
    u16 unknown_1c [[hidden]];
    u8 flag_1e [[name("Flag 0x1E")]];
    u8 flag_1f [[name("Flag 0x1F")]];
    u8 flag_20 [[name("Flag 0x20")]];
    u8 flag_21 [[name("Flag 0x21")]];
    u8 unknown_22[4] [[hidden]];
    u8 layer_type [[name("Layer Type"), comment("Process if != 3")]];
    u8 unknown_27 [[hidden]];
    u8 unknown_28[4] [[hidden]];
    u32 anim_data_offset [[name("Animation Ptr"), comment("Animation data pointer offset")]];
    u8 additional[48] [[hidden]];
} [[name("Layer"), color("FFCC80"), static]];

// =============================================================================
// Sprite Structures - Asset 600 (0x258)
// =============================================================================

// RLE command for sprite decompression
bitfield RLECommand {
    copy_count : 8 [[name("Copy"), comment("Literal pixels to copy")]];
    skip_count : 7 [[name("Skip"), comment("Transparent pixels")]];
    new_line : 1 [[name("NewLine"), comment("Start new row")]];
} [[color("FF8080")]];

// Animation entry (12 bytes)
struct AnimationEntry {
    u32 animation_id [[name("Animation ID")]];
    u16 frame_count [[name("Frame Count")]];
    u16 frame_offset [[name("Frame Index"), comment("Index into frame metadata array")]];
    u16 flags [[name("Flags")]];
    u16 extra [[hidden]];
} [[name("Animation"), color("CC80FF"), static]];

// Frame metadata (36 bytes = 0x24)
struct SpriteFrameMetadata {
    u16 reserved_00 [[hidden]];
    u16 reserved_02 [[hidden]];
    u16 flags [[name("Flags"), comment("1 or 2")]];
    s16 render_x [[name("Render X"), comment("Render X offset (signed)")]];
    s16 render_y [[name("Render Y"), comment("Render Y offset (signed)")]];
    u16 render_width [[name("Render W")]];
    u16 render_height [[name("Render H")]];
    u16 unknown_0e [[hidden]];
    u16 reserved_10 [[hidden]];
    s16 anchor_x [[name("Anchor X"), comment("Signed offset")]];
    s16 anchor_y [[name("Anchor Y"), comment("Signed offset")]];
    u16 clip_width [[name("Clip W")]];
    u16 clip_height [[name("Clip H")]];
    u8 pad_1a[6] [[hidden]];
    u32 rle_offset [[name("RLE Offset"), comment("Offset to RLE compressed data")]];
} [[name("Frame"), color("FF80CC"), static]];

// Sprite header (12 bytes)
struct SpriteHeader {
    u16 animation_count [[name("Anims"), comment("Number of animation groups")]];
    u16 frame_meta_offset [[name("Frame Meta"), comment("Offset to frame metadata array")]];
    u32 rle_offset [[name("RLE Data"), comment("Offset to RLE compressed pixels")]];
    u32 palette_offset [[name("Palette"), comment("Offset to embedded CLUT")]];
} [[name("Sprite Header"), color("FF8080"), static]];

// =============================================================================
// Tilemap Entry - Asset 200 (0x0C8)
// =============================================================================

// Single tilemap entry (u16)
// Bits 0-10: Tile index (1-based, 0 = transparent)
// Bits 11-15: Unused
bitfield TilemapEntry {
    tile_index : 11 [[name("Tile"), comment("Tile index (1-based, 0=transparent)")]];
    unused : 5 [[hidden]];
} [[color("80CCFF")]];

// =============================================================================
// Tile Flags - Asset 302 (0x12E)
// =============================================================================

// Per-tile rendering flags
bitfield TileFlags {
    semi_transparent : 1 [[name("Alpha"), comment("Enable alpha blending")]];
    size_8x8 : 1 [[name("8x8"), comment("0=16x16, 1=8x8")]];
    skip : 1 [[name("Skip"), comment("Don't upload to GPU")]];
    reserved : 5 [[hidden]];
} [[color("CCFF80")]];

// =============================================================================
// Movie Entry (0x1C = 28 bytes)
// =============================================================================

struct MovieEntry {
    u16 reserved [[hidden]];
    u16 sector_count [[name("Sectors"), comment("Movie size in sectors")]];
    char movie_id[5] [[name("ID"), comment("4-char null-terminated ID")]];
    char short_name[3] [[name("Short"), comment("2-char abbreviation")]];
    char filename[16] [[name("Filename"), comment("CD path")]];
} [[name("Movie"), format("format_movie"), color("FFFF40"), static]];

// =============================================================================
// Sector Table Entry (0x10 = 16 bytes)
// =============================================================================

// Entry type flags
enum SectorEntryFlags : u8 {
    Level = 0x00,           // Level loading screen
    GameOver = 0x03,        // Game over screen
    Special = 0x05          // Special loading screen
};

struct SectorEntry {
    u8 level_index [[name("Level"), comment("Level index (0-25)")]];
    SectorEntryFlags entry_flags [[name("Type"), color("FF8040")]];
    u8 unknown_byte [[hidden]];
    char code[5] [[name("Code"), comment("4-char null-terminated ID")]];
    char short_name[4] [[name("Short"), comment("Truncated description")]];
    u16 sector_offset [[name("Sector"), comment("Sector offset in BLB")]];
    u16 sector_count [[name("Count"), comment("Number of sectors")]];
} [[name("Sector Entry"), format("format_sector_offset"), color("80CCCC"), static]];

// =============================================================================
// Level Metadata Entry (0x70 = 112 bytes)
// =============================================================================

struct LevelEntry {
    // Primary data pointers (0x00-0x0B)
    u16 sector_offset [[name("Sector"), comment("Primary data sector offset")]];
    u16 sector_count [[name("Count"), comment("Primary data sector count")]];
    u16 unknown_04 [[hidden]];
    u16 unknown_06 [[name("Unk06"), comment("Unknown count (7-20)")]];
    u16 entry1_offset_lo [[name("Entry1 Lo"), comment("Entry[1].offset low 16 bits")]];
    u16 unknown_0a [[name("Unk0a"), comment("Unknown count (0-16)")]];
    
    // Level identification (0x0C-0x0D)
    u8 level_index [[name("Index"), comment("Level asset index"), color("40FF40")]];
    u8 level_flag [[name("Flag"), comment("Level flag (0 or 1)")]];
    
    // Tertiary block configuration (0x0E-0x1B)
    u16 tert_block_count [[name("Tert Blocks"), comment("Active tertiary sub-blocks (1-6)")]];
    u16 tert_data_off[6] [[name("Tert Offsets"), comment("Offsets within tertiary blocks")]];
    u16 pad_1c [[hidden]];
    
    // Secondary data structure (0x1E-0x39)
    u16 secondary_offset [[name("Sec Offset"), comment("Secondary sector offset")]];
    u16 sec_sub_off[5] [[name("Sec Sub Offs")]];
    u16 pad_2a [[hidden]];
    u16 secondary_count [[name("Sec Count"), comment("Secondary sector count")]];
    u16 sec_sub_cnt[5] [[name("Sec Sub Cnts")]];
    u16 pad_38 [[hidden]];
    
    // Tertiary data structure (0x3A-0x55)
    u16 tert_sub_off[6] [[name("Tert Sub Offs")]];
    u16 pad_46 [[hidden]];
    u16 tert_sub_cnt[6] [[name("Tert Sub Cnts")]];
    u16 pad_54 [[hidden]];
    
    // Level strings (0x56-0x6F)
    char level_id[5] [[name("ID"), comment("4-char null-terminated ID"), color("FFFF80")]];
    char name[21] [[name("Name"), comment("Level display name"), color("80FF80")]];
} [[name("Level"), format("format_level_name"), color("40CCFF"), static]];

// =============================================================================
// Credits Sequence Entry (0x0C = 12 bytes)
// =============================================================================

// Credits table at 0xF10, accessed when playback mode == 2
// Only 2 complete entries fit before count fields at 0xF31
// Entry 2 (CRD2) at 0xF28 overlaps with counts - its params are garbage
//
// Sector calculation: CRED.sector_offset = param_b + level_count
// Example: 171 + 26 = 197 = CRED sector entry

struct CreditsEntry {
    char code[4] [[name("Code"), comment("4-char ID: '', 'CRD1', 'CRD2'"), color("FFCC40")]];
    u8 reserved[4] [[hidden]];
    u16 param_a [[name("Param A"), comment("Entry 0: level_count; CRD1/2: counter")]];
    u16 param_b [[name("Param B"), comment("Base sector offset (e.g., 171)")]];
} [[name("Credits"), color("FFCC80"), static]];

// =============================================================================
// Playback Sequence Mode Values
// =============================================================================

// Mode values control what type of content is loaded/played
// Accessed via: mode = header[0xF36 + stateOffset]
//
// Values:
//   0 = Invalid/Reset - AdvancePlaybackSequence calls SeekToLevelInSequence
//   1 = Movie - Play movie from movie table at 0xB60
//   2 = Credits - Display credits from credits table at 0xF10
//   3 = Level - Load and play level from level table at 0x000
//   4 = SpecialSector4 - Special sector loading (uses sector order at 0xCD0)
//   5 = SpecialSector5 - Initial sector loading (similar to 4)
//   6 = EndSpecial - Sequence terminator, exits playback loop
enum PlaybackMode : u8 {
    Invalid = 0,
    Movie = 1,
    Credits = 2,
    Level = 3,
    SpecialSector4 = 4,
    SpecialSector5 = 5,
    EndSpecial = 6
};

// =============================================================================
// Playback Sequence Entry
// =============================================================================

// The playback sequence is a state machine that controls game flow.
// Each "step" has a paired mode byte and index byte at different offsets:
//   mode  = header[0xF36 + stateOffset]
//   index = header[0xF92 + stateOffset]
//
// The stateOffset starts at 0 and increments as the game progresses.
// Example sequence (from game start):
//   Step 0: mode=5 (SpecialSector5), index=0 -> Load initial sector
//   Step 1: mode=1 (Movie), index=0 -> Play DREA movie
//   Step 2: mode=1 (Movie), index=1 -> Play LOGO movie
//   ...
//   Step N: mode=3 (Level), index=0 -> Load MENU level

struct PlaybackSequenceEntry {
    PlaybackMode mode [[name("Mode"), comment("Content type to load")]];
    u8 table_index [[name("Index"), comment("Index into corresponding table")]];
} [[static]];

// =============================================================================
// Playback Sequence Data (0xF34-0xFFF, 204 bytes)
// =============================================================================

// The playback sequence controls game flow: intro movies, credits, level loading.
// Two parallel arrays store mode and index values, accessed via stateOffset.
//
// Flow control markers (4-char codes in movie/credits tables):
//   "INT1" - After intro 1, skip next two items
//   "CRD1" - After credits 1, skip to level
//   "CRD2" - Credits 2 terminator
//   "END2" - Ending 2, conditionally skipped

struct PlaybackSequenceData {
    u8 unknown_00[2] [[hidden]];
    
    // Mode array: 92 entries starting at 0xF36
    // Accessed as: header[0xF36 + stateOffset]
    PlaybackMode modes[92] [[name("Modes"), comment("Playback mode for each step"), color("FF8080")]];
    
    // Index array: 110 entries starting at 0xF92
    // Accessed as: header[0xF92 + stateOffset]
    // Value is index into corresponding table based on mode:
    //   mode=1: index into movies[13] at 0xB60
    //   mode=2: index into credits[2] at 0xF10
    //   mode=3: index into levels[26] at 0x000
    //   mode=4/5: index into sectors[32] at 0xCD0
    u8 indices[110] [[name("Indices"), comment("Table index for each step"), color("80FF80")]];
} [[name("Playback Sequence"), color("CC80CC")]];

// =============================================================================
// BLB Header (0x1000 = 4096 bytes)
// =============================================================================

struct BLBHeader {
    LevelEntry levels[26] [[name("Levels"), comment("Level metadata (26 entries)"), color("40CCFF")]];
    MovieEntry movies[13] [[name("Movies"), comment("Movie table (13 entries)"), color("FFFF40")]];
    u8 movie_sector_pad[8] [[hidden]];
    SectorEntry sectors[32] [[name("Sectors"), comment("Sector table (32 entries)"), color("80CCCC")]];
    u32 unknown_array[16] [[name("Unknown"), comment("Unknown u32 array")]];
    CreditsEntry credits[2] [[name("Credits"), comment("Credits entries (only 2 complete)"), color("FFCC80")]];
    u8 gap_to_counts[9] [[hidden]];
    
    u8 level_count [[name("Level Count"), comment("Number of levels (26)"), color("40FF40")]];
    u8 movie_count [[name("Movie Count"), comment("Number of movies (13)"), color("FFFF40")]];
    u8 sector_entry_count [[name("Sector Count"), comment("Sector table entries"), color("80CCCC")]];
    
    // Playback sequence state machine (0xF34-0xFFF)
    PlaybackSequenceData playback [[name("Playback"), comment("Sequence mode/index arrays"), color("CC80CC")]];
} [[name("BLB Header"), comment("Skullmonkeys game archive header (4096 bytes)"), single_color]];

// =============================================================================
// Apply BLB Header at file start
// =============================================================================

BLBHeader header @ 0x00 [[name("GAME.BLB Header"), comment("Main archive header")]];
