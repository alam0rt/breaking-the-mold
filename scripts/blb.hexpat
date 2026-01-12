// GAME.BLB Pattern File for Skullmonkeys PSX
// Skullmonkeys (PAL: SLES-01090/91/92, NTSC-US: SLUS-00601, JP: SLPS-01501)
//
// The BLB file is the main game archive containing all levels, assets,
// and resources. The first 0x1000 bytes (4096) contain the header.
//
// This template auto-detects PAL/NTSC vs JP layout variants (see 0xCD0 detection).
//
// =============================================================================
// FILE STRUCTURE OVERVIEW
// =============================================================================
// 
// FILE COVERAGE (SLES-01090 / 76,021,760 bytes / 72.50 MB):
//   - Header:             0x0000 - 0x0FFF  (4 KB)
//   - Hidden Legal:       0x1000 - 0x5FFF  (sectors 2-11, unreferenced BS frame)
//   - Title/Legal:        0x6000 - 0xCFFF  (sectors 12-25)
//   - Credits Container:  0xD000 - 0x627FF (sectors 26-196, 20 interlaced BS frames)
//   - Level Data:         26 levels with primary/secondary/tertiary segments
//   - Sector Table:       32 entries at 0xCD0 (PAL) / 0xCB0 (JP)
//   - Password Screens:   17 containers (referenced via table at 0xECC)
//   - Duplicate Credits:  0x4820800 - 0x4875FFF (sectors 36929-37099, exact copy)
//   - Game Over/End:      0x4876000 - 0x487FFFF (sectors 37100-37119)
//   - Coverage:           100% (all sectors accounted for)
//
// SECTOR COVERAGE SUMMARY:
//   Sectors 0-1:      Header (padded to 2 sectors)
//   Sectors 2-11:     Hidden legal BS frame (unreferenced, likely replaced content)
//   Sectors 12-25:    Boot sequence (title, legal screens)
//   Sectors 26-196:   Credits container #1 (20 interlaced BS frames for CRT display)
//   Sectors 197-36928: Level data, loading screens, password screens
//   Sectors 36929-37099: Credits container #2 (duplicate for CD seek optimization)
//   Sectors 37100-37119: Game over screens and final padding
//
// =============================================================================
// SECTOR TABLE (32 ENTRIES - PAL: 0xCD0, JP: 0xCB0)
// =============================================================================
// The sector table maps to BS (bitstream/MDEC) images for loading screens.
// Each entry is 16 bytes with different field order for PAL vs JP.
//
// PAL/NTSC-US (at 0xCD0):
//   [0]  PIRA Title       [1]  LEGL Legal       [2]  CRED Credits <-- BUG TRIGGER
//   [3]  MENU             [4-28] Level loading screens
//   [29] Game Over        [30] Game Over Alt   [31] Password Screen
//
// JP (at 0xCB0) - Credits moved to fix intro bug (see TCRF):
//   [0]  PIRA Title       [1]  LEGL Legal       [2]  MENU (CRED removed)
//   [3-27] Level loading screens
//   [28] CRED Credits     [29] OVER Game Over  [30] OVER Alt
//
// The CRED entry at index 2 in PAL was triggering a bug where credits would
// play between intro movies. JP fixed this by splicing the two intro movies
// into one and moving CRED to index 28. (Source: TCRF regional differences)
//
// =============================================================================
// PASSWORD SCREEN TABLE (17 ENTRIES AT OFFSET 0xECC)
// =============================================================================
// Referenced by playback mode 6. Game code accesses: header + (index * 4) + 0xECC
// 
// IMPORTANT: Entry[0] overlaps with sectors[31].sector_offset/sector_count!
// This is why sectors[31] points to password screen #0 at sector 4895.
//
// Each password screen is a TOC container with 11 assets (except #16 with 12):
//   Asset 100: Tile Header (30×16 dimensions)
//   Asset 200: Tilemap Container (5 tilemaps)
//   Asset 201: Layer Entries (5 layers)
//   Asset 300: Tile Pixels (8bpp indexed)
//   Asset 301: Palette Indices
//   Asset 302: Tile Size Flags
//   Asset 400: Palette Container (4 × 256-color)
//   Asset 401: Animation Config
//   Asset 600: Tile Graphics (compressed)
//   Asset 601: Audio Sample Bank (SPU ADPCM)
//   Asset 602: Raw Palette (16-color CLUT)
//
// Password screens appear after their corresponding level/world:
//   Entry[0]:  0x0098F800 (Sector 4895) - Referenced via sectors[31] overlap
//   Entry[1]:  0x00EB7000 (World 1) ... Entry[15]: 0x044AA800 (World 15)
//   Entry[16]: 0x047DC800 (Victory "YOU WIN" - 12 assets)
//
// =============================================================================
// RENDERING PIPELINE (from render_level.py)
// =============================================================================
// 1. Each level has PRIMARY, SECONDARY, and TERTIARY segments
// 2. SECONDARY contains: Tile Header (100), Tile Pixels (300), Palette Indices (301),
//    Tile Flags (302), Palette Container (400)
// 3. TERTIARY contains: Stage Header (100), Tilemap Container (200), Layer Entries (201),
//    Sprite Container (600), Audio (500-503)
// 
// Stage/Secondary Pairing:
//   - Stage 0 uses base secondary tiles
//   - Stage N uses stage (N-1)'s secondary sub-block for tiles
//
// Layer Rendering:
//   - Each layer has dimensions (width, height) and position (x_offset, y_offset)
//   - Tilemap is width*height u16 values, bits 0-10 = tile index (1-based, 0=transparent)
//   - Parallax: scroll_x/scroll_y control scrolling (0x10000=1.0, 0x8000=0.5)
//   - Layers rendered back-to-front
//
// Reference: docs/blb-data-format.md, docs/blb-asset-handling.md

#pragma description Skullmonkeys GAME.BLB Archive
#pragma endian little

// Standard library imports
import std.mem;
import std.string;
import std.core;
import std.sys;

// Type library imports - better formatting and color display
import type.base;   // type::Hex<T> for hex-formatted integers
import type.color;  // type::RGBA5551 for PSX 15-bit colors

// =============================================================================
// Constants
// =============================================================================

#define SECTOR_SIZE 2048
#define HEADER_SIZE 4096

// =============================================================================
// Region/Layout Detection
// =============================================================================
// Two layout variants exist for sector table entries:
//   PAL/NTSC-US: code at byte[3], sector offset/count at bytes[12-15]
//   JP (SLPS):   sector offset/count at bytes[0-3], code at byte[7]
//
// CRITICAL: Sector table starts at DIFFERENT offsets!
//   PAL/NTSC: Sector table at 0xCD0 (code access via base 0xCCC)
//   JP:       Sector table at 0xCB0 (code access via base 0xCB0)
//
// The 32-byte (2 entry) shift is due to:
//   - JP removed CRED from index 2 (credits bug fix)
//   - JP has 12 movies vs PAL's 13 (intro spliced into 1 movie)
//
// Detection: At PAL offset 0xCD0, check if byte[3] is A-Z (code field).
//            JP will have a different pattern at 0xCD0 (it's entry[2], not [0]).

fn is_jp_layout() {
    // Read byte[3] of PAL's first sector entry location (0xCD0)
    // PAL: byte[3] is A-Z (65-90) - start of code field
    // JP: 0xCD0 is entry[2], so byte[3] would be 0x00 or different
    u8 byte3 = std::mem::read_unsigned(0xCD3, 1);
    return !(byte3 >= 65 && byte3 <= 90);
};

bool g_is_jp = is_jp_layout();

// Sector table base address (differs by region)
u32 SECTOR_TABLE_OFFSET = g_is_jp ? 0xCB0 : 0xCD0;

// Count offsets differ by region (JP is -0x1C from PAL)
// PAL: 0xF31/F32/F33, JP: 0xF15/F16/F17
u8 g_level_count = std::mem::read_unsigned(g_is_jp ? 0xF15 : 0xF31, 1);
u8 g_movie_count = std::mem::read_unsigned(g_is_jp ? 0xF16 : 0xF32, 1);
u8 g_sector_count = std::mem::read_unsigned(g_is_jp ? 0xF17 : 0xF33, 1);

// =============================================================================
// Level Entry Access Functions
// =============================================================================
// LevelEntry is 0x70 (112) bytes at offset 0x00 in header
// Layout is identical for PAL and JP, so we read directly from memory

fn get_level_sector_offset(u8 idx) {
    return std::mem::read_unsigned(idx * 0x70 + 0x00, 2);
};

fn get_level_sector_count(u8 idx) {
    return std::mem::read_unsigned(idx * 0x70 + 0x02, 2);
};

fn get_level_stage_count(u8 idx) {
    return std::mem::read_unsigned(idx * 0x70 + 0x0E, 2);
};

fn get_level_sec_sector_off(u8 idx, u8 stage) {
    return std::mem::read_unsigned(idx * 0x70 + 0x1E + stage * 2, 2);
};

fn get_level_sec_sector_cnt(u8 idx, u8 stage) {
    return std::mem::read_unsigned(idx * 0x70 + 0x2C + stage * 2, 2);
};

fn get_level_tert_sector_off(u8 idx, u8 stage) {
    return std::mem::read_unsigned(idx * 0x70 + 0x3A + stage * 2, 2);
};

fn get_level_tert_sector_cnt(u8 idx, u8 stage) {
    return std::mem::read_unsigned(idx * 0x70 + 0x48 + stage * 2, 2);
};

fn get_level_name(u8 idx) {
    return std::mem::read_string(idx * 0x70 + 0x5B, 21);
};

// =============================================================================
// Formatting Functions
// =============================================================================

// Format RGB color as hex string for display
fn format_rgb(auto bg) {
    return std::format("#{:02X}{:02X}{:02X}", bg.bg_r, bg.bg_g, bg.bg_b);
};

// Convert PSX 15-bit color to RGBA32 for visualizer
fn psx_to_rgba32(u16 color16) {
    u8 r = ((color16 & 0x1F) << 3);
    u8 g = (((color16 >> 5) & 0x1F) << 3);
    u8 b = (((color16 >> 10) & 0x1F) << 3);
    u8 a = (color16 == 0) ? 0 : 255;  // Color 0 is transparent
    return r | (g << 8) | (b << 16) | (a << 24);
};

// Format PSX 15-bit color as 24-bit RGB
fn format_psx_color(auto c) {
    u8 r8 = c.r << 3;
    u8 g8 = c.g << 3;
    u8 b8 = c.b << 3;
    return std::format("#{:02X}{:02X}{:02X} (STP:{})", r8, g8, b8, c.stp);
};

// PSX Color type alias - uses type::RGBA for proper color visualization in ImHex
// PSX uses 15-bit color: 5 bits R, 5 bits G, 5 bits B, 1 bit STP (semi-transparency)
using PSXColor = type::RGBA5551;

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

// Format tile counts summary
fn format_tile_header(auto h) {
    u16 total = h.tile_count_16x16 + h.tile_count_8x8 + h.tile_count_extra;
    return std::format("{}x{} tiles, {} total", h.level_width, h.level_height, total);
};

// Format layer summary
fn format_layer(auto l) {
    return std::format("{}x{} @ ({},{}) pri={}", l.width, l.height, l.position_x, l.position_y, l.priority);
};

// Format sprite summary
fn format_sprite(auto s) {
    return std::format("{} anims, {} frames", s.animation_count, s.frame_count);
};

// Format tilemap summary
fn format_tilemap_entry(auto e) {
    return std::format("Layer {}: {} tiles @ 0x{:X}", e.layer_index, e.tile_count, e.data_offset);
};

// =============================================================================
// PSX Color Format with Visualizer Support
// =============================================================================

// Transform PSX BGR5551 to RGBA32 for ImHex bitmap visualizer
// PSX format: bit 15 = STP, bits 14-10 = Blue, bits 9-5 = Green, bits 4-0 = Red
fn psx_color_to_rgba32(auto v) {
    u8 r8 = (v.r << 3) | (v.r >> 2);  // Expand 5-bit to 8-bit
    u8 g8 = (v.g << 3) | (v.g >> 2);
    u8 b8 = (v.b << 3) | (v.b >> 2);
    u8 a8 = 255;  // Fully opaque (color 0 transparency handled separately)
    return r8 | (g8 << 8) | (b8 << 16) | (a8 << 24);
};

// Transform raw u16 PSX color to RGBA32
fn psx_u16_to_rgba32(u16 color16) {
    u8 r = ((color16 & 0x1F) << 3);
    u8 g = (((color16 >> 5) & 0x1F) << 3);
    u8 b = (((color16 >> 10) & 0x1F) << 3);
    u8 a = (color16 == 0) ? 0 : 255;  // Color 0 is transparent
    return r | (g << 8) | (b << 16) | (a << 24);
};

// PSX 15-bit color format: 5-bit R, 5-bit G, 5-bit B, 1-bit STP (semi-transparency)
// Uses type::RGBA5551 which provides automatic color visualization in ImHex
// Note: PSXColor alias is defined in the imports section above

// 256-color palette (CLUT) - 512 bytes, using standard RGBA5551 type
struct Palette256 {
    PSXColor colors[256];
} [[name("CLUT Palette"), color("FF80FF"), comment("256-entry color lookup table (512 bytes)")]];

// 256-color palette as raw u16 array for easy conversion
struct Palette256Raw {
    u16 colors[256];
} [[name("CLUT Raw"), color("FF80FF")]];

// =============================================================================
// Asset ID Reference
// =============================================================================
//
// SECONDARY SEGMENT - Tile Data:
//   100 (0x064) TILE_HEADER      - Tile counts and level dimensions (36 bytes)
//   101 (0x065) VRAM_SLOT_CONFIG - VRAM texture bank counts (12 bytes, sparse)
//                                  Format: u32 bank_a_count, u32 bank_b_count, u32 pad
//                                  Used by FUN_80013b1c to allocate texture page slots
//   200 (0x0C8) TILEMAP_CONTAINER - Tilemap sub-TOC with layer data
//   201 (0x0C9) LAYER_ENTRIES    - Layer definitions (92 bytes each)
//   300 (0x12C) TILE_PIXELS      - 8bpp indexed pixel data
//   301 (0x12D) PALETTE_INDICES  - Palette index per tile (1 byte each)
//   302 (0x12E) TILE_FLAGS       - Tile size/rendering flags (1 byte each)
//   303 (0x12F) ANIMATED_TILES   - Animated tile lookup table
//   400 (0x190) PALETTE_CONTAINER - Palette sub-TOC with 256-color CLUTs
//   401 (0x191) PALETTE_ANIM     - Palette animation data
//
// SECONDARY SEGMENT - Audio & Tiles:
//   601 (0x259) AUDIO_SAMPLES    - SPU audio sample bank (ADPCM + TOC)
//   602 (0x25A) AUDIO_SETTINGS   - Per-sample volume/pan table
//
// PRIMARY SEGMENT - Level Geometry:
//   600 (0x258) GEOMETRY         - Level geometry/RLE sprites container
//   601 (0x259) AUDIO_SAMPLES    - SPU audio sample bank (same as secondary)
//   602 (0x25A) LEVEL_PALETTE    - Level palette (15-bit PSX colors)
//
// TERTIARY SEGMENT - Sprites & Stage Data:
//   500 (0x1F4) TILE_ATTRS       - Tile attribute map (collision/triggers, 1 byte/tile)
//   501 (0x1F5) ENTITIES         - Entity placement data (24-byte structures)
//   502 (0x1F6) VRAM_RECTS       - VRAM rectangles / texture page definitions
//   503 (0x1F7) ANIM_OFFSETS     - Animation frame offsets (ToolX sequence data)
//   504 (0x1F8) VEHICLE_DATA     - Vehicle level waypoints (FINN/RUNN only)
//   600 (0x258) SPRITE_CONTAINER - RLE sprites with embedded palettes
//   700 (0x2BC) SPU_SAMPLES      - Additional SPU audio samples (ADPCM)
//
// Note: Assets 100, 200, 201, 302, 401 may also appear in tertiary segments

// =============================================================================
// Generic TOC Entry (12 bytes)
// =============================================================================
// All TOC/sub-TOC entries in BLB share the same format:
//   u32 id      - Asset type ID (segments) or entry index (containers)
//   u32 size    - Data size in bytes
//   u32 offset  - Offset from segment/container start
//
// Used by: Segment TOCs, Palette/Tilemap/Sprite containers, Collision, etc.

struct TOCEntry {
    u32 id [[name("ID"), comment("Asset type or entry index")]];
    u32 size [[name("Size"), comment("Data size in bytes")]];
    type::Hex<u32> offset [[name("Offset"), comment("Offset from parent start")]];
} [[static, color("80FFCC")]];

// =============================================================================
// Tile Header - Asset 100 (0x064) - 36 bytes
// =============================================================================
// Verified via Ghidra decompilation of CopyTilePixelData (0x8007b588)
// and GetTotalTileCount (0x8007b53c).
//
// Color fields verified via Ghidra (2026-01-10):
// - LoadBGColorFromTileHeader (0x80024678) reads bytes 0-2 → GameState+0x131/132/133
// - LoadSecondaryColorFromTileHeader (0x800246d0) reads bytes 4-6 → GameState+0x127/128/129
// - GetTileHeaderPtr (0x8007b4b8) returns ctx[1] (TileHeader pointer)
// - GetSecondaryColorPtr (0x8007b4c4) returns TileHeader + 4

struct TileHeader {
    // Background color (0x00-0x03) - VERIFIED via Ghidra
    // Read by LoadBGColorFromTileHeader, stored at GameState+0x131/132/133
    // Most levels have (0,0,0) black; some have actual colors for distance fog
    u8 bg_r [[name("BG R"), color("FF0000"), comment("Background red 0-255")]];
    u8 bg_g [[name("BG G"), color("00FF00"), comment("Background green")]];
    u8 bg_b [[name("BG B"), color("0000FF"), comment("Background blue")]];
    u8 pad_03 [[hidden]];
    
    // Secondary/fog color (0x04-0x07) - VERIFIED via Ghidra
    // Read by LoadSecondaryColorFromTileHeader, stored at GameState+0x127/128/129
    // Most levels use gray (64,64,64); BOIL/SCIE/CAVE/CLOU have colored fog
    u8 secondary_r [[name("Fog R"), color("FF4040"), comment("Fog/secondary red")]];
    u8 secondary_g [[name("Fog G"), color("40FF40"), comment("Fog/secondary green")]];
    u8 secondary_b [[name("Fog B"), color("4040FF"), comment("Fog/secondary blue")]];
    u8 pad_07 [[hidden]];
    
    // Level dimensions (0x08-0x0B)
    u16 level_width [[name("Level W"), comment("Level width in tiles")]];
    u16 level_height [[name("Level H"), comment("Level height in tiles")]];
    
    // Spawn position (0x0C-0x0F) - VERIFIED via Ghidra
    // Read by FUN_8007b458, converted to pixels and stored at GameState+0x116/0x118
    // spawn_x_pixels = spawn_x * 16 + 8, spawn_y_pixels = spawn_y * 16 + 15
    u16 spawn_x [[name("Spawn X"), comment("Player spawn X position in tiles"), color("FFCC00")]];
    u16 spawn_y [[name("Spawn Y"), comment("Player spawn Y position in tiles"), color("FFCC00")]];
    
    // Tile counts (0x10-0x15) - used by GetTotalTileCount
    // Total = count_16x16 + count_8x8_a + count_8x8_b
    u16 count_16x16 [[name("16x16 Tiles"), comment("Number of 16x16 pixel tiles"), color("80FF80")]];
    u16 count_8x8_a [[name("8x8 Tiles A"), comment("Primary 8x8 tile count"), color("80CCFF")]];
    u16 count_8x8_b [[name("8x8 Tiles B"), comment("Additional tile count (often 0)")]];
    
    // Field 0x16: Vehicle Waypoint Count (VERIFIED 2026-01-11)
    // - Matches Asset 504 entry count exactly: FINN=78, RUNN=1, all others=0
    // - Used by vehicle/flying gameplay modes (FINN=FlynnBoy, RUNN=Runner)
    u16 vehicle_waypoint_count [[name("Vehicle Waypoints"), comment("Asset 504 waypoint count (FINN/RUNN only)"), color("40FF40")]];
    
    // Field 0x18: Level Flags (TENTATIVE 2026-01-11)
    // Bitfield controlling level rendering/behavior. Analyzed per-level values:
    //   Bit 1 (0x0002): SEVN, FINN - special gameplay mode (correlates with special_level_id=99)
    //   Bit 2 (0x0004): FINN only - FlynnBoy-specific flag
    //   Bit 3 (0x0008): EGGS, GLID, WEED, HEAD, MEGA, TMPL, FOOD - possibly collision behavior
    //   Bit 4 (0x0010): RUNN only - runner vehicle mode
    //   Bit 6 (0x0040): WIZZ, HEAD, RUNN, GLEN, MEGA, TMPL, FOOD - boss/special rendering?
    //   Bit 7 (0x0080): EVIL only
    //   Bit 8 (0x0100): GLEN only  
    //   Bit 9 (0x0200): MENU only
    //   Bit 12 (0x1000): PHRO, MEGA - possibly world 1 / first encounter
    //   Bit 14 (0x4000): CLOU only
    //   Bit 15 (0x8000): FOOD only
    // Values: 0x0000 (9 levels), 0x0008 (3), 0x1048 (2), unique flags for others
    u16 level_flags [[name("Level Flags"), comment("Level behavior bitfield"), color("CCCCCC")]];
    
    // Field 0x1A: Special Level ID (VERIFIED 2026-01-11)
    // - Value 99 in FINN and SEVN (special gameplay mode identifier)
    // - Value 0 in all other levels (normal gameplay)
    // - Correlates with level_flags bit 1 (0x0002)
    u16 special_level_id [[name("Special Level ID"), comment("99 = special mode (FINN/SEVN)"), color("40FF40")]];
    
    // Field 0x1C: VRAM Rectangle Count (VERIFIED 2026-01-11)
    // - Read by GetAsset100Field1C (0x8007b7c8), stored at GameState+0x78
    // - Also stored at GameState+0x78 by InitLayersAndTileState (0x80024778)
    // - Matches Asset 502 (vram_rects) entry count exactly for all levels
    // - Boss levels=0 (no VRAM rects), scrolling levels have values 1-300+
    u16 vram_rect_count [[name("VRAM Rect Count"), comment("Number of VRAM rectangles (Asset 502 entries)"), color("FFAA00")]];
    
    // Field 0x1E: Entity Count (VERIFIED 2026-01-11)
    // - Matches Asset 501 (entities) byte size / 24 exactly for all levels
    // - Read by GetEntityCount (0x8007b7a8), used by LoadEntitiesFromAsset501 (0x80024dc4)
    u16 entity_count [[name("Entity Count"), comment("Number of entities in Asset 501"), color("FF8080")]];
    
    // Field 0x20: World/Area Index (VERIFIED 2026-01-13)
    // - Values 0-6 observed, accumulated across level transitions
    // - Read by GetTileHeaderField08 (MISNAMED, actually reads +0x20) @ 0x8007b490
    // - InitGameState stores initial value in g_pPlayerState[4]
    // - Level transition (FUN_8007d8a0) ADDS field_20 to g_pPlayerState[4]
    // - Likely selects audio theme or visual style per world
    u16 world_index [[name("World Index"), comment("World/area index (0-6), accumulated across levels"), color("80CCFF")]];
    
    // Remaining padding
    u8 remaining[2] [[hidden]];
} [[name("Tile Header"), color("80FF80"), static]];

// =============================================================================
// VRAM Slot Configuration - Asset 101 (0x65) - 12 bytes
// =============================================================================
// VERIFIED 2026-01-13 via Ghidra (GetAsset101Entry @ 0x8007b3fc, InitVRAMSlotTable @ 0x80013b1c)
//
// Controls texture page slot allocation for the level's sprites/tiles.
// Only present in ~14 levels that need custom VRAM layouts.
//
// GetAsset101Entry(ctx, index) returns field at index 0 or 1.
// InitVRAMSlotTable allocates (bank_a_count + bank_b_count) * 3 texture slots.
//
// Observations:
//   - bank_a_count: 1-4 (higher for complex levels like MEGA, EGGS)
//   - bank_b_count: 0-1 (only FOOD and HEAD have 1 - special rendering)
//   - reserved: always 0

struct VRAMSlotConfig {
    u32 bank_a_count [[name("Bank A Count"), comment("Texture slots in upper VRAM (y=0xF0), values 1-4"), color("80CCFF")]];
    u32 bank_b_count [[name("Bank B Count"), comment("Texture slots in lower VRAM (y=0x1F0), values 0-1"), color("80AAFF")]];
    u32 reserved [[name("Reserved"), comment("Always 0 - padding"), hidden]];
} [[name("VRAM Slot Config"), color("AA80FF"), static]];

// =============================================================================
// Entity Structure - Asset 501 (0x1F5) - 24 bytes each
// =============================================================================
// VERIFIED via Ghidra (FUN_80024dc4 entity loader, FUN_8007b7a8 entity count)
//
// Entities are placed objects in the level (collectibles, enemies, ammo, etc.)
// Entity data is stored as an array of 24-byte structures in Asset 501.
// Entity count is at TileHeader + 0x1E (entity_count field, verified 2026-01-10).
//
// IMPORTANT: Entity Type → Sprite ID mapping is HARDCODED IN GAME CODE, not in BLB data.
// See "Sprite ID Lookup Chain" section below for details.
//
// Known entity types (verified 2026-01-10):
//   Type 2  = Clayballs (coins/collectibles) - 5727 total
//   Type 3  = Ammo pickup (bullets for player weapon) - 308 total
//   Type 8  = Item pickup - 144 total
//   Type 24 = Special ammo pickup - 227 total
//   Type 25, 27 = Enemies
//   Type 28, 48 = Moving platforms / directional objects
//   Type 45 = Message box

struct Entity {
    // Bounding box in pixels (0x00-0x07)
    u16 x1 [[name("X1"), comment("Bounding box min X (pixels)"), color("FFCC80")]];
    u16 y1 [[name("Y1"), comment("Bounding box min Y (pixels)"), color("FFCC80")]];
    u16 x2 [[name("X2"), comment("Bounding box max X (pixels)"), color("FFCC80")]];
    u16 y2 [[name("Y2"), comment("Bounding box max Y (pixels)"), color("FFCC80")]];
    
    // Entity center position (0x08-0x0B)
    u16 x_center [[name("Center X"), comment("Entity center X (pixels)"), color("FF8080")]];
    u16 y_center [[name("Center Y"), comment("Entity center Y (pixels)"), color("FF8080")]];
    
    // Variant/subtype selector (0x0C-0x0D)
    // For Type 2 (clayballs): animation frame cycle (0,3,4,5,6,7,8)
    // For Type 45 (message): message ID (0,1,2,12,13,14)
    // For Type 28,48: direction/state (1,2)
    u16 variant [[name("Variant"), comment("Animation frame or subtype selector"), color("80CCFF")]];
    
    // Padding (0x0E-0x11) - always zero
    u16 padding1 [[hidden]];
    u16 padding2 [[hidden]];
    
    // Entity type ID (0x12-0x13)
    // This determines which sprite to use - but the sprite ID is looked up in code,
    // not stored in BLB data. See sprite lookup chain documentation.
    // Known types: 2=clayball, 3=ammo, 8=item, 24=special ammo, 25/27=enemy, 45=message
    u16 entity_type [[name("Type"), comment("Entity type ID (2=clayball, 3=ammo, 24=special ammo, 45=message, etc)"), color("FF80FF")]];
    
    // Render layer with flags (0x14-0x15)
    // Lower byte (bits 0-7): Actual layer index (1, 2, or 3)
    // Upper byte (bits 8-15): Render flags or z-order modifiers
    // Examples from CSTL: 0xF301 = layer 1 + 0xF3 flags, 0x2E02 = layer 2 + 0x2E flags
    // Most entities have simple values (1, 2, 3) but type 81 and type 9 use extended values
    u16 layer [[name("Layer"), comment("Lower byte=layer, upper byte=render flags"), color("80FF80")]];
    
    // Padding (0x16-0x17) - always zero
    u16 padding3 [[hidden]];
} [[static, color("FFCC80")]];

fn format_entity(auto e) {
    return std::format("Type {} @ ({},{}) variant={}", e.entity_type, e.x_center, e.y_center, e.variant);
};

// =============================================================================
// Sprite ID Lookup Chain - VERIFIED via Ghidra (2026-01-09)
// =============================================================================
// Entity type → sprite ID mapping is HARDCODED in game code, not in BLB.
//
// Lookup chain:
//   FUN_8001cdac(context, priority, sprite_id)
//     └─► InitSpriteContext(context+0x78, sprite_id) @ 0x8007bc3c
//           └─► LookupSpriteById(sprite_id) @ 0x8007bb10
//                 ├─► FindSpriteInTOC(DAT_800a6064, sprite_id) @ 0x8007b968
//                 │   (Searches tertiary container's sprite TOC at context+0x70)
//                 └─► Fallback: Search DAT_800a6060 (secondary sprite table)
//
// Sprite IDs are 32-bit hash-like values in Asset 600 TOC:
//   0x21842018 - Player sprite (hardcoded in FUN_8001fcf0)
//   0xb8700ca1 - Common UI element
//   0xe2f188   - Menu/UI (heavily reused across multiple sprite indices)
//   0xa9240484 - Used for indices 0x118, 0xe
//   0xe8628689 - Single use at index 0x98
//   0x88a28194 - Loop objects at index 0x60
//   0x80e85ea0 - Loop objects at index 0xd0
//   0x9158a0f6 - Player-related at index 0x18
//   0x902c0002 - Used at index 0x118
//
// To render entities in a BLB viewer, you must build a static lookup table
// by tracing each entity type's spawn dispatcher in the game code.

// =============================================================================
// Color Tint Entry - RGB triplet for layer color tinting
// =============================================================================
// Used in LayerEntry color_tints[16] array at offset 0x2C
// Tilemap entries use upper 4 bits (12-15) to select which color to use
struct ColorTintEntry {
    u8 r [[name("R"), color("FF4040")]];
    u8 g [[name("G"), color("40FF40")]];
    u8 b [[name("B"), color("4040FF")]];
} [[static]];

// =============================================================================
// Layer Entry - Asset 201 (0x0C9) - 92 bytes
// =============================================================================
// Verified via Ghidra analysis and extraction testing.
//
// Rendering: Each layer has its own tilemap from Asset 200. Layers are rendered
// back-to-front. The tilemap is width*height u16 values, placed at (x_offset, y_offset).
// Parallax: scroll_x/scroll_y control scrolling speed relative to camera.
//   0x10000 = 1.0 (scrolls with camera), 0x8000 = 0.5 (parallax background)

// Format parallax factor as float
fn format_parallax(u32 val) {
    float f = float(val) / 65536.0;
    return std::format("{:.3f}x", f);
};

struct LayerEntry {
    // Position and dimensions (0x00-0x0B)
    u16 x_offset [[name("X Offset"), comment("X position in tiles")]];
    u16 y_offset [[name("Y Offset"), comment("Y position in tiles")]];
    u16 width [[name("Width"), comment("Layer width in tiles")]];
    u16 height [[name("Height"), comment("Layer height in tiles")]];
    u16 level_width [[name("Level W"), comment("Level width in tiles (from Asset 100)")]];
    u16 level_height [[name("Level H"), comment("Level height in tiles")]];
    
    // Render parameter (0x0C-0x0F) - VERIFIED 2026-01-12 via Ghidra
    // Low 16 bits = priority (signed short, lower = further back)
    // Used for z-ordering in render lists. Typical values:
    //   150-800: Background/parallax layers
    //   900-1100: Main gameplay layers
    //   1200-1500: Foreground layers
    // Entity z_order values share this space (player=10000, particles=959)
    u32 render_param [[name("Render Param"), comment("Low 16 bits = priority for z-ordering")]];
    
    // Scroll factors (0x10-0x17) - 0x10000 = 1.0, 0x8000 = 0.5
    u32 scroll_x [[name("Scroll X"), format("format_parallax"), comment("X parallax (1.0 = camera, 0.5 = parallax)")]];
    u32 scroll_y [[name("Scroll Y"), format("format_parallax"), comment("Y parallax factor")]];
    
    // Stored at render context +0x30/+0x32 (0x18-0x1B)
    u16 render_field_30 [[name("Render 0x30"), comment("Stored at renderCtx+0x30")]];
    u16 render_field_32 [[name("Render 0x32"), comment("Stored at renderCtx+0x32")]];
    
    // Stored at render context +0x3A/+0x3B (0x1C-0x1D)
    u8 render_field_3a [[name("Render 0x3A"), comment("Stored at renderCtx+0x3A")]];
    u8 render_field_3b [[name("Render 0x3B"), comment("Stored at renderCtx+0x3B")]];
    
    // Scroll enable flags (0x1E-0x21) - VERIFIED via Ghidra
    // Set corresponding GameState flags when non-zero
    u8 scroll_left_enable [[name("Scroll L"), comment("If !=0, set GameState+0x59 (enable scroll left)")]];
    u8 scroll_right_enable [[name("Scroll R"), comment("If !=0, set GameState+0x5B (enable scroll right)")]];
    u8 scroll_up_enable [[name("Scroll U"), comment("If !=0, set GameState+0x58 (enable scroll up)")]];
    u8 scroll_down_enable [[name("Scroll D"), comment("If !=0, set GameState+0x5A (enable scroll down)")]];
    
    // Render mode selection (0x22-0x25) - VERIFIED via Ghidra
    // Used to select which render function is called
    u16 render_mode_h [[name("Render Mode H"), comment("If !=0, affects render path (uVar15)")]];
    u16 render_mode_v [[name("Render Mode V"), comment("If !=0, affects render path (uVar17)")]];
    
    // Layer type (0x26)
    u8 layer_type [[name("Type"), comment("Layer type (0=normal, 3=skip render)")]];
    
    // Padding (0x27)
    u8 pad_27 [[hidden]];
    
    // Render skip flag (0x28-0x29) - VERIFIED via Ghidra
    // If layer_type != 3 AND this == 0, layer is rendered
    u16 skip_render [[name("Skip Render"), comment("If !=0 (and type!=3), skip this layer")]];
    
    // Padding (0x2A-0x2B) - CONFIRMED 2026-01-13: All values are 0 across all levels
    u16 pad_2a [[hidden]];
    
    // ==========================================================================
    // Color Tint Table (0x2C-0x5B) - 16 RGB entries, 48 bytes
    // ==========================================================================
    // DISCOVERED 2026-01-06: Tilemap u16 entries use upper 4 bits (12-15) as color selector
    // The tile render function (FUN_80017540) reads: (tilemap >> 12) * 3 + color_table
    // This allows 16 different tint colors per tile, selected by tilemap bits 12-15
    //
    // Entry 0 at 0x2C is the "default" color (previously misidentified as bg_r/g/b)
    // Entry 1-15 provide additional tinting options for the layer
    //
    // Tilemap encoding (16 bits):
    //   Bits 0-11:  Tile index (0 = transparent, 1-4095 = tile index)
    //   Bits 12-15: Color tint selector (0-15, indexes into this table)
    
    ColorTintEntry color_tints[16] [[name("Color Tints"), comment("16 RGB tint colors, selected by tilemap bits 12-15")]];
    
} [[name("Layer"), color("FFCC80"), static]];

// =============================================================================
// Sprite Structures - Asset 600 (0x258)
// =============================================================================
// Sprites use RLE compression with embedded 256-color palettes.
// Each sprite has: header, animation entries, frame metadata, RLE data, palette.

// PSX 15-bit color with transform to RGBA32 for visualization
bitfield PSXColorEntry {
    r : 5;
    g : 5;
    b : 5;
    stp : 1;
} [[sealed, transform("psx_entry_to_rgba32"), format("format_psx_color_entry")]];

fn psx_entry_to_rgba32(auto c) {
    // Expand 5-bit to 8-bit with proper rounding
    u8 r8 = (c.r << 3) | (c.r >> 2);
    u8 g8 = (c.g << 3) | (c.g >> 2);
    u8 b8 = (c.b << 3) | (c.b >> 2);
    return r8 | (g8 << 8) | (b8 << 16) | (0xFF << 24);
};

fn format_psx_color_entry(auto c) {
    u8 r8 = c.r << 3;
    u8 g8 = c.g << 3;
    u8 b8 = c.b << 3;
    return std::format("#{:02X}{:02X}{:02X}", r8, g8, b8);
};

// RLE command for sprite decompression
// Bit 15: new_line (advance Y, reset X)
// Bits 14-8: skip_count (transparent pixels)
// Bits 7-0: copy_count (literal pixels to copy)
bitfield RLECommand {
    copy_count : 8 [[name("Copy"), comment("Literal pixels to copy")]];
    skip_count : 7 [[name("Skip"), comment("Transparent pixels")]];
    new_line : 1 [[name("NL"), comment("Start new row")]];
} [[format("format_rle_cmd"), color("FF8080")]];

fn format_rle_cmd(auto cmd) {
    if (cmd.new_line) {
        return std::format("NL skip={} copy={}", cmd.skip_count, cmd.copy_count);
    }
    return std::format("skip={} copy={}", cmd.skip_count, cmd.copy_count);
};

// Animation entry (12 bytes)
// VERIFIED via Ghidra FUN_8001d748: flags bit 0 triggers frame callback
struct AnimationEntry {
    u32 animation_id [[name("Anim ID")]];
    u16 frame_count [[name("Frames")]];
    u16 frame_offset [[name("Frame Idx"), comment("Index into frame metadata array")]];
    u16 flags [[name("Flags"), comment("Bit 0: has_frame_callback (triggers FUN_8001c4a4)")]];
    u16 extra [[hidden]];
} [[format("format_anim_entry"), color("CC80FF"), static]];

fn format_anim_entry(auto e) {
    return std::format("ID {} ({} frames)", e.animation_id, e.frame_count);
};

// Frame metadata (36 bytes = 0x24)
// VERIFIED via Ghidra: DecodeRLESprite (0x80010068), FUN_8001d748, GetFrameMetadata (0x8007bebc)
struct SpriteFrameMetadata {
    u16 callback_id [[name("Callback"), comment("0=none, triggers FUN_8001c4a4 for SFX/particles")]];
    u16 reserved_02 [[hidden]];
    u16 flip_flags [[name("Flip"), comment("0=normal, non-zero=horizontal mirror (verified in DecodeRLESprite)")]];
    s16 render_x [[name("X")]];
    s16 render_y [[name("Y")]];
    u16 render_width [[name("W")]];
    u16 render_height [[name("H")]];
    u16 frame_delay [[name("Delay"), comment("Per-frame timing (verified: copied to entity+0xE6)")]];
    u16 reserved_10 [[hidden]];
    s16 hitbox_x [[name("Hit X"), comment("Hitbox offset (verified in FUN_8001d748)")]];
    s16 hitbox_y [[name("Hit Y")]];
    u16 hitbox_width [[name("Hit W")]];
    u16 hitbox_height [[name("Hit H")]];
    u8 pad_1a[6] [[hidden]];
    type::Hex<u32> rle_offset [[name("RLE Off"), comment("Offset to RLE data")]];
} [[format("format_frame_meta"), color("FF80CC"), static]];

fn format_frame_meta(auto f) {
    return std::format("{}x{} @ ({},{}) RLE=0x{:X}", f.render_width, f.render_height, f.render_x, f.render_y, f.rle_offset);
};

// Sprite header (12 bytes)
struct SpriteHeader {
    u16 animation_count [[name("Anims")]];
    type::Hex<u16> frame_meta_offset [[name("Frame Off")]];
    type::Hex<u32> rle_offset [[name("RLE Off")]];
    type::Hex<u32> palette_offset [[name("Pal Off")]];
} [[format("format_sprite_hdr"), color("FF8080"), static]];

fn format_sprite_hdr(auto h) {
    return std::format("{} anims, frames@0x{:X}, RLE@0x{:X}, pal@0x{:X}", 
        h.animation_count, h.frame_meta_offset, h.rle_offset, h.palette_offset);
};

// PSX 15-bit color for palette visualization
// Uses inline color display so each entry shows its actual color
struct PaletteColor {
    u16 raw [[hidden]];
    
    // Extract components
    u8 _r = (raw & 0x1F) << 3;
    u8 _g = ((raw >> 5) & 0x1F) << 3;
    u8 _b = ((raw >> 10) & 0x1F) << 3;
} [[format("format_pal_color"), sealed, 
    hex::inline_visualize("color", _r, _g, _b, 255)]];

fn format_pal_color(auto c) {
    u8 r = (c.raw & 0x1F) << 3;
    u8 g = ((c.raw >> 5) & 0x1F) << 3;
    u8 b = ((c.raw >> 10) & 0x1F) << 3;
    return std::format("#{:02X}{:02X}{:02X}", r, g, b);
};

// Embedded sprite palette (256 colors = 512 bytes)
struct SpritePalette {
    PaletteColor colors[256] [[name("Colors")]];
} [[name("Palette"), color("FF80FF")]];

// RLE data block with command visualization
struct RLEDataBlock {
    u16 command_count [[name("Cmd Count")]];
    RLECommand commands[command_count] [[name("Commands")]];
    // Pixel data follows but size is variable (sum of all copy_counts)
} [[name("RLE Data"), color("FFCC80")]];

// Complete sprite with all components for visualization
struct SpriteWithPalette<auto sprite_base, auto sprite_size> {
    // Read header
    SpriteHeader header @ sprite_base [[name("Header")]];
    
    // Read animations (immediately after header)
    AnimationEntry animations[header.animation_count] @ (sprite_base + 12) [[name("Animations")]];
    
    // Calculate total frame count from animations
    u32 _total_frames = 0;
    // Note: Would need to sum animation frame counts - using approximation
    
    // Frame metadata at frame_meta_offset
    // For visualization we'll show frames if we can determine count
    
    // Show embedded palette
    SpritePalette palette @ (sprite_base + header.palette_offset) 
        [[name("Palette"), comment("256-color CLUT")]];
    
    // Show RLE data header
    RLEDataBlock rle_data @ (sprite_base + header.rle_offset)
        [[name("RLE Data"), hex::visualize("hex_viewer", rle_data)]];
};

// Sprite Sub-TOC entry with embedded sprite data
struct SpriteTOCEntry {
    u32 sprite_id [[name("ID"), comment("Sprite lookup ID")]];
    u32 size [[name("Size")]];
    type::Hex<u32> offset [[name("Offset")]];
} [[format("format_sprite_toc"), color("FF8080"), static]];

fn format_sprite_toc(auto e) {
    return std::format("Sprite {} ({} bytes @ 0x{:X})", e.sprite_id, e.size, e.offset);
};

// Individual sprite data block - shown when navigating to sprite offset
struct SpriteDataFull {
    u128 _sprite_base = $ [[hidden]];
    
    // Header (12 bytes)
    SpriteHeader header [[name("Header")]];
    
    // Animations (12 bytes each, immediately after header)
    AnimationEntry animations[header.animation_count] [[name("Animations")]];
    
    // Calculate approximate frame count (sum of animation frame counts)
    // We'll read up to 100 frames max for safety
    u16 _approx_frames = 0;
    
    // Frame metadata is at header.frame_meta_offset from sprite start
    // For now, show first few frames based on first animation
    if (header.animation_count > 0) {
        SpriteFrameMetadata frames[animations[0].frame_count] 
            @ (_sprite_base + header.frame_meta_offset) [[name("Frames (Anim 0)")]];
    }
    
    // Embedded palette at header.palette_offset
    SpritePalette palette @ (_sprite_base + header.palette_offset) 
        [[name("Palette"), comment("256-color CLUT embedded in sprite")]];
    
    // RLE data header at header.rle_offset
    RLEDataBlock rle @ (_sprite_base + header.rle_offset)
        [[name("RLE Commands"), hex::visualize("hex_viewer", rle)]];
} [[name("Sprite"), color("FF8080")]];

// Sprite container with full parsing of first few sprites
struct SpriteContainerFull {
    u128 _container_base = $ [[hidden]];
    u32 sprite_count [[name("Count")]];
    SpriteTOCEntry toc[sprite_count] [[name("Sprite TOC")]];
    
    // Parse first sprite if available
    if (sprite_count > 0) {
        SpriteDataFull sprite_0 @ (_container_base + toc[0].offset) 
            [[name("Sprite 0")]];
    }
    if (sprite_count > 1) {
        SpriteDataFull sprite_1 @ (_container_base + toc[1].offset) 
            [[name("Sprite 1")]];
    }
    if (sprite_count > 2) {
        SpriteDataFull sprite_2 @ (_container_base + toc[2].offset) 
            [[name("Sprite 2")]];
    }
    if (sprite_count > 3) {
        SpriteDataFull sprite_3 @ (_container_base + toc[3].offset) 
            [[name("Sprite 3")]];
    }
    if (sprite_count > 4) {
        SpriteDataFull sprite_4 @ (_container_base + toc[4].offset) 
            [[name("Sprite 4")]];
    }
    // Additional sprites can be accessed via TOC offsets
} [[name("Sprite Container"), color("FF8080")]];

// Simple sprite container (just TOC, no deep parsing)
struct SpriteContainer {
    u128 _base = $ [[hidden]];
    u32 sprite_count [[name("Count")]];
    SpriteTOCEntry sprites[sprite_count] [[name("Sprite TOC")]];
} [[name("Sprite Container"), color("FF8080")]];

// =============================================================================
// Tilemap Entry - Asset 200 (0x0C8)
// =============================================================================
// Each tilemap is an array of u16 values, one per tile position in the layer.
// The layer dimensions (width * height) determine how many entries.
// Rendering: for y in height, for x in width: tile = tilemap[y * width + x]

// Format tilemap entry to show actual tile index
fn format_tile_index(auto e) {
    u16 idx = e.tile_index;
    if (idx == 0) {
        return "transparent";
    }
    return std::format("Tile {}", idx - 1);  // Convert to 0-based
};

// Single tilemap entry (u16)
// Bits 0-10: Tile index (1-based, 0 = transparent)
// Bits 11-15: Unused (no flip flags in Skullmonkeys)
bitfield TilemapEntry {
    tile_index : 11 [[name("Tile"), comment("Tile index (1-based, 0=transparent)")]];
    unused : 5 [[hidden]];
} [[format("format_tile_index"), color("80CCFF")]];

// =============================================================================
// Tile Flags - Asset 302 (0x12E)
// =============================================================================

// Per-tile rendering flags
// Verified via Ghidra decompilation of LoadTileDataToVRAM (0x80025240)
bitfield TileFlags {
    semi_transparent : 1 [[name("Alpha"), comment("Bit 0: Enable GPU alpha blending")]];
    size_8x8 : 1 [[name("8x8"), comment("Bit 1: Tile is 8x8 (not 16x16)")]];
    skip : 1 [[name("Skip"), comment("Bit 2: Don't load/render this tile")]];
    reserved : 5 [[hidden]];
} [[color("CCFF80")]];

// =============================================================================
// Animated Tiles - Asset 303 (0x12F)
// =============================================================================

// Animated tile lookup table - maps animated tile indices to frame data
// Structure details TBD - appears to be variable length

// =============================================================================
// Palette Animation - Asset 401 (0x191)
// =============================================================================

// Palette animation data - controls color cycling effects
// Structure details TBD - appears in tertiary segments

// =============================================================================
// Tile Attribute Map - Asset 500 (0x1F4)
// =============================================================================
// Per-tile collision/attribute map. Each byte represents properties for one tile.
// Size = 8-byte header + (level_width × level_height) bytes
//
// VERIFIED via analysis (see docs/unconfirmed_findings.md):
// - Appears in 98 of ~104 stages
// - Size matches level dimensions (e.g., 535×43 = 23,005 tiles + 8 = 23,013 bytes)
//
// Known tile attribute values:
//   0x00 = Empty/passable
//   0x02 = Solid/collision
//   0x12 = Unknown trigger
//   0x53 = Checkpoint/save point?
//   0x65 = Entity spawn zone/trigger
//
// Header fields (offset +0x00 to +0x07):
//   The first 4 bytes contain TWO u16 values (not a single u32):
//   - unknown_lo (0x00): Values 0-21 observed, purpose unknown
//   - unknown_hi (0x02): Values 0-21 observed, purpose unknown
//   These are NOT padding - 69/98 stages have non-zero values.
//   No clear correlation with level dimensions, spawn position, or indices found.
//   MENU levels (level_00) lack Asset 500 entirely.

struct TileAttributeHeader {
    u16 unknown_lo [[name("Unknown Lo"), comment("Range 0-21, purpose unverified")]];
    u16 unknown_hi [[name("Unknown Hi"), comment("Range 0-21, purpose unverified")]];
    u16 level_width [[name("Level Width"), comment("Width in tiles")]];
    u16 level_height [[name("Level Height"), comment("Height in tiles")]];
} [[static, color("FF9966")]];

struct TileAttributeMap<auto asset_size> {
    TileAttributeHeader header;
    
    // Tile data: one byte per tile in row-major order
    if (asset_size > 8) {
        u8 tile_data[asset_size - 8] [[name("Tile Attributes"), comment("Per-tile collision/trigger values")]];
    }
} [[color("FF8844")]];

// =============================================================================
// Audio System (VERIFIED 2026-01-07)
// =============================================================================
// Audio is loaded from Secondary Assets 601/602 and Tertiary Asset 700.
// The game uses PSX SPU with ADPCM-encoded samples.
//
// Loading flow (from InitializeAndLoadLevel @ 0x8007d1d0):
//   1. Load secondary segment containing Assets 601 + 602
//   2. Call UploadAudioToSPU(asset601_ptr, asset602_ptr, asset601_size)
//   3. Samples are transferred to SPU RAM starting at 0x1010
//
// Key functions:
//   UploadAudioToSPU @ 0x8007c088 - Uploads samples to SPU RAM
//   GetAsset601Ptr   @ 0x8007ba78 - Returns audio sample bank pointer
//   GetAsset602Ptr   @ 0x8007baa0 - Returns volume/pan table pointer

// =============================================================================
// Secondary Asset 601 - Audio Sample Bank (CODE-VERIFIED)
// =============================================================================
// Contains audio samples for SPU upload. Verified via UploadAudioToSPU.
// Format:
//   u16 sample_count
//   u16 reserved (always 0)
//   AudioSampleEntry[sample_count]
//   ADPCM audio data...

struct AudioSampleEntry {
    u32 sample_id [[name("Sample ID"), comment("Hash/identifier for this sample")]];
    u32 sample_size [[name("SPU Size"), comment("Size in SPU RAM (bytes)")]];
    u32 data_offset [[name("Data Offset"), comment("Offset within audio data block")]];
} [[static, color("80FF80")]];

// Example: SCIE Stage 0 has 13 samples (sizes: 4432, 3296, 5296, 3984, 11072...)

// =============================================================================
// Secondary Asset 602 - Volume/Pan Table (CODE-VERIFIED)
// =============================================================================
// Per-sample volume and pan settings. One entry per sample in Asset 601.
// If Asset 602 is NULL, defaults are used: volume=0x3FFF, pan=0.

struct AudioVolumeEntry {
    u16 volume [[name("Volume"), comment("0-0x3FFF, where 0x3FFF = maximum")]];
    u16 pan [[name("Pan"), comment("0 = center, non-zero = offset")]];
} [[static, color("80FFFF")]];

// Common values:
//   0x3FFF, 0x0000 - Full volume, centered
//   0x2000, 0x0000 - Half volume, centered
//   0x1333, 0x0010 - ~30% volume, slight pan offset

// =============================================================================
// Tertiary Asset 700 - Unknown Audio-Related Data
// =============================================================================
// Only appears in 9 of 26 levels: MENU, SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED
// Stored at LevelDataContext offset 0x54 (pointer) and 0x58 (size).
//
// VERIFIED:
// - Uses standard Asset 601-style container header (count=1, entry with ID/size/offset)
// - Data starts at offset 0x10 within the asset
// - Sizes range from 208-496 bytes
//
// UNCONFIRMED (see docs/unconfirmed_findings.md for analysis):
// - Data content structure is unknown
// - May be sound event sequences, music selection, or unused data
// - No code found that reads this data after LoadAssetContainer stores it

struct Asset700Header {
    u16 entry_count [[name("Entry Count"), comment("Always 1")]];
    u16 reserved [[name("Reserved"), comment("Always 0")]];
    u32 entry_id [[name("Entry ID"), comment("Varies per level")]];
    u32 data_size [[name("Data Size"), comment("Size of data after header")]];
    u32 data_offset [[name("Data Offset"), comment("Always 0x10")]];
} [[static, color("FF6666")]];

struct Asset700Data<auto asset_size> {
    Asset700Header header;
    
    // Raw data - structure unknown
    // See docs/unconfirmed_findings.md for analysis attempts
    if (asset_size > 16) {
        u8 raw_data[asset_size - 16] [[name("Raw Data"), comment("Unknown structure - see unconfirmed_findings.md")]];
    }
} [[color("FF4444")]];

// =============================================================================
// Asset 504 (0x1F8) - Vehicle Level Data (UNCONFIRMED)
// =============================================================================
// Only appears in vehicle/driving levels:
//   - FINN (Level 4): Submarine level, 4992 bytes (155 entries)
//   - RUNN (Level 22): Auto-runner level, 64 bytes (2 entries)
//
// Structure appears to be 32-byte paired entries with bounding boxes,
// likely defining collectible positions and path waypoints for vehicle gameplay.
//
// Entry types (based on type_flags1):
//   0x0000 = Zone trigger (path boundaries)
//   0x8000 = Collectible marker (e.g., clay pods)
//
// Common reference values:
//   251 (0xFB) = Collectible type
//   1351 (0x547) = Zone link reference
//   1601 (0x641) = Start/end marker

struct VehicleZoneEntry {
    // Bounding box in pixels (0x00-0x07)
    s16 x1 [[name("X1"), comment("Zone min X (pixels)")]];
    s16 y1 [[name("Y1"), comment("Zone min Y (pixels)")]];
    s16 x2 [[name("X2"), comment("Zone max X (pixels)")]];
    s16 y2 [[name("Y2"), comment("Zone max Y (pixels)")]];
    
    // Center point (0x08-0x0B)
    s16 center_x [[name("Center X"), comment("Zone center X, or 0 for linked zones")]];
    s16 center_y [[name("Center Y"), comment("Zone center Y, or type indicator")]];
    
    // Type flags (0x0C-0x0F)
    u16 type_flags1 [[name("Type1"), comment("0x0000=zone, 0x8000=collectible")]];
    u16 type_flags2 [[name("Type2"), comment("1=zone type A, 2=zone type B")]];
} [[static, color("FFCC80")]];

struct VehicleDataEntry {
    // Flags and references (0x00-0x07)
    u16 flags [[name("Flags"), comment("0, 1351, or 1601")]];
    u16 index [[name("Index"), comment("Usually 0 or 1")]];
    u16 ref_id [[name("Ref ID"), comment("251=collectible, 1351=zone link, 0=waypoint")]];
    u16 reserved1 [[name("Reserved")]];
    
    // Timing and next waypoint (0x08-0x0F)
    u16 delay [[name("Delay"), comment("Frame count (5-149 range)")]];
    u16 reserved2 [[name("Reserved")]];
    u16 next_x [[name("Next X"), comment("Next waypoint X or entry index")]];
    u16 next_y [[name("Next Y"), comment("Next waypoint Y")]];
} [[static, color("80CCFF")]];

struct VehiclePairedEntry {
    VehicleZoneEntry zone [[name("Zone")]];
    VehicleDataEntry data [[name("Data")]];
} [[static, color("FFFFCC")]];

// Asset 504 with size template parameter for calculating entry count
struct Asset504VehicleData<auto asset_size> {
    u32 entry_count [[name("Entry Count"), comment("Raw count field (may not match actual entries)")]];
    u32 flags [[name("Flags"), comment("FINN=16, RUNN=1")]];
    // Calculate actual entries from asset size: (size - 8 byte header) / 32 bytes per entry
    VehiclePairedEntry entries[(asset_size - 8) / 32] [[name("Entries")]];
} [[name("Vehicle Data"), color("FFCC80")]];

// =============================================================================
// Level Palette - Asset 602 (0x25A) in PRIMARY
// =============================================================================

// Level palette in Primary segment (different from tile palettes)
// Contains 15-bit PSX colors, size varies by level
// Use at specific offset with known size

// =============================================================================
// BS/MDEC Frame Header (8 bytes)
// =============================================================================
// PlayStation Bitstream compressed still image header.
// Magic 0x3800 identifies BS/MDEC format.
// Used for loading screens (PIRA, LEGL, etc.) and cut content.
//
// To decode: jpsxdec -f <file.bs> -static bs -dim 320x256 -fmt png

// BS frame quality level (higher = more compression, lower quality)
fn format_bs_quality(u16 quant) {
    if (quant <= 2) return "High";
    if (quant <= 4) return "Medium-High";
    if (quant <= 7) return "Medium";
    if (quant <= 12) return "Low";
    return "Very Low";
};

// Format BS frame info
fn format_bs_frame(auto f) {
    u32 data_bytes = f.frame_size_words * 2;
    return std::format("BS v{}: {} bytes, Q={} ({})", 
        f.version, data_bytes, f.quant_scale, format_bs_quality(f.quant_scale));
};

// BS/MDEC version
enum BSVersion : u16 {
    Version2 = 0x0002,  // Standard version (STR v2)
    Version3 = 0x0003   // Alternate version (STR v3)
};

struct BSFrameHeader {
    u16 frame_size_words [[name("Size"), comment("Frame data size in 16-bit words (includes header)")]];
    u16 magic [[name("Magic"), comment("Must be 0x3800 for valid BS frame"), color("FF4040")]];
    u16 quant_scale [[name("Quant"), comment("Quantization scale (lower = better quality)"), color("40FF40")]];
    BSVersion version [[name("Version"), comment("BS format version (2 or 3)"), color("4040FF")]];
} [[name("BS Frame Header"), format("format_bs_frame"), color("FF8080"), static]];

// Validate BS magic
fn is_valid_bs(auto h) {
    return h.magic == 0x3800;
};

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
// Sector Table Entry (0x10 = 16 bytes) - PAL: 0xCD0, JP: 0xCB0
// =============================================================================
// The sector table contains 32 entries pointing to BS/MDEC loading screen frames.
//
// PAL/NTSC-US (table at 0xCD0):
//   [0]  PIRA Title Screen    [1]  LEGL Legal Screen    [2]  CRED Credits
//   [3]  MENU Options         [4-28] Level loading screens
//   [29] EVIL (end loading)   [30] OVER Game Over       [31] OVER Alt / Password
//
// JP (table at 0xCB0, 32 bytes earlier):
//   [0]  PIRA Title           [1]  LEGL Legal           [2]  MENU (no CRED!)
//   [3-27] Level loading      [28] CRED Credits (moved) [29] OVER Game Over
//   [30] OVER Alt
//
// JP BUG FIX: In PAL, CRED at index 2 caused credits to trigger between the
// two intro movies. JP fixed this by splicing intro into 1 movie and moving
// CRED to index 28. (Source: https://tcrf.net/Skullmonkeys)
//
// LAYOUT VARIANTS (same 16-byte size, different field order):
// 
// PAL/NTSC-US Layout:           JP Layout (SLPS-01501):
//   0x00: u8  level_index         0x00: u16 sector_offset
//   0x01: u8  entry_flags         0x02: u16 sector_count
//   0x02: u8  unknown_byte        0x04: u8  level_index
//   0x03: char[5] code            0x05: u8  entry_flags
//   0x08: char[4] short_name      0x06: u8  unknown_byte
//   0x0C: u16 sector_offset       0x07: char[5] code
//   0x0E: u16 sector_count        0x0C: char[4] short_name
//
// Detection: At 0xCD0, PAL byte[3] is A-Z (code). JP byte[3] is 0x00 (part of MENU).

// Entry type flags
enum SectorEntryFlags : u8 {
    Level = 0x00,           // Level loading screen
    GameOver = 0x03,        // Game over screen
    Special = 0x05          // Special loading screen
};

// PAL/NTSC-US Layout (code at byte 3, offset/count at end)
// For loading/splash screens (entry_flags 0x03 or 0x05):
//   entry_flags = minimum display time in seconds (before skip allowed)
//   display_timeout = maximum display time in seconds (99 = special Game Over mode)
struct SectorEntryPAL {
    u8 level_index [[name("Level"), comment("Level index (0-25)")]];
    SectorEntryFlags entry_flags [[name("Type"), color("FF8040"), comment("Entry type AND min display time (seconds) for screens")]];
    u8 display_timeout [[name("Timeout"), comment("Max display time (seconds). 0=no display, 99=Game Over mode")]];
    char code[5] [[name("Code"), comment("4-char null-terminated ID")]];
    char short_name[4] [[name("Short"), comment("Truncated description")]];
    type::Hex<u16> sector_offset [[name("Sector"), comment("Sector offset in BLB")]];
    u16 sector_count [[name("Count"), comment("Number of sectors")]];
} [[name("Sector Entry (PAL)"), format("format_sector_offset"), color("80CCCC"), static]];

// JP Layout (offset/count at front, code at byte 7)
// Same timing fields as PAL layout
struct SectorEntryJP {
    type::Hex<u16> sector_offset [[name("Sector"), comment("Sector offset in BLB")]];
    u16 sector_count [[name("Count"), comment("Number of sectors")]];
    u8 level_index [[name("Level"), comment("Level index (0-25)")]];
    SectorEntryFlags entry_flags [[name("Type"), color("FF8040"), comment("Entry type AND min display time (seconds) for screens")]];
    u8 display_timeout [[name("Timeout"), comment("Max display time (seconds). 0=no display, 99=Game Over mode")]];
    char code[5] [[name("Code"), comment("4-char null-terminated ID")]];
    char short_name[4] [[name("Short"), comment("Truncated description")]];
} [[name("Sector Entry (JP)"), format("format_sector_jp"), color("80CCCC"), static]];

// Format for JP layout (sector is at different offset in struct)
fn format_sector_jp(auto entry) {
    u32 file_offset = entry.sector_offset * SECTOR_SIZE;
    return std::format("Sector {} (0x{:06X})", entry.sector_offset, file_offset);
};

// Auto-detecting wrapper that selects the correct layout
struct SectorEntry {
    if (g_is_jp) {
        SectorEntryJP entry [[inline]];
    } else {
        SectorEntryPAL entry [[inline]];
    }
} [[name("Sector Entry"), color("80CCCC"), static]];

// =============================================================================
// Level Metadata Entry (0x70 = 112 bytes)
// =============================================================================

struct LevelEntry {
    // Primary data pointers (0x00-0x0B)
    type::Hex<u16> sector_offset [[name("Sector"), comment("Primary data sector offset")]];
    u16 sector_count [[name("Count"), comment("Primary data sector count")]];
    type::Hex<u32> primary_buffer_size [[name("BufSize"), comment("Total primary buffer size for level (returned by GetPrimaryBufferSize @ 8007a5cc)")]];
    type::Hex<u32> entry1_offset [[name("Entry1 Off"), comment("Offset to Entry[1] (asset 601/collision) in primary TOC")]];
    
    // Level identification (0x0C-0x0D)
    u8 level_index [[name("Index"), comment("Level asset index"), color("40FF40")]];
    u8 level_flag [[name("Selectable"), comment("1=password-selectable level, 0=not selectable (bosses, special modes, later worlds)")]];
    
    // Stage count and tertiary data offsets (0x0E-0x1D)
    // tert_data_off[i] << 5 gives the tertiary data size for stage i (used by GetCurrentTertiaryDataSize)
    u16 stage_count [[name("Stages"), comment("Number of stages in this level (1-6)")]];
    u16 tert_data_off[6] [[name("Tert Data Off"), comment("Per-stage tertiary data offsets (value << 5 = size)")]];
    u16 pad_1c [[hidden]];
    
    // Secondary sector locations (0x1E-0x39) - 6 stages
    // Stage 0 uses sec_sector_off[0], stage N uses sec_sector_off[N]
    u16 sec_sector_off[6] [[name("Sec Sect Off"), comment("Per-stage secondary sector offsets")]];
    u16 pad_2a [[hidden]];
    u16 sec_sector_cnt[6] [[name("Sec Sect Cnt"), comment("Per-stage secondary sector counts")]];
    u16 pad_38 [[hidden]];
    
    // Tertiary sector locations (0x3A-0x55) - 6 stages
    u16 tert_sector_off[6] [[name("Tert Sect Off"), comment("Per-stage tertiary sector offsets")]];
    u16 pad_46 [[hidden]];
    u16 tert_sector_cnt[6] [[name("Tert Sect Cnt"), comment("Per-stage tertiary sector counts")]];
    u16 pad_54 [[hidden]];
    
    // Level strings (0x56-0x6F)
    char level_id[5] [[name("ID"), comment("4-char null-terminated ID"), color("FFFF80")]];
    char name[21] [[name("Name"), comment("Level display name"), color("80FF80")]];
} [[name("Level"), format("format_level_name"), color("40CCFF"), static]];

// =============================================================================
// Credits Sequence Entry (0x0C = 12 bytes)
// =============================================================================

// Credits table at 0xF10 (PAL) / 0xEF4 (JP), accessed when playback mode == 2
// Only 2 entries fit before count fields at 0xF31 (PAL) / 0xF15 (JP)
//
// PAL Layout:
//   Entry[0] at 0xF10: code='', sector=26, count=171 → Credits at 0xD000 (20 BS frames)
//   Entry[1] at 0xF1C: code='CRD1', sector=36929, count=171 → Duplicate at 0x4820800
//
// JP Layout:
//   Entry[0] at 0xEF4: code='', sector=38343, count=182 → Credits at 0x4AE3800 (21 BS frames)
//   Entry[1] at 0xF00: code='CRD2', sector=0, count=0 → No duplicate
//
// The credits container has interlaced BS frames (pairs for CRT display).
// PAL has credits early + duplicate at end (CD seek optimization).
// JP has credits only at end of file.

struct CreditsEntry {
    char code[4] [[name("Code"), comment("4-char ID: '', 'CRD1', 'CRD2'"), color("FFCC40")]];
    u8 pad[4] [[hidden]];  // Always 0x00000000
    u16 sector_offset [[name("Sector"), comment("Sector number where credits container starts")]];
    u16 sector_count [[name("Count"), comment("Number of sectors for credits data")]];
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
//   6 = PasswordScreen - Load password screen from table at 0xECC (17 entries)
//                        Accessed as: header + (level_index * 4) + 0xECC
enum PlaybackMode : u8 {
    Invalid = 0,
    Movie = 1,
    Credits = 2,
    Level = 3,
    SpecialSector4 = 4,
    SpecialSector5 = 5,
    PasswordScreen = 6
};

// =============================================================================
// PASSWORD SCREEN TABLE (17 ENTRIES AT OFFSET 0xECC)
// =============================================================================
// Password screen sector entries are accessed via: header + (index * 4) + 0xECC
// 
// IMPORTANT: Entry[0] overlaps with sectors[31].sector_offset/sector_count!
// The game code calculates: base + index*4 + 0xECC, so:
//   - Entry[0] at 0x0ECC = sectors[31] offset/count fields (4895, 130)
//   - Entry[1] at 0x0ED0 = first dedicated password entry
//   - Entry[16] at 0x0F0C = last password entry (Victory screen)
//
// These 17 containers are WORLD COMPLETION PASSWORD SCREENS.
// Each has 11 assets (except Victory with 12): tile header, tilemaps, layers,
// pixels, palettes, etc. for rendering the password display.

struct PasswordScreenEntry {
    type::Hex<u16> sector_offset [[name("Sector"), comment("Sector offset in BLB")]];
    u16 sector_count [[name("Count"), comment("Number of sectors")]];
} [[static, color("CC80FF")]];

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

struct PlaybackSequenceDataPAL {
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
    //   mode=6: index into password_screens[17] at 0xECC (0-16)
    u8 indices[110] [[name("Indices"), comment("Table index for each step"), color("80FF80")]];
} [[name("Playback Sequence"), color("CC80CC")]];

// JP version has shifted offsets (everything -0x1C from PAL)
struct PlaybackSequenceDataJP {
    u8 unknown_00[2] [[hidden]];
    
    // Mode array: 92 entries starting at 0xF1A (PAL 0xF36 - 0x1C)
    PlaybackMode modes[92] [[name("Modes"), comment("Playback mode for each step"), color("FF8080")]];
    
    // Index array: 110 entries starting at 0xF76 (PAL 0xF92 - 0x1C)  
    u8 indices[110] [[name("Indices"), comment("Table index for each step"), color("80FF80")]];
} [[name("Playback Sequence"), color("CC80CC")]];

// =============================================================================
// BLB Header - PAL/NTSC-US Layout (0x1000 = 4096 bytes)
// =============================================================================
// Layout: levels[26] @ 0x000, movies[13] @ 0xB60, pad[4] @ 0xCCC, sectors[32] @ 0xCD0
//         password[16] @ 0xED0, credits[2] @ 0xF10, counts @ 0xF31, playback @ 0xF34

struct BLBHeaderPAL {
    LevelEntry levels[26] [[name("Levels"), comment("Level metadata (26 entries)"), color("40CCFF")]];
    MovieEntry movies[13] [[name("Movies"), comment("Movie table (13 entries)"), color("FFFF40")]];
    u8 movie_sector_pad[4] [[hidden]];  // 4-byte gap between movies (0xCCC) and sectors (0xCD0)
    SectorEntryPAL sectors[32] [[name("Sectors"), comment("Sector table (32 entries) @ 0xCD0"), color("80CCCC")]];
    
    // Password Screen table: 16 additional entries at 0xED0-0xF0F (64 bytes)
    // NOTE: Entry[0] is at 0xECC, overlapping with sectors[31].sector_offset/count
    PasswordScreenEntry password_screens[16] [[name("Password Screens"), 
        comment("Password entries 1-16 (entry 0 is in sectors[31])"), color("CC80FF")]];
    
    CreditsEntry credits[2] [[name("Credits"), comment("Credits entries (only 2 complete)"), color("FFCC80")]];
    u8 gap_to_counts[9] [[hidden]];  // Gap from credits end (0xF28) to counts (0xF31)
    
    u8 level_count [[name("Level Count"), comment("Number of levels (26)"), color("40FF40")]];
    u8 movie_count [[name("Movie Count"), comment("Number of movies (13)"), color("FFFF40")]];
    u8 sector_entry_count [[name("Sector Count"), comment("Sector table entries (32)"), color("80CCCC")]];
    
    // Playback sequence state machine (0xF34-0xFFF)
    PlaybackSequenceDataPAL playback [[name("Playback"), comment("Sequence mode/index arrays"), color("CC80CC")]];
} [[name("BLB Header (PAL)"), comment("PAL/NTSC-US layout"), single_color]];

// =============================================================================
// BLB Header - JP Layout (0x1000 = 4096 bytes)
// =============================================================================
// Layout: levels[26] @ 0x000, movies[12] @ 0xB60, sectors[31] @ 0xCB0 (no gap!)
//         password[16] @ 0xEB0, credits[2] @ 0xEF4, counts @ 0xF15, playback @ 0xF18
//
// JP differences:
//   - 12 movies (intro spliced into 1)
//   - 31 sector entries (CRED moved from index 2 to index 28)
//   - No padding between movies and sectors
//   - All offsets after movies shifted by -0x20 (32 bytes = 1 fewer movie + no pad)
//   - Count fields shifted by -0x1C (28 bytes)

struct BLBHeaderJP {
    LevelEntry levels[26] [[name("Levels"), comment("Level metadata (26 entries)"), color("40CCFF")]];
    MovieEntry movies[12] [[name("Movies"), comment("Movie table (12 entries - intro spliced)"), color("FFFF40")]];
    // No padding - sectors start immediately after movies at 0xCB0
    SectorEntryJP sectors[31] [[name("Sectors"), comment("Sector table (31 entries) @ 0xCB0"), color("80CCCC")]];
    
    // Password Screen table: 16 entries at 0xEB0-0xEEF (shifted by -0x20)
    PasswordScreenEntry password_screens[16] [[name("Password Screens"), 
        comment("Password entries 1-16"), color("CC80FF")]];
    
    CreditsEntry credits[2] [[name("Credits"), comment("Credits entries"), color("FFCC80")]];
    u8 gap_to_counts[9] [[hidden]];  // Gap from credits end (0xF0C) to counts (0xF15)
    
    u8 level_count [[name("Level Count"), comment("Number of levels (26)"), color("40FF40")]];
    u8 movie_count [[name("Movie Count"), comment("Number of movies (12)"), color("FFFF40")]];
    u8 sector_entry_count [[name("Sector Count"), comment("Sector table entries (31)"), color("80CCCC")]];
    
    // Playback sequence state machine (0xF18-0xFFF) - shifted by -0x1C
    PlaybackSequenceDataJP playback [[name("Playback"), comment("Sequence mode/index arrays"), color("CC80CC")]];
} [[name("BLB Header (JP)"), comment("Japanese version layout"), single_color]];

// =============================================================================
// Auto-detecting BLB Header wrapper
// =============================================================================

struct BLBHeader {
    if (g_is_jp) {
        BLBHeaderJP header [[inline]];
    } else {
        BLBHeaderPAL header [[inline]];
    }
} [[name("BLB Header"), single_color]];

// =============================================================================
// Apply BLB Header at file start
// =============================================================================

BLBHeader header @ 0x00 [[name("GAME.BLB Header"), comment("Main archive header")]];

// =============================================================================
// EXTRACTABLE HEADER CONTAINERS
// =============================================================================
// These structures expose header tables (sectors, credits, password_screens) with
// a TOC-like interface so extraction tools can discover them generically.
// 
// The extraction script looks for any dict with:
//   - "_base" field (segment/container offset)
//   - "toc" array with entries having: asset_type, size, offset, asset_name
//
// Header containers use virtual asset types 1000+ to distinguish from level assets:
//   1000 = BS loading screen (from sectors table)
//   1001 = BS credits frame (from credits table) 
//   1002 = BS password screen (from password_screens table)
//   1003 = BS unreferenced (known cut content)

// Virtual TOC entry for header extractables
struct HeaderTOCEntry {
    u32 asset_type [[export]];
    u32 size [[export]];
    type::Hex<u32> offset [[export]];  // Absolute byte offset in file
    str asset_name [[export]];
};

// Build a virtual TOC for sectors table
fn build_sectors_toc() {
    // This would need ImHex scripting to iterate - for now we rely on
    // the extraction script reading the sectors array directly
};

// =============================================================================
// ASSET TYPE NAME LOOKUP (for extraction tools)
// =============================================================================
// This function returns a human-readable name for each asset type ID.
// Used by SegmentTOCEntry to include asset_name in JSON export.

fn get_asset_name(u32 id) {
    if (id == 100) return "tile_header";
    if (id == 101) return "vram_slot_config";
    if (id == 200) return "tilemap_container";
    if (id == 201) return "layer_entries";
    if (id == 300) return "tile_pixels";
    if (id == 301) return "palette_indices";
    if (id == 302) return "tile_flags";
    if (id == 303) return "animated_tiles";
    if (id == 400) return "palette_container";
    if (id == 401) return "palette_anim";
    if (id == 500) return "tile_attributes";
    if (id == 501) return "entities";
    if (id == 502) return "vram_rects";
    if (id == 503) return "anim_offsets";
    if (id == 504) return "vehicle_data";
    if (id == 600) return "geometry_or_sprites";
    if (id == 601) return "audio_samples";
    if (id == 602) return "palette_data";
    if (id == 700) return "spu_audio";
    return "unknown";
};

// =============================================================================
// LEVEL DATA STRUCTURES
// =============================================================================
// The following structures parse the actual level content stored at sector offsets.
// Each level has Primary, Secondary, and Tertiary data segments.

// -----------------------------------------------------------------------------
// Segment TOC Entry - Alias of generic TOCEntry for segments
// -----------------------------------------------------------------------------
// For segments: id = asset_type (100, 200, 300, 400, 600, etc.)
// The absolute_offset is computed when placed inside a segment struct.

struct SegmentTOCEntry {
    u32 asset_type [[name("Type"), comment("Asset type ID"), color("FFCC40")]];
    u32 size [[name("Size"), comment("Asset size in bytes")]];
    type::Hex<u32> offset [[name("Offset"), comment("Offset from segment start")]];
    str asset_name = get_asset_name(asset_type) [[export, name("Name")]];
};

// Format segment TOC entry
fn format_toc_entry(auto e) {
    return std::format("Type {} ({} bytes @ 0x{:X})", e.asset_type, e.size, e.offset);
};

// -----------------------------------------------------------------------------
// Segment TOC - Table of Contents for a data segment
// -----------------------------------------------------------------------------
struct SegmentTOC {
    u32 entry_count [[name("Count"), comment("Number of TOC entries")]];
    SegmentTOCEntry entries[entry_count] [[name("Entries")]];
} [[name("Segment TOC"), color("FF8040")]];

// -----------------------------------------------------------------------------
// Sub-TOC Entry - Alias of generic TOCEntry for containers
// -----------------------------------------------------------------------------
// For containers: id = entry index or sub-asset ID

using SubTOCEntry = TOCEntry;

// =============================================================================
// PALETTE CONTAINER - Asset 400 (0x190)
// =============================================================================

// Full palette container with inline palette visualization
struct PaletteContainerAsset {
    // Remember base for offset calculations
    u128 _container_base = $;
    
    u32 palette_count [[name("Count"), comment("Number of palettes")]];
    SubTOCEntry palette_toc[palette_count] [[name("Palette TOC")]];
    
    // Note: Palettes are 512 bytes each (256 x 16-bit PSX colors)
    // Access at: container_base + palette_toc[n].offset
} [[name("Palette Container"), color("FF80FF")]];

// =============================================================================
// TILEMAP CONTAINER - Asset 200 (0x0C8)
// =============================================================================

// Tilemap sub-entry (12 bytes, same as TOCEntry)
// Note: Tile count = size / 2 (each tile is 2 bytes)
struct TilemapSubEntry {
    u32 layer_id [[name("Layer"), comment("Layer index")]];
    u32 size [[name("Size"), comment("Tilemap data size in bytes (÷2 = tiles)")]];
    type::Hex<u32> offset [[name("Offset"), comment("Offset from container start")]];
} [[static, color("FFCC80")]];

// Tilemap data block - array of u16 tile indices
struct TilemapData {
    u16 tiles[while(!std::mem::eof())] [[name("Tiles"), color("80CCFF")]];
} [[name("Tilemap Data")]];

// Complete tilemap container
struct TilemapContainerAsset {
    u32 layer_count [[name("Count"), comment("Number of tilemap layers")]];
    TilemapSubEntry tilemap_toc[layer_count] [[name("Tilemap TOC")]];
} [[name("Tilemap Container"), color("FFCC80")]];

// =============================================================================
// SPRITE CONTAINER - Asset 600 (0x258)
// =============================================================================

// Sprite container - shows TOC only (sprites are complex, parse manually)
struct SpriteContainerAsset {
    u32 sprite_count [[name("Count"), comment("Number of sprites")]];
    SubTOCEntry sprite_toc[sprite_count] [[name("Sprite TOC")]];
    // To view a sprite: go to container_start + sprite_toc[n].offset
    // Each sprite has: header (12 bytes) + animations + frames + RLE + palette
} [[name("Sprite Container"), color("FF8080")]];

// =============================================================================
// AUDIO SAMPLE BANK - Asset 601 (0x259) - CODE-VERIFIED
// =============================================================================
// Contains SPU ADPCM audio samples. Verified via UploadAudioToSPU @ 0x8007c088.
// Format: u16 count, u16 reserved, AudioSampleEntry[count], ADPCM data...
// See AudioSampleEntry struct in Audio System section above for entry format.
// NOTE: Primary segment also has Asset 601 type ID but with different content.

struct AudioSampleBankAsset {
    u16 sample_count [[name("Sample Count"), comment("Number of audio samples")]];
    u16 reserved [[name("Reserved"), comment("Always 0")]];
    AudioSampleEntry samples[sample_count] [[name("Sample Entries")]];
    // Followed by ADPCM audio data (size = total_size - 4 - sample_count * 12)
} [[name("Audio Sample Bank"), color("80FF80")]];

// -----------------------------------------------------------------------------
// Container Asset Header - for assets with sub-TOC (0x258, 0x259, 0x190)
// -----------------------------------------------------------------------------
struct ContainerAssetHeader {
    u32 sub_count [[name("Sub-Count"), comment("Number of sub-entries")]];
    SubTOCEntry sub_entries[sub_count] [[name("Sub-TOC")]];
} [[name("Container"), color("80FFCC")]];

// -----------------------------------------------------------------------------
// Tile Header - Asset 100 (0x064), 36 bytes
// Uses the main TileHeader struct defined earlier
// -----------------------------------------------------------------------------
using TileHeaderAsset = TileHeader;

// =============================================================================
// LEVEL DATA EXPLORER
// =============================================================================
// These structures parse level data into a browsable tree structure.

// Helper to calculate file offset from sector
fn sector_to_offset(u16 sector) {
    return sector * SECTOR_SIZE;
};

// -----------------------------------------------------------------------------
// Tile Pixel Data Block - for visualization
// -----------------------------------------------------------------------------

// Single 16x16 tile (256 bytes) - can be visualized with a palette
struct TilePixelBlock {
    u8 pixels[256];  // 16x16 tile = 256 bytes (8bpp indexed)
} [[name("Tile 16x16"), color("80FF80")]];

struct TilePixelBlock8x8 {
    u8 pixels[64];  // 8x8 tile = 64 bytes (8bpp indexed)
} [[name("Tile 8x8"), color("80CCFF")]];

// Visualizable tile - includes both pixels and palette for bitmap visualizer
// Use this at the tile pixel offset, palette must be passed as template param
struct VisualTile16x16<auto palette_ptr> {
    u8 pixels[256];
} [[name("Tile 16x16"), color("80FF80"), 
    hex::visualize("bitmap", pixels, 16, 16, palette_ptr)]];

struct VisualTile8x8<auto palette_ptr> {
    u8 pixels[64];
} [[name("Tile 8x8"), color("80CCFF"),
    hex::visualize("bitmap", pixels, 8, 8, palette_ptr)]];

// -----------------------------------------------------------------------------
// Tile Sheet - All tiles in a row for visualization
// -----------------------------------------------------------------------------
// Helper to visualize multiple tiles as a sprite sheet
struct TileSheet16x16<auto tile_count, auto palette_ptr> {
    u8 pixels[256 * tile_count];
} [[name("Tile Sheet"), color("80FF80"),
    hex::visualize("bitmap", pixels, 16, tile_count * 16, palette_ptr)]];

// Single tile with inline palette for self-contained visualization
struct TileWithPalette {
    u8 pixels[256];           // 16x16 8bpp tile
    PSXColor palette[256];    // 256-entry CLUT
} [[name("Tile+Palette"), color("80FF80"),
    hex::visualize("bitmap", pixels, 16, 16, palette)]];

// -----------------------------------------------------------------------------
// Single Sprite with full data and palette visualization
// -----------------------------------------------------------------------------
struct SpriteAsset {
    // Header (12 bytes)
    u16 animation_count [[name("Animations")]];
    u16 frame_meta_offset [[name("Frame Offset")]];
    u32 rle_offset [[name("RLE Offset")]];
    u32 palette_offset [[name("Palette Offset")]];
    
    // Animation entries follow header
    AnimationEntry animations[animation_count] [[name("Animations")]];
    
    // Note: Palette is at palette_offset from sprite start (512 bytes = 256 colors)
    // Frame metadata is at frame_meta_offset
    // RLE compressed pixels at rle_offset
} [[name("Sprite"), color("FF8080")]];

// -----------------------------------------------------------------------------
// Primary Segment - Full Parse
// -----------------------------------------------------------------------------
struct PrimarySegmentFull {
    // Remember base for offset calculations - export for extraction tools
    type::Hex<u128> _base = $ [[export, name("segment_offset")]];
    
    // TOC
    u32 toc_count [[name("Asset Count")]];
    SegmentTOCEntry toc[toc_count] [[name("Asset TOC")]];
    
    // Parse Asset 600 (Sprite Container) 
    if (toc_count > 0 && toc[0].asset_type == 600) {
        SpriteContainerAsset sprites0 @ (_base + toc[0].offset) [[name("Sprites")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 600) {
        SpriteContainerAsset sprites1 @ (_base + toc[1].offset) [[name("Sprites")]];
    }
    
    // Note: Asset 601 in Primary segment is Audio Sample Bank (SPU ADPCM data)
    // Not parsed here - raw audio data without structured header
} [[name("Primary Segment"), color("40CCFF")]];

// -----------------------------------------------------------------------------
// Secondary Segment - Full Parse with Tiles
// -----------------------------------------------------------------------------
struct SecondarySegmentFull {
    // Remember base for offset calculations - export for extraction tools
    type::Hex<u128> _base = $ [[export, name("segment_offset")]];
    
    // TOC
    u32 toc_count [[name("Asset Count")]];
    SegmentTOCEntry toc[toc_count] [[name("Asset TOC")]];
    
    // Parse Asset 100 (Tile Header) - usually first
    if (toc_count > 0 && toc[0].asset_type == 100) {
        TileHeaderAsset tile_header @ (_base + toc[0].offset) [[name("Tile Header")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 100) {
        TileHeaderAsset tile_header1 @ (_base + toc[1].offset) [[name("Tile Header")]];
    }
    
    // Parse Asset 101 (VRAM Slot Config - controls texture page allocation)
    // VERIFIED 2026-01-13: bank_a_count + bank_b_count determines texture slots
    if (toc_count > 0 && toc[0].asset_type == 101) {
        VRAMSlotConfig vram_config_0 @ (_base + toc[0].offset) [[name("VRAM Slot Config")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 101) {
        VRAMSlotConfig vram_config_1 @ (_base + toc[1].offset) [[name("VRAM Slot Config")]];
    }
    if (toc_count > 2 && toc[2].asset_type == 101) {
        VRAMSlotConfig vram_config_2 @ (_base + toc[2].offset) [[name("VRAM Slot Config")]];
    }
    
    // Parse Asset 400 (Palette Container) 
    if (toc_count > 0 && toc[0].asset_type == 400) {
        PaletteContainerAsset palettes0 @ (_base + toc[0].offset) [[name("Palettes")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 400) {
        PaletteContainerAsset palettes1 @ (_base + toc[1].offset) [[name("Palettes")]];
    }
    if (toc_count > 2 && toc[2].asset_type == 400) {
        PaletteContainerAsset palettes2 @ (_base + toc[2].offset) [[name("Palettes")]];
    }
    if (toc_count > 3 && toc[3].asset_type == 400) {
        PaletteContainerAsset palettes3 @ (_base + toc[3].offset) [[name("Palettes")]];
    }
    
    // Parse Asset 302 (Tile Flags) - 1 byte per tile
    if (toc_count > 0 && toc[0].asset_type == 302) {
        TileFlags tile_flags0[toc[0].size] @ (_base + toc[0].offset) [[name("Tile Flags")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 302) {
        TileFlags tile_flags1[toc[1].size] @ (_base + toc[1].offset) [[name("Tile Flags")]];
    }
    if (toc_count > 2 && toc[2].asset_type == 302) {
        TileFlags tile_flags2[toc[2].size] @ (_base + toc[2].offset) [[name("Tile Flags")]];
    }
    if (toc_count > 3 && toc[3].asset_type == 302) {
        TileFlags tile_flags3[toc[3].size] @ (_base + toc[3].offset) [[name("Tile Flags")]];
    }
    if (toc_count > 4 && toc[4].asset_type == 302) {
        TileFlags tile_flags4[toc[4].size] @ (_base + toc[4].offset) [[name("Tile Flags")]];
    }
    
    // Parse Asset 301 (Palette Indices) - 1 byte per tile (palette index from Asset 400)
    if (toc_count > 0 && toc[0].asset_type == 301) {
        u8 palette_indices0[toc[0].size] @ (_base + toc[0].offset) [[name("Palette Indices")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 301) {
        u8 palette_indices1[toc[1].size] @ (_base + toc[1].offset) [[name("Palette Indices")]];
    }
    if (toc_count > 2 && toc[2].asset_type == 301) {
        u8 palette_indices2[toc[2].size] @ (_base + toc[2].offset) [[name("Palette Indices")]];
    }
    if (toc_count > 3 && toc[3].asset_type == 301) {
        u8 palette_indices3[toc[3].size] @ (_base + toc[3].offset) [[name("Palette Indices")]];
    }
    if (toc_count > 4 && toc[4].asset_type == 301) {
        u8 palette_indices4[toc[4].size] @ (_base + toc[4].offset) [[name("Palette Indices")]];
    }
    
    // Parse Asset 300 (Tile Pixels) - raw 8bpp indexed pixel data
    // 16x16 tiles = 256 bytes each, 8x8 tiles = 64 bytes each
    if (toc_count > 0 && toc[0].asset_type == 300) {
        u8 tile_pixels0[toc[0].size] @ (_base + toc[0].offset) [[name("Tile Pixels")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 300) {
        u8 tile_pixels1[toc[1].size] @ (_base + toc[1].offset) [[name("Tile Pixels")]];
    }
    if (toc_count > 2 && toc[2].asset_type == 300) {
        u8 tile_pixels2[toc[2].size] @ (_base + toc[2].offset) [[name("Tile Pixels")]];
    }
    if (toc_count > 3 && toc[3].asset_type == 300) {
        u8 tile_pixels3[toc[3].size] @ (_base + toc[3].offset) [[name("Tile Pixels")]];
    }
    if (toc_count > 4 && toc[4].asset_type == 300) {
        u8 tile_pixels4[toc[4].size] @ (_base + toc[4].offset) [[name("Tile Pixels")]];
    }
    if (toc_count > 5 && toc[5].asset_type == 300) {
        u8 tile_pixels5[toc[5].size] @ (_base + toc[5].offset) [[name("Tile Pixels")]];
    }
} [[name("Secondary Segment"), color("FF80CC")]];

// -----------------------------------------------------------------------------
// Tertiary Segment - Full Parse with Sprites/Layers
// -----------------------------------------------------------------------------
struct TertiarySegmentFull {
    // Remember base for offset calculations - export for extraction tools
    type::Hex<u128> _base = $ [[export, name("segment_offset")]];
    
    // TOC
    u32 toc_count [[name("Asset Count")]];
    SegmentTOCEntry toc[toc_count] [[name("Asset TOC")]];
    
    // The tertiary segment typically contains:
    // - Asset 200 (0x0C8): Tilemap container
    // - Asset 201 (0x0C9): Layer entries
    // - Asset 600 (0x258): Sprite container (different from primary)
    // - Asset 500-503: Audio data
} [[name("Tertiary Segment"), color("80FFCC")]];

// =============================================================================
// DYNAMIC LEVEL EXPLORER
// =============================================================================
// Dynamically parses all levels based on header.level_count
// Each level has: Primary segment, Secondary segment, and up to 6 Tertiary stages

// -----------------------------------------------------------------------------
// Tertiary Stage - parses a single tertiary sub-block with full asset access
// -----------------------------------------------------------------------------

// Tilemap container with inline tile data
struct TilemapContainerFull {
    // Remember base offset
    u128 _container_base = $;
    
    u32 layer_count [[name("Layers")]];
    TilemapSubEntry entries[layer_count] [[name("Layer TOC")]];
    
    // Parse the actual tilemap data for each layer
    // Each tilemap is an array of u16 tile indices at entries[n].offset
    // We parse the first few layers inline
    if (layer_count > 0) {
        u16 tilemap0[entries[0].size / 2] @ (_container_base + entries[0].offset) 
            [[name("Layer 0 Tiles"), color("80CCFF")]];
    }
    if (layer_count > 1) {
        u16 tilemap1[entries[1].size / 2] @ (_container_base + entries[1].offset) 
            [[name("Layer 1 Tiles"), color("80DDFF")]];
    }
    if (layer_count > 2) {
        u16 tilemap2[entries[2].size / 2] @ (_container_base + entries[2].offset) 
            [[name("Layer 2 Tiles"), color("80EEFF")]];
    }
    if (layer_count > 3) {
        u16 tilemap3[entries[3].size / 2] @ (_container_base + entries[3].offset) 
            [[name("Layer 3 Tiles"), color("80FFFF")]];
    }
    if (layer_count > 4) {
        u16 tilemap4[entries[4].size / 2] @ (_container_base + entries[4].offset) 
            [[name("Layer 4 Tiles"), color("90CCFF")]];
    }
    if (layer_count > 5) {
        u16 tilemap5[entries[5].size / 2] @ (_container_base + entries[5].offset) 
            [[name("Layer 5 Tiles"), color("A0CCFF")]];
    }
} [[name("Tilemaps"), color("FFCC80")]];

// Tertiary stage that parses tilemaps and layers inline
struct TertiaryStageWithAssets {
    // Remember base for offset calculations - export for extraction tools
    type::Hex<u128> _base = $ [[export, name("segment_offset")]];
    
    // TOC at segment start
    u32 toc_count [[name("Asset Count")]];
    SegmentTOCEntry toc[toc_count] [[name("Asset TOC")]];
    
    // Search for Asset 100 (Tile Header) - contains level dimensions for this stage
    if (toc_count > 0 && toc[0].asset_type == 100) {
        TileHeader stage_header @ (_base + toc[0].offset) [[name("Stage Header (100)")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 100) {
        TileHeader stage_header1 @ (_base + toc[1].offset) [[name("Stage Header (100)")]];
    }
    
    // Search for Asset 200 (Tilemap Container) in first 4 TOC entries
    if (toc_count > 0 && toc[0].asset_type == 200) {
        TilemapContainerFull tilemaps @ (_base + toc[0].offset) [[name("Tilemaps")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 200) {
        TilemapContainerFull tilemaps1 @ (_base + toc[1].offset) [[name("Tilemaps")]];
    }
    if (toc_count > 2 && toc[2].asset_type == 200) {
        TilemapContainerFull tilemaps2 @ (_base + toc[2].offset) [[name("Tilemaps")]];
    }
    if (toc_count > 3 && toc[3].asset_type == 200) {
        TilemapContainerFull tilemaps3 @ (_base + toc[3].offset) [[name("Tilemaps")]];
    }
    
    // Search for Asset 201 (Layer Entries) - 92 bytes each
    if (toc_count > 0 && toc[0].asset_type == 201) {
        LayerEntry layers0[toc[0].size / 92] @ (_base + toc[0].offset) [[name("Layers")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 201) {
        LayerEntry layers1[toc[1].size / 92] @ (_base + toc[1].offset) [[name("Layers")]];
    }
    if (toc_count > 2 && toc[2].asset_type == 201) {
        LayerEntry layers2[toc[2].size / 92] @ (_base + toc[2].offset) [[name("Layers")]];
    }
    if (toc_count > 3 && toc[3].asset_type == 201) {
        LayerEntry layers3[toc[3].size / 92] @ (_base + toc[3].offset) [[name("Layers")]];
    }
    
    // Search for Asset 600 (Sprite Container) - with full parsing
    if (toc_count > 0 && toc[0].asset_type == 600) {
        SpriteContainerFull sprites0 @ (_base + toc[0].offset) [[name("Sprites")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 600) {
        SpriteContainerFull sprites1 @ (_base + toc[1].offset) [[name("Sprites")]];
    }
    if (toc_count > 2 && toc[2].asset_type == 600) {
        SpriteContainerFull sprites2 @ (_base + toc[2].offset) [[name("Sprites")]];
    }
    if (toc_count > 3 && toc[3].asset_type == 600) {
        SpriteContainerFull sprites3 @ (_base + toc[3].offset) [[name("Sprites")]];
    }
    
    // Search for Asset 500 (Tile Attribute Map) - collision/trigger data
    // Present in ~98 of 104 stages
    if (toc_count >= 1) { if (toc[0].asset_type == 500) {
        TileAttributeMap<toc[0].size> tile_attrs0 @ (_base + toc[0].offset) [[name("Tile Attributes (500)")]];
    }}
    if (toc_count >= 2) { if (toc[1].asset_type == 500) {
        TileAttributeMap<toc[1].size> tile_attrs1 @ (_base + toc[1].offset) [[name("Tile Attributes (500)")]];
    }}
    if (toc_count >= 3) { if (toc[2].asset_type == 500) {
        TileAttributeMap<toc[2].size> tile_attrs2 @ (_base + toc[2].offset) [[name("Tile Attributes (500)")]];
    }}
    if (toc_count >= 4) { if (toc[3].asset_type == 500) {
        TileAttributeMap<toc[3].size> tile_attrs3 @ (_base + toc[3].offset) [[name("Tile Attributes (500)")]];
    }}
    if (toc_count >= 5) { if (toc[4].asset_type == 500) {
        TileAttributeMap<toc[4].size> tile_attrs4 @ (_base + toc[4].offset) [[name("Tile Attributes (500)")]];
    }}
    if (toc_count >= 6) { if (toc[5].asset_type == 500) {
        TileAttributeMap<toc[5].size> tile_attrs5 @ (_base + toc[5].offset) [[name("Tile Attributes (500)")]];
    }}
    if (toc_count >= 7) { if (toc[6].asset_type == 500) {
        TileAttributeMap<toc[6].size> tile_attrs6 @ (_base + toc[6].offset) [[name("Tile Attributes (500)")]];
    }}
    if (toc_count >= 8) { if (toc[7].asset_type == 500) {
        TileAttributeMap<toc[7].size> tile_attrs7 @ (_base + toc[7].offset) [[name("Tile Attributes (500)")]];
    }}
    
    // Search for Asset 501 (Entity Placement Data) - 24-byte entity structures
    // Entity types: 2=Clayball, 3=Ammo, 8=Item, 24=AmmoSpecial, 25/27=Enemy, 45=Message, etc.
    if (toc_count >= 1) { if (toc[0].asset_type == 501) {
        Entity entities0[toc[0].size / 24] @ (_base + toc[0].offset) [[name("Entities (501)")]];
    }}
    if (toc_count >= 2) { if (toc[1].asset_type == 501) {
        Entity entities1[toc[1].size / 24] @ (_base + toc[1].offset) [[name("Entities (501)")]];
    }}
    if (toc_count >= 3) { if (toc[2].asset_type == 501) {
        Entity entities2[toc[2].size / 24] @ (_base + toc[2].offset) [[name("Entities (501)")]];
    }}
    if (toc_count >= 4) { if (toc[3].asset_type == 501) {
        Entity entities3[toc[3].size / 24] @ (_base + toc[3].offset) [[name("Entities (501)")]];
    }}
    if (toc_count >= 5) { if (toc[4].asset_type == 501) {
        Entity entities4[toc[4].size / 24] @ (_base + toc[4].offset) [[name("Entities (501)")]];
    }}
    if (toc_count >= 6) { if (toc[5].asset_type == 501) {
        Entity entities5[toc[5].size / 24] @ (_base + toc[5].offset) [[name("Entities (501)")]];
    }}
    if (toc_count >= 7) { if (toc[6].asset_type == 501) {
        Entity entities6[toc[6].size / 24] @ (_base + toc[6].offset) [[name("Entities (501)")]];
    }}
    if (toc_count >= 8) { if (toc[7].asset_type == 501) {
        Entity entities7[toc[7].size / 24] @ (_base + toc[7].offset) [[name("Entities (501)")]];
    }}
    
    // Search for Asset 504 (Vehicle Data) - only in FINN/RUNN levels
    // Asset 504 can appear at various TOC positions (e.g., index 8 in FINN)
    if (toc_count >= 1) { if (toc[0].asset_type == 504) {
        Asset504VehicleData<toc[0].size> vehicle0 @ (_base + toc[0].offset) [[name("Vehicle Data (504)")]];
    }}
    if (toc_count >= 2) { if (toc[1].asset_type == 504) {
        Asset504VehicleData<toc[1].size> vehicle1 @ (_base + toc[1].offset) [[name("Vehicle Data (504)")]];
    }}
    if (toc_count >= 3) { if (toc[2].asset_type == 504) {
        Asset504VehicleData<toc[2].size> vehicle2 @ (_base + toc[2].offset) [[name("Vehicle Data (504)")]];
    }}
    if (toc_count >= 4) { if (toc[3].asset_type == 504) {
        Asset504VehicleData<toc[3].size> vehicle3 @ (_base + toc[3].offset) [[name("Vehicle Data (504)")]];
    }}
    if (toc_count >= 5) { if (toc[4].asset_type == 504) {
        Asset504VehicleData<toc[4].size> vehicle4 @ (_base + toc[4].offset) [[name("Vehicle Data (504)")]];
    }}
    if (toc_count >= 6) { if (toc[5].asset_type == 504) {
        Asset504VehicleData<toc[5].size> vehicle5 @ (_base + toc[5].offset) [[name("Vehicle Data (504)")]];
    }}
    if (toc_count >= 7) { if (toc[6].asset_type == 504) {
        Asset504VehicleData<toc[6].size> vehicle6 @ (_base + toc[6].offset) [[name("Vehicle Data (504)")]];
    }}
    if (toc_count >= 8) { if (toc[7].asset_type == 504) {
        Asset504VehicleData<toc[7].size> vehicle7 @ (_base + toc[7].offset) [[name("Vehicle Data (504)")]];
    }}
    if (toc_count >= 9) { if (toc[8].asset_type == 504) {
        Asset504VehicleData<toc[8].size> vehicle8 @ (_base + toc[8].offset) [[name("Vehicle Data (504)")]];
    }}
    if (toc_count >= 10) { if (toc[9].asset_type == 504) {
        Asset504VehicleData<toc[9].size> vehicle9 @ (_base + toc[9].offset) [[name("Vehicle Data (504)")]];
    }}
    
    // Asset 700 - Additional Audio Data (under investigation)
    // Only in 9 of 26 levels: MENU, SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED
    if (toc_count >= 1) { if (toc[0].asset_type == 700) {
        Asset700Data<toc[0].size> audio700_0 @ (_base + toc[0].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 2) { if (toc[1].asset_type == 700) {
        Asset700Data<toc[1].size> audio700_1 @ (_base + toc[1].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 3) { if (toc[2].asset_type == 700) {
        Asset700Data<toc[2].size> audio700_2 @ (_base + toc[2].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 4) { if (toc[3].asset_type == 700) {
        Asset700Data<toc[3].size> audio700_3 @ (_base + toc[3].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 5) { if (toc[4].asset_type == 700) {
        Asset700Data<toc[4].size> audio700_4 @ (_base + toc[4].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 6) { if (toc[5].asset_type == 700) {
        Asset700Data<toc[5].size> audio700_5 @ (_base + toc[5].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 7) { if (toc[6].asset_type == 700) {
        Asset700Data<toc[6].size> audio700_6 @ (_base + toc[6].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 8) { if (toc[7].asset_type == 700) {
        Asset700Data<toc[7].size> audio700_7 @ (_base + toc[7].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 9) { if (toc[8].asset_type == 700) {
        Asset700Data<toc[8].size> audio700_8 @ (_base + toc[8].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 10) { if (toc[9].asset_type == 700) {
        Asset700Data<toc[9].size> audio700_9 @ (_base + toc[9].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 11) { if (toc[10].asset_type == 700) {
        Asset700Data<toc[10].size> audio700_10 @ (_base + toc[10].offset) [[name("Audio Data (700)")]];
    }}
    if (toc_count >= 12) { if (toc[11].asset_type == 700) {
        Asset700Data<toc[11].size> audio700_11 @ (_base + toc[11].offset) [[name("Audio Data (700)")]];
    }}
} [[name("Stage"), color("80FFCC")]];

// Simple tertiary stage - just TOC (for fallback)
struct TertiaryStage {
    // Remember base for offset calculations - export for extraction tools
    type::Hex<u128> _base = $ [[export, name("segment_offset")]];
    
    u32 toc_count [[name("Asset Count")]];
    SegmentTOCEntry toc[toc_count] [[name("Asset TOC")]];
} [[name("Stage"), color("80FFCC")]];

// -----------------------------------------------------------------------------
// Secondary Sub-block - for per-stage tile data
// -----------------------------------------------------------------------------
struct SecondarySubBlock {
    u32 toc_count [[name("Asset Count")]];
    SegmentTOCEntry toc[toc_count] [[name("Assets")]];
} [[name("Secondary Sub"), color("FFCC80")]];

// -----------------------------------------------------------------------------
// Complete Level Data - Primary + Secondary + all Tertiary stages
// -----------------------------------------------------------------------------
struct LevelData<auto level_idx> {
    // Set dynamic display name from level data using helper function
    std::core::set_display_name(this, std::format("{} (Lv{})", get_level_name(level_idx), level_idx));
    
    // Get the level entry data using helper functions
    u16 _pri_sector = get_level_sector_offset(level_idx);
    u16 _pri_count = get_level_sector_count(level_idx);
    u16 _stage_count = get_level_stage_count(level_idx);
    
    // Secondary sector offsets (one per stage)
    u16 _sec0_sector = get_level_sec_sector_off(level_idx, 0);
    u16 _sec0_count = get_level_sec_sector_cnt(level_idx, 0);
    u16 _sec1_sector = get_level_sec_sector_off(level_idx, 1);
    u16 _sec1_count = get_level_sec_sector_cnt(level_idx, 1);
    u16 _sec2_sector = get_level_sec_sector_off(level_idx, 2);
    u16 _sec2_count = get_level_sec_sector_cnt(level_idx, 2);
    u16 _sec3_sector = get_level_sec_sector_off(level_idx, 3);
    u16 _sec3_count = get_level_sec_sector_cnt(level_idx, 3);
    u16 _sec4_sector = get_level_sec_sector_off(level_idx, 4);
    u16 _sec4_count = get_level_sec_sector_cnt(level_idx, 4);
    u16 _sec5_sector = get_level_sec_sector_off(level_idx, 5);
    u16 _sec5_count = get_level_sec_sector_cnt(level_idx, 5);
    
    // Primary Segment
    if (_pri_count > 0) {
        PrimarySegmentFull primary @ (_pri_sector * SECTOR_SIZE) [[name("Primary")]];
    }
    
    // Secondary Segments (one per stage - contains shared tile/palette data)
    if (_sec0_count > 0 && _sec0_sector > 0) {
        SecondarySegmentFull secondary @ (_sec0_sector * SECTOR_SIZE) [[name("Secondary (Stage 0)")]];
    }
    if (_sec1_count > 0 && _sec1_sector > 0) {
        SecondarySegmentFull secondary1 @ (_sec1_sector * SECTOR_SIZE) [[name("Secondary (Stage 1)")]];
    }
    if (_sec2_count > 0 && _sec2_sector > 0) {
        SecondarySegmentFull secondary2 @ (_sec2_sector * SECTOR_SIZE) [[name("Secondary (Stage 2)")]];
    }
    if (_sec3_count > 0 && _sec3_sector > 0) {
        SecondarySegmentFull secondary3 @ (_sec3_sector * SECTOR_SIZE) [[name("Secondary (Stage 3)")]];
    }
    if (_sec4_count > 0 && _sec4_sector > 0) {
        SecondarySegmentFull secondary4 @ (_sec4_sector * SECTOR_SIZE) [[name("Secondary (Stage 4)")]];
    }
    if (_sec5_count > 0 && _sec5_sector > 0) {
        SecondarySegmentFull secondary5 @ (_sec5_sector * SECTOR_SIZE) [[name("Secondary (Stage 5)")]];
    }
    
    // Tertiary Stages (up to 6) - with full asset parsing
    u16 _tert0 = get_level_tert_sector_off(level_idx, 0);
    u16 _tert1 = get_level_tert_sector_off(level_idx, 1);
    u16 _tert2 = get_level_tert_sector_off(level_idx, 2);
    u16 _tert3 = get_level_tert_sector_off(level_idx, 3);
    u16 _tert4 = get_level_tert_sector_off(level_idx, 4);
    u16 _tert5 = get_level_tert_sector_off(level_idx, 5);
    
    if (_stage_count >= 1 && _tert0 > 0) {
        TertiaryStageWithAssets stage0 @ (_tert0 * SECTOR_SIZE) [[name("Stage 0")]];
    }
    if (_stage_count >= 2 && _tert1 > 0) {
        TertiaryStageWithAssets stage1 @ (_tert1 * SECTOR_SIZE) [[name("Stage 1")]];
    }
    if (_stage_count >= 3 && _tert2 > 0) {
        TertiaryStageWithAssets stage2 @ (_tert2 * SECTOR_SIZE) [[name("Stage 2")]];
    }
    if (_stage_count >= 4 && _tert3 > 0) {
        TertiaryStageWithAssets stage3 @ (_tert3 * SECTOR_SIZE) [[name("Stage 3")]];
    }
    if (_stage_count >= 5 && _tert4 > 0) {
        TertiaryStageWithAssets stage4 @ (_tert4 * SECTOR_SIZE) [[name("Stage 4")]];
    }
    if (_stage_count >= 6 && _tert5 > 0) {
        TertiaryStageWithAssets stage5 @ (_tert5 * SECTOR_SIZE) [[name("Stage 5")]];
    }
} [[color("40CCFF")]];

// -----------------------------------------------------------------------------
// All Levels Container - uses g_level_count global
// -----------------------------------------------------------------------------
struct AllLevels {
    // Instantiate level data for each level in the header
    // Note: ImHex doesn't support true dynamic loops, so we use conditionals
    if (g_level_count > 0) LevelData<0> level_00;
    if (g_level_count > 1) LevelData<1> level_01;
    if (g_level_count > 2) LevelData<2> level_02;
    if (g_level_count > 3) LevelData<3> level_03;
    if (g_level_count > 4) LevelData<4> level_04;
    if (g_level_count > 5) LevelData<5> level_05;
    if (g_level_count > 6) LevelData<6> level_06;
    if (g_level_count > 7) LevelData<7> level_07;
    if (g_level_count > 8) LevelData<8> level_08;
    if (g_level_count > 9) LevelData<9> level_09;
    if (g_level_count > 10) LevelData<10> level_10;
    if (g_level_count > 11) LevelData<11> level_11;
    if (g_level_count > 12) LevelData<12> level_12;
    if (g_level_count > 13) LevelData<13> level_13;
    if (g_level_count > 14) LevelData<14> level_14;
    if (g_level_count > 15) LevelData<15> level_15;
    if (g_level_count > 16) LevelData<16> level_16;
    if (g_level_count > 17) LevelData<17> level_17;
    if (g_level_count > 18) LevelData<18> level_18;
    if (g_level_count > 19) LevelData<19> level_19;
    if (g_level_count > 20) LevelData<20> level_20;
    if (g_level_count > 21) LevelData<21> level_21;
    if (g_level_count > 22) LevelData<22> level_22;
    if (g_level_count > 23) LevelData<23> level_23;
    if (g_level_count > 24) LevelData<24> level_24;
    if (g_level_count > 25) LevelData<25> level_25;
};

// Instantiate the level explorer - placed after header for clean organization
// This is a "virtual" struct that only contains pointers to other locations
AllLevels levels @ HEADER_SIZE [[name("Levels"), comment("All level data dynamically parsed")]];

// =============================================================================
// PASSWORD SCREEN CONTAINER
// =============================================================================
// Password screen container structure - same as segment TOC
// Most have 11 assets, but Victory screen (#16) has 12
struct PasswordScreenContainer {
    u32 entry_count [[name("Count"), comment("Number of asset entries (11 for passwords, 12 for victory)")]];
    SegmentTOCEntry entries[entry_count] [[name("Assets")]];
} [[name("Password Screen"), color("CC66CC")]];

// =============================================================================
// USAGE TIPS
// =============================================================================
// 1. Browse Levels -> Level XX -> Stage N -> Tilemaps to see layer TOCs
// 2. Each layer's tile data is an array of u16 values (bits 0-10 = tile index)
// 3. Palettes are in Secondary -> Palettes -> Palette N (256 colors each)
// 4. Tile pixels are in Secondary -> Tile Pixels (256 bytes per 16x16 tile)
// 5. Sprites have embedded palettes at their palette_offset
// 6. To visualize a tile sheet:
//    - Find palette address from Secondary/Palettes
//    - Find tile pixel data address from Secondary/Tile Pixels
//    - Use the TileSheet16x16 template with those addresses
//
// =============================================================================
// PAL-SPECIFIC OFFSETS (for reference only - not instantiated)
// =============================================================================
// These offsets are valid for PAL version (SLES-01090) only.
// JP version has different sector offsets due to spliced intro movie.
//
// Loading Screens (BS frames before each level):
//   MENU: 0x00062800, PHRO: 0x0040C800, SCIE: 0x00643000, MONK: 0x009CA800
//   FINN: 0x00CC7000, MEGA: 0x00D6C800, BOIL: 0x00EF1000, SNOW: 0x0138E800
//   FOOD: 0x01777800, HEAD: 0x01997800, GLID: 0x01F13800, CAVE: 0x022D2800
//   WEED: 0x025E5800, EGGS: 0x028BA800, GLEN: 0x02C2C000, CLOU: 0x02D55000
//   SEVN: 0x03114800, SOAR: 0x03310800, CRYS: 0x035B3800, CSTL: 0x039B8800
//   WIZZ: 0x03D36800, RUNN: 0x03ECC800, MOSS: 0x04036000, KLOG: 0x043E0800
//   EVIL: 0x044E1000
//
// Password Screens (world completion):
//   #0: 0x0098F800, #1: 0x00EB7000, #2: 0x01355000, #3: 0x0173E800
//   #4: 0x01AFB800, #5: 0x01ED8800, #6: 0x02297800, #7: 0x025AB000
//   #8: 0x02880800, #9: 0x02D1B000, #10: 0x032D7000, #11: 0x0357A800
//   #12: 0x0397F800, #13: 0x03E93000, #14: 0x03FFC800, #15: 0x044AA800
//   #16 (Victory): 0x047DC800
//
// Boot Sequence:
//   Title Screen (Dreamworks): 0x6000 (sector 12)
//   Legal Screen: 0xB000 (sector 22)
//   Credits Container: 0xD000 (sectors 26-196)
//
// End Screens:
//   End Loading: 0x0481E800, Game Over: 0x04876000, Game Over Alt: 0x0487B000
//
// Unused Content:
//   Hidden Legal Screen: 0x1000 (sectors 2-11, unreferenced)
