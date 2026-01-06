// GAME.BLB Pattern File for Skullmonkeys PSX
// Skullmonkeys (PAL: SLES-01090, NTSC: SLUS-00601)
//
// The BLB file is the main game archive containing all levels, assets,
// and resources. The first 0x1000 bytes (4096) contain the header.
//
// =============================================================================
// FILE STRUCTURE OVERVIEW
// =============================================================================
// 
// FILE COVERAGE (SLES-01090 / 76,021,760 bytes / 72.50 MB):
//   - Header:           0x0000 - 0x8000  (32 KB)
//   - Level Data:       26 levels with primary/secondary/tertiary segments
//   - Sector Table:     32 entries at 0xCD0 (BS loading screens)
//   - Password Screens: 17 containers scattered between levels
//   - Coverage:         ~97% referenced, ~3% sector-alignment padding
//
// UNREFERENCED GAPS (~2.1 MB total):
//   These are sector-alignment padding between referenced regions.
//   Example gaps: 0x0000D000-0x00062800 (342 KB), ~90 KB gaps after each
//   password screen for sector alignment.
//
// =============================================================================
// SECTOR TABLE (32 ENTRIES AT OFFSET 0xCD0)
// =============================================================================
// The sector table maps to BS (bitstream/MDEC) images for loading screens.
// Each entry is 16 bytes: level_index(u8), flags(u8), code(u16), 
//                         short_name(4 chars), sector_offset(u32), sector_count(u32)
//
// Entry Mapping:
//   [0]     Title Screen         Sector 12    (0x6000)
//   [1]     Legal Screen         Sector 22    (0xB000)
//   [2-27]  Level Loading        sectors[i+2] → level[i] loading screen
//   [28]    End Loading          Sector 36925 (0x481E800)
//   [29]    Game Over            Sector 37100 (0x4876000)
//   [30]    Game Over (Alt)      Sector 37110 (0x487B000)
//   [31]    Password Container   Sector 4895  (0x98F800) - Contains password_screen_00
//
// =============================================================================
// PASSWORD SCREENS (17 CONTAINERS)
// =============================================================================
// Each password screen is a TOC container with 11 assets (except #16 with 12):
//   Asset 0: Tilemap (30×16 = 480 tiles → 320×256 rendering)
//   Assets 1-10: Various graphics (digits, letters, decorations)
//
// Password screens appear in gaps AFTER their corresponding level:
//   #0:  0x0098F800 (Unused? - inside SectorEntry[31])
//   #1:  0x00EB7000 (World 1)  ... #15: 0x044AA800 (World 15)
//   #16: 0x047DC800 (Victory "YOU WIN" - 12 assets)
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
// Known asset type IDs in BLB containers (matches blb.py AssetID class):
//
// SECONDARY SEGMENT - Tile Data:
//   100 (0x064) TILE_HEADER      - Tile counts and level dimensions (36 bytes)
//   101 (0x065) TILE_HEADER_101  - Unknown geometry (12 bytes, sparse)
//   200 (0x0C8) TILEMAP_CONTAINER - Tilemap sub-TOC with layer data
//   201 (0x0C9) LAYER_ENTRIES    - Layer definitions (92 bytes each)
//   300 (0x12C) TILE_PIXELS      - 8bpp indexed pixel data
//   301 (0x12D) PALETTE_INDICES  - Palette index per tile (1 byte each)
//   302 (0x12E) TILE_FLAGS       - Tile size/rendering flags (1 byte each)
//   303 (0x12F) ANIMATED_TILES   - Animated tile lookup table
//   400 (0x190) PALETTE_CONTAINER - Palette sub-TOC with 256-color CLUTs
//   401 (0x191) PALETTE_ANIM     - Palette animation data
//
// PRIMARY SEGMENT - Level Geometry:
//   600 (0x258) GEOMETRY         - Level geometry/RLE sprites container
//   601 (0x259) COLLISION        - Collision/layout data container
//   602 (0x25A) LEVEL_PALETTE    - Level palette (15-bit PSX colors)
//
// TERTIARY SEGMENT - Audio & Sprites:
//   500 (0x1F4) SPRITE_METADATA  - Sprite metadata
//   501 (0x1F5) SPRITE_RLE       - Sprite RLE pixel data
//   502 (0x1F6) AUDIO_502        - Audio configuration
//   503 (0x1F7) AUDIO_503        - Audio configuration
//   700 (0x2BC) SPU_SAMPLES      - SPU audio samples (ADPCM)
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

struct TileHeader {
    // Background and secondary colors (0x00-0x07)
    u8 bg_r [[name("BG R"), color("FF0000"), comment("Background red 0-255")]];
    u8 bg_g [[name("BG G"), color("00FF00"), comment("Background green")]];
    u8 bg_b [[name("BG B"), color("0000FF"), comment("Background blue")]];
    u8 pad_03 [[hidden]];
    u8 secondary_r [[name("Alt R"), color("FF4040"), comment("Secondary color red")]];
    u8 secondary_g [[name("Alt G"), color("40FF40"), comment("Secondary color green")]];
    u8 secondary_b [[name("Alt B"), color("4040FF"), comment("Secondary color blue")]];
    u8 pad_07 [[hidden]];
    
    // Level dimensions (0x08-0x0F)
    u16 level_width [[name("Level W"), comment("Level width in tiles")]];
    u16 level_height [[name("Level H"), comment("Level height in tiles")]];
    u16 unknown_0c [[hidden]];
    u16 unknown_0e [[hidden]];
    
    // Tile counts (0x10-0x15) - used by GetTotalTileCount
    // Total = count_16x16 + count_8x8_a + count_8x8_b
    u16 count_16x16 [[name("16x16 Tiles"), comment("Number of 16x16 pixel tiles"), color("80FF80")]];
    u16 count_8x8_a [[name("8x8 Tiles A"), comment("Primary 8x8 tile count"), color("80CCFF")]];
    u16 count_8x8_b [[name("8x8 Tiles B"), comment("Additional tile count (often 0)")]];
    
    // Unknown region (0x16-0x1B)
    u8 unknown_16[6] [[hidden]];
    
    // Field 0x1C (read by GetAsset100Field1C)
    u16 field_1c [[name("Field 1C"), comment("Unknown field read by GetAsset100Field1C")]];
    u8 remaining[6] [[hidden]];
} [[name("Tile Header"), color("80FF80"), static]];

// =============================================================================
// Layer Entry - Asset 201 (0x0C9) - 92 bytes
// =============================================================================
// Verified via Ghidra analysis and extraction testing.
// NOTE: Field offsets match blb.py LayerEntry class
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
    
    // Unknown region (0x0C-0x0F)
    u8 unknown_0c[4] [[hidden]];
    
    // Scroll factors (0x10-0x17) - 0x10000 = 1.0, 0x8000 = 0.5
    u32 scroll_x [[name("Scroll X"), format("format_parallax"), comment("X parallax (1.0 = camera, 0.5 = parallax)")]];
    u32 scroll_y [[name("Scroll Y"), format("format_parallax"), comment("Y parallax factor")]];
    
    // Unknown region (0x18-0x25)
    u8 unknown_18[14] [[hidden]];
    
    // Layer type (0x26)
    u8 layer_type [[name("Type"), comment("Layer type (0=normal, 2=foreground?)")]];
    
    // Unknown region (0x27-0x2B)
    u8 unknown_27[5] [[hidden]];
    
    // Background color (0x2C-0x2E) - used when layer has no tiles
    u8 bg_r [[name("BG R"), color("FF4040")]];
    u8 bg_g [[name("BG G"), color("40FF40")]];
    u8 bg_b [[name("BG B"), color("4040FF")]];
    
    // Padding to 92 bytes (0x2F-0x5B)
    // Note: Tile count = width × height
    u8 layer_padding[45] [[hidden]];
} [[name("Layer"), color("FFCC80"), static,
    hex::inline_visualize("color", bg_r, bg_g, bg_b, 255)]];

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
struct AnimationEntry {
    u32 animation_id [[name("Anim ID")]];
    u16 frame_count [[name("Frames")]];
    u16 frame_offset [[name("Frame Idx"), comment("Index into frame metadata array")]];
    u16 flags [[name("Flags")]];
    u16 extra [[hidden]];
} [[format("format_anim_entry"), color("CC80FF"), static]];

fn format_anim_entry(auto e) {
    return std::format("ID {} ({} frames)", e.animation_id, e.frame_count);
};

// Frame metadata (36 bytes = 0x24)
struct SpriteFrameMetadata {
    u16 reserved_00 [[hidden]];
    u16 reserved_02 [[hidden]];
    u16 flags [[name("Flags"), comment("1 or 2")]];
    s16 render_x [[name("X")]];
    s16 render_y [[name("Y")]];
    u16 render_width [[name("W")]];
    u16 render_height [[name("H")]];
    u16 unknown_0e [[hidden]];
    u16 reserved_10 [[hidden]];
    s16 anchor_x [[name("Anchor X")]];
    s16 anchor_y [[name("Anchor Y")]];
    u16 clip_width [[name("Clip W")]];
    u16 clip_height [[name("Clip H")]];
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
// Audio Assets - Assets 502/503 (0x1F6/0x1F7)
// =============================================================================

// Audio configuration structures
// Used by SPU/sound system - structure details TBD

// =============================================================================
// SPU Samples - Asset 700 (0x2BC)
// =============================================================================

// SPU audio samples in ADPCM format
// Raw compressed audio data for PlayStation sound chip

// =============================================================================
// Level Palette - Asset 602 (0x25A)
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
// Sector Table Entry (0x10 = 16 bytes) - Located at offset 0xCD0
// =============================================================================
// The sector table at 0xCD0 contains 32 entries that point to BS/MDEC frames:
//
// INDEX | PURPOSE              | DESCRIPTION
// ------|----------------------|------------------------------------------
//   0   | Title Screen         | Dreamworks/Skullmonkeys logo (sector 12)
//   1   | Legal Screen         | Copyright text (sector 22)
// 2-27  | Loading Screens      | sectors[i+2] → level[i] loading screen
//  28   | End Loading          | Loading screen for credits/end sequence
//  29   | Game Over 1          | Game Over screen
//  30   | Game Over 2          | Game Over screen (alternate)
//  31   | Password Screen      | Mystery password container reference
//
// The BS frame is located at: sectors[i].sector_offset * 2048
// For level loading screens: level[i].sector_offset - sectors[i+2].sector_offset ≈ 9 sectors

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
    type::Hex<u16> sector_offset [[name("Sector"), comment("Sector offset in BLB")]];
    u16 sector_count [[name("Count"), comment("Number of sectors")]];
} [[name("Sector Entry"), format("format_sector_offset"), color("80CCCC"), static]];

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
//   6 = Mode6Sector - Load data from mode6_sectors table at 0xECC (17 entries)
//                     Accessed as: header + (level_index * 4) + 0xECC
enum PlaybackMode : u8 {
    Invalid = 0,
    Movie = 1,
    Credits = 2,
    Level = 3,
    SpecialSector4 = 4,
    SpecialSector5 = 5,
    Mode6Sector = 6
};

// =============================================================================
// Mode 6 Sector Entry (4 bytes)
// =============================================================================

// Mode 6 sector entries store CD sector location for special playback sequences.
// Table starts at 0xECC with 17 entries (68 bytes).
// NOTE: Entry[0] overlaps with sectors[31].sector_offset/sector_count!
// Game code accesses: header + (index * 4) + 0xECC
//
// These entries point to level asset data used during mode 6 playback.
// Each entry has ~110-132 sectors of data (225-270 KB).

struct Mode6SectorEntry {
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
    //   mode=6: index into mode6_sectors[17] at 0xECC (0-16)
    u8 indices[110] [[name("Indices"), comment("Table index for each step"), color("80FF80")]];
} [[name("Playback Sequence"), color("CC80CC")]];

// =============================================================================
// BLB Header (0x1000 = 4096 bytes)
// =============================================================================

struct BLBHeader {
    LevelEntry levels[26] [[name("Levels"), comment("Level metadata (26 entries)"), color("40CCFF")]];
    MovieEntry movies[13] [[name("Movies"), comment("Movie table (13 entries)"), color("FFFF40")]];
    u8 movie_sector_pad[4] [[hidden]];  // 4-byte gap between movies (0xCCC) and sectors (0xCD0)
    SectorEntry sectors[32] [[name("Sectors"), comment("Sector table (32 entries)"), color("80CCCC")]];
    
    // Mode 6 sector table: 16 additional entries at 0xED0-0xF0F (64 bytes)
    // NOTE: Entry[0] is at 0xECC, overlapping with sectors[31].sector_offset/count
    // The game accesses all 17 entries via: header + (index * 4) + 0xECC
    // These point to level asset data for mode 6 playback sequences
    Mode6SectorEntry mode6_sectors[16] [[name("Mode6 Sectors"), 
        comment("Extra mode 6 entries (entry 0 is in sectors[31])"), color("CC80FF")]];
    
    CreditsEntry credits[2] [[name("Credits"), comment("Credits entries (only 2 complete)"), color("FFCC80")]];
    u8 gap_to_counts[9] [[hidden]];  // Gap from credits end (0xF28) to counts (0xF31)
    
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

// =============================================================================
// LEVEL DATA STRUCTURES
// =============================================================================
// The following structures parse the actual level content stored at sector offsets.
// Each level has Primary, Secondary, and Tertiary data segments.

// -----------------------------------------------------------------------------
// Segment TOC Entry - Alias of generic TOCEntry for segments
// -----------------------------------------------------------------------------
// For segments: id = asset_type (100, 200, 300, 400, 600, etc.)

struct SegmentTOCEntry {
    u32 asset_type [[name("Type"), comment("Asset type ID"), color("FFCC40")]];
    u32 size [[name("Size"), comment("Asset size in bytes")]];
    type::Hex<u32> offset [[name("Offset"), comment("Offset from segment start")]];
} [[static]];

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
// COLLISION CONTAINER - Asset 601 (0x259) 
// =============================================================================

struct CollisionContainerAsset {
    u32 entry_count [[name("Count"), comment("Number of collision entries")]];
    SubTOCEntry collision_toc[entry_count] [[name("Collision TOC")]];
} [[name("Collision Container"), color("8080FF")]];

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
    // Remember base for offset calculations
    u128 _base = $;
    
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
    
    // Parse Asset 601 (Collision Container)
    if (toc_count > 0 && toc[0].asset_type == 601) {
        CollisionContainerAsset collision0 @ (_base + toc[0].offset) [[name("Collision")]];
    }
    if (toc_count > 1 && toc[1].asset_type == 601) {
        CollisionContainerAsset collision1 @ (_base + toc[1].offset) [[name("Collision")]];
    }
    if (toc_count > 2 && toc[2].asset_type == 601) {
        CollisionContainerAsset collision2 @ (_base + toc[2].offset) [[name("Collision")]];
    }
} [[name("Primary Segment"), color("40CCFF")]];

// -----------------------------------------------------------------------------
// Secondary Segment - Full Parse with Tiles
// -----------------------------------------------------------------------------
struct SecondarySegmentFull {
    // Remember base for offset calculations
    u128 _base = $;
    
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
    // Remember our base address
    u128 _base = $;
    
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
} [[name("Stage"), color("80FFCC")]];

// Simple tertiary stage - just TOC (for fallback)
struct TertiaryStage {
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
    // Set dynamic display name from level data
    std::core::set_display_name(this, std::format("{} (Lv{})", header.levels[level_idx].name, level_idx));
    
    // Get the level entry from header
    u16 _pri_sector = header.levels[level_idx].sector_offset;
    u16 _pri_count = header.levels[level_idx].sector_count;
    u16 _sec_sector = header.levels[level_idx].sec_sector_off[0];
    u16 _sec_count = header.levels[level_idx].sec_sector_cnt[0];
    u16 _stage_count = header.levels[level_idx].stage_count;
    
    // Primary Segment
    if (_pri_count > 0) {
        PrimarySegmentFull primary @ (_pri_sector * SECTOR_SIZE) [[name("Primary")]];
    }
    
    // Secondary Segment (base)
    if (_sec_count > 0) {
        SecondarySegmentFull secondary @ (_sec_sector * SECTOR_SIZE) [[name("Secondary")]];
    }
    
    // Tertiary Stages (up to 6) - with full asset parsing
    if (_stage_count >= 1 && header.levels[level_idx].tert_sector_off[0] > 0) {
        TertiaryStageWithAssets stage0 @ (header.levels[level_idx].tert_sector_off[0] * SECTOR_SIZE) [[name("Stage 0")]];
    }
    if (_stage_count >= 2 && header.levels[level_idx].tert_sector_off[1] > 0) {
        TertiaryStageWithAssets stage1 @ (header.levels[level_idx].tert_sector_off[1] * SECTOR_SIZE) [[name("Stage 1")]];
    }
    if (_stage_count >= 3 && header.levels[level_idx].tert_sector_off[2] > 0) {
        TertiaryStageWithAssets stage2 @ (header.levels[level_idx].tert_sector_off[2] * SECTOR_SIZE) [[name("Stage 2")]];
    }
    if (_stage_count >= 4 && header.levels[level_idx].tert_sector_off[3] > 0) {
        TertiaryStageWithAssets stage3 @ (header.levels[level_idx].tert_sector_off[3] * SECTOR_SIZE) [[name("Stage 3")]];
    }
    if (_stage_count >= 5 && header.levels[level_idx].tert_sector_off[4] > 0) {
        TertiaryStageWithAssets stage4 @ (header.levels[level_idx].tert_sector_off[4] * SECTOR_SIZE) [[name("Stage 4")]];
    }
    if (_stage_count >= 6 && header.levels[level_idx].tert_sector_off[5] > 0) {
        TertiaryStageWithAssets stage5 @ (header.levels[level_idx].tert_sector_off[5] * SECTOR_SIZE) [[name("Stage 5")]];
    }
} [[color("40CCFF")]];

// -----------------------------------------------------------------------------
// All Levels Container - uses level_count from header
// -----------------------------------------------------------------------------
struct AllLevels {
    // Instantiate level data for each level in the header
    // Note: ImHex doesn't support true dynamic loops, so we use conditionals
    if (header.level_count > 0) LevelData<0> level_00;
    if (header.level_count > 1) LevelData<1> level_01;
    if (header.level_count > 2) LevelData<2> level_02;
    if (header.level_count > 3) LevelData<3> level_03;
    if (header.level_count > 4) LevelData<4> level_04;
    if (header.level_count > 5) LevelData<5> level_05;
    if (header.level_count > 6) LevelData<6> level_06;
    if (header.level_count > 7) LevelData<7> level_07;
    if (header.level_count > 8) LevelData<8> level_08;
    if (header.level_count > 9) LevelData<9> level_09;
    if (header.level_count > 10) LevelData<10> level_10;
    if (header.level_count > 11) LevelData<11> level_11;
    if (header.level_count > 12) LevelData<12> level_12;
    if (header.level_count > 13) LevelData<13> level_13;
    if (header.level_count > 14) LevelData<14> level_14;
    if (header.level_count > 15) LevelData<15> level_15;
    if (header.level_count > 16) LevelData<16> level_16;
    if (header.level_count > 17) LevelData<17> level_17;
    if (header.level_count > 18) LevelData<18> level_18;
    if (header.level_count > 19) LevelData<19> level_19;
    if (header.level_count > 20) LevelData<20> level_20;
    if (header.level_count > 21) LevelData<21> level_21;
    if (header.level_count > 22) LevelData<22> level_22;
    if (header.level_count > 23) LevelData<23> level_23;
    if (header.level_count > 24) LevelData<24> level_24;
    if (header.level_count > 25) LevelData<25> level_25;
};

// Instantiate the level explorer - placed after header for clean organization
// This is a "virtual" struct that only contains pointers to other locations
AllLevels levels @ HEADER_SIZE [[name("Levels"), comment("All level data dynamically parsed")]];

// =============================================================================
// HIDDEN/UNUSED CONTENT - Unreferenced BS Frames
// =============================================================================
// These BS/MDEC compressed images exist in the BLB file but are NOT referenced
// by any game code. They represent cut or replaced content from development.
//
// Discovered via sector coverage analysis (2026-01-06).
// Total unreferenced: 704 KB (0.9% of file)
//
// To decode these images:
//   nix-shell -p jpsxdec --run "jpsxdec -f <extracted.bs> -static bs -dim 320x256 -fmt png"

// --- Unused Legal/Copyright Screen (Sectors 2-11, offset 0x1000) ---
// An alternate legal screen that was replaced during development.
// The game uses the LEGL asset at sector 22 instead.
BSFrameHeader hidden_legal @ 0x1000 [[name("⚠️ UNUSED: Legal Screen"), 
    comment("Unreferenced BS frame - alternate legal/copyright screen (replaced)"),
    color("FF4040")]];

// --- Unused Credits Slideshow (Sectors 26-196, starting at 0xD000) ---
// A complete 10-frame credits sequence that was CUT from the final game!
// The container has a header at 0xD000, frames start at 0xD0F4.
//
// Frame offsets (from start of file):
//   Frame 0: 0xD0F4  (25152 bytes, quant=4)
//   Frame 1: 0x15DB8 (23296 bytes, quant=4)  
//   Frame 2: 0x1EEF0 (23488 bytes, quant=2)
//   Frame 3: 0x281FC (21504 bytes, quant=3)
//   Frame 4: 0x30764 (26112 bytes, quant=3)
//   Frame 5: 0x39964 (24832 bytes, quant=4)
//   Frame 6: 0x429A0 (21248 bytes, quant=3)
//   Frame 7: 0x4A9CC (24576 bytes, quant=2)
//   Frame 8: 0x52F78 (20928 bytes, quant=3)
//   Frame 9: 0x5AF44 (22080 bytes, quant=2)

BSFrameHeader hidden_credits_0 @ 0xD0F4 [[name("⚠️ UNUSED: Credits Frame 0"), 
    comment("Unreferenced BS frame - first slide of cut credits sequence"),
    color("FF8040")]];
BSFrameHeader hidden_credits_1 @ 0x15DB8 [[name("⚠️ UNUSED: Credits Frame 1"), 
    comment("Unreferenced BS frame - credits slide 2"),
    color("FF8040")]];
BSFrameHeader hidden_credits_2 @ 0x1EEF0 [[name("⚠️ UNUSED: Credits Frame 2"), 
    comment("Unreferenced BS frame - credits slide 3"),
    color("FF8040")]];
BSFrameHeader hidden_credits_3 @ 0x281FC [[name("⚠️ UNUSED: Credits Frame 3"), 
    comment("Unreferenced BS frame - credits slide 4"),
    color("FF8040")]];
BSFrameHeader hidden_credits_4 @ 0x30764 [[name("⚠️ UNUSED: Credits Frame 4"), 
    comment("Unreferenced BS frame - credits slide 5"),
    color("FF8040")]];
BSFrameHeader hidden_credits_5 @ 0x39964 [[name("⚠️ UNUSED: Credits Frame 5"), 
    comment("Unreferenced BS frame - credits slide 6"),
    color("FF8040")]];
BSFrameHeader hidden_credits_6 @ 0x429A0 [[name("⚠️ UNUSED: Credits Frame 6"), 
    comment("Unreferenced BS frame - credits slide 7"),
    color("FF8040")]];
BSFrameHeader hidden_credits_7 @ 0x4A9CC [[name("⚠️ UNUSED: Credits Frame 7"), 
    comment("Unreferenced BS frame - credits slide 8"),
    color("FF8040")]];
BSFrameHeader hidden_credits_8 @ 0x52F78 [[name("⚠️ UNUSED: Credits Frame 8"), 
    comment("Unreferenced BS frame - credits slide 9"),
    color("FF8040")]];
BSFrameHeader hidden_credits_9 @ 0x5AF44 [[name("⚠️ UNUSED: Credits Frame 9"), 
    comment("Unreferenced BS frame - final credits slide"),
    color("FF8040")]];

// --- Duplicate of Credits (Sectors 36929-37099, offset 0x4820800) ---
// Byte-for-byte identical to the credits slideshow above.
// This is likely a build artifact from the disc mastering process.
BSFrameHeader hidden_credits_duplicate @ 0x48208F4 [[name("⚠️ DUPLICATE: Credits Frame 0 (copy)"), 
    comment("Duplicate of credits - build artifact at end of file"),
    color("808080")]];

// =============================================================================
// LEVEL LOADING SCREENS (VERIFIED 2026-01-06)
// =============================================================================
// Each level has a BS/MDEC loading screen referenced via the sector table.
//
// SECTOR TABLE MAPPING:
//   header.sectors[0]  → Title Screen (Dreamworks logo)
//   header.sectors[1]  → Legal Screen (copyright text)
//   header.sectors[i+2] → Level[i] loading screen (for i = 0..25)
//   header.sectors[28] → End sequence loading screen
//   header.sectors[29] → Game Over screen
//   header.sectors[30] → Game Over screen (alternate)
//   header.sectors[31] → Mystery password screen reference
//
// BS Frame Location: header.sectors[n].sector_offset * 2048
// Delta: level[i].sector_offset - sectors[i+2].sector_offset ≈ 9 sectors
//
// The individual instances below are generated from the sector table for
// quick navigation in ImHex.

BSFrameHeader loading_MENU @ 0x00062800 [[name("🔄 Loading: MENU"),
    comment("Loading screen before MENU (sector 197, level at 201)"),
    color("4080FF")]];
BSFrameHeader loading_PHRO @ 0x0040C800 [[name("🔄 Loading: PHRO (Skullmonkey Gate)"),
    comment("Loading screen before PHRO (sector 2073, level at 2083)"),
    color("4080FF")]];
BSFrameHeader loading_SCIE @ 0x00643000 [[name("🔄 Loading: SCIE (Science Center)"),
    comment("Loading screen before SCIE (sector 3206, level at 3215)"),
    color("4080FF")]];
BSFrameHeader loading_MONK @ 0x009CA800 [[name("🔄 Loading: MONK (Monkey Shrines)"),
    comment("Loading screen before MONK (sector 5013, level at 5023)"),
    color("4080FF")]];
BSFrameHeader loading_FINN @ 0x00CC7000 [[name("🔄 Loading: FINN (Swimming)"),
    comment("Loading screen before FINN (sector 6542, level at 6551)"),
    color("4080FF")]];
BSFrameHeader loading_MEGA @ 0x00D6C800 [[name("🔄 Loading: MEGA (Shriney Guard Boss)"),
    comment("Loading screen before MEGA (sector 6873, level at 6882)"),
    color("4080FF")]];
BSFrameHeader loading_BOIL @ 0x00EF1000 [[name("🔄 Loading: BOIL (Boilerroom)"),
    comment("Loading screen before BOIL (sector 7650, level at 7660)"),
    color("4080FF")]];
BSFrameHeader loading_SNOW @ 0x0138E800 [[name("🔄 Loading: SNOW"),
    comment("Loading screen before SNOW (sector 10013, level at 10022)"),
    color("4080FF")]];
BSFrameHeader loading_FOOD @ 0x01777800 [[name("🔄 Loading: FOOD"),
    comment("Loading screen before FOOD (sector 12015, level at 12024)"),
    color("4080FF")]];
BSFrameHeader loading_HEAD @ 0x01997800 [[name("🔄 Loading: HEAD (Joe-Head-Joe Boss)"),
    comment("Loading screen before HEAD (sector 13103, level at 13113)"),
    color("4080FF")]];
BSFrameHeader loading_BRG1 @ 0x01B35000 [[name("🔄 Loading: BRG1 (Bridge)"),
    comment("Loading screen before BRG1 (sector 13930, level at 13940)"),
    color("4080FF")]];
BSFrameHeader loading_GLID @ 0x01F13800 [[name("🔄 Loading: GLID (Gliding)"),
    comment("Loading screen before GLID (sector 15911, level at 15920)"),
    color("4080FF")]];
BSFrameHeader loading_CAVE @ 0x022D2800 [[name("🔄 Loading: CAVE"),
    comment("Loading screen before CAVE (sector 17829, level at 17838)"),
    color("4080FF")]];
BSFrameHeader loading_WEED @ 0x025E5800 [[name("🔄 Loading: WEED"),
    comment("Loading screen before WEED (sector 19403, level at 19413)"),
    color("4080FF")]];
BSFrameHeader loading_EGGS @ 0x028BA800 [[name("🔄 Loading: EGGS"),
    comment("Loading screen before EGGS (sector 20853, level at 20863)"),
    color("4080FF")]];
BSFrameHeader loading_GLEN @ 0x02C2C000 [[name("🔄 Loading: GLEN (Glenn Yntis Boss)"),
    comment("Loading screen before GLEN (sector 22616, level at 22625)"),
    color("4080FF")]];
BSFrameHeader loading_CLOU @ 0x02D55000 [[name("🔄 Loading: CLOU (Clouds)"),
    comment("Loading screen before CLOU (sector 23210, level at 23220)"),
    color("4080FF")]];
BSFrameHeader loading_SEVN @ 0x03114800 [[name("🔄 Loading: SEVN (1970s Bonus)"),
    comment("Loading screen before SEVN (sector 25129, level at 25139)"),
    color("4080FF")]];
BSFrameHeader loading_SOAR @ 0x03310800 [[name("🔄 Loading: SOAR (Soaring)"),
    comment("Loading screen before SOAR (sector 26145, level at 26154)"),
    color("4080FF")]];
BSFrameHeader loading_CRYS @ 0x035B3800 [[name("🔄 Loading: CRYS (Crystal)"),
    comment("Loading screen before CRYS (sector 27495, level at 27504)"),
    color("4080FF")]];
BSFrameHeader loading_CSTL @ 0x039B8800 [[name("🔄 Loading: CSTL (Castle)"),
    comment("Loading screen before CSTL (sector 29553, level at 29562)"),
    color("4080FF")]];
BSFrameHeader loading_WIZZ @ 0x03D36800 [[name("🔄 Loading: WIZZ (Monkey Mage Boss)"),
    comment("Loading screen before WIZZ (sector 31341, level at 31350)"),
    color("4080FF")]];
BSFrameHeader loading_RUNN @ 0x03ECC800 [[name("🔄 Loading: RUNN (Runner)"),
    comment("Loading screen before RUNN (sector 32153, level at 32163)"),
    color("4080FF")]];
BSFrameHeader loading_MOSS @ 0x04036000 [[name("🔄 Loading: MOSS"),
    comment("Loading screen before MOSS (sector 32876, level at 32886)"),
    color("4080FF")]];
BSFrameHeader loading_KLOG @ 0x043E0800 [[name("🔄 Loading: KLOG (Klogg Boss)"),
    comment("Loading screen before KLOG (sector 34753, level at 34763)"),
    color("4080FF")]];
BSFrameHeader loading_EVIL @ 0x044E1000 [[name("🔄 Loading: EVIL (Evil Engine)"),
    comment("Loading screen before EVIL (sector 35266, level at 35275)"),
    color("4080FF")]];

// =============================================================================
// WORLD COMPLETION PASSWORD SCREENS (VERIFIED 2026-01-06)
// =============================================================================
// These 17 containers are WORLD COMPLETION PASSWORD SCREENS.
// Skullmonkeys has NO memory card support, so after completing each world,
// players are shown a password screen to return to that checkpoint.
//
// Container Format (11 assets each):
//   Asset 100: Tile Header (36 bytes) - 30×16 level dimensions
//   Asset 200: Tilemap Container (5 tilemaps)
//   Asset 201: Layer Entries (5 layers)
//   Asset 300: Tile Pixels (8bpp, ~540 tiles)
//   Asset 301: Palette Indices
//   Asset 302: Tile Size Flags
//   Asset 400: Palette Container (4 × 256-color)
//   Asset 401: Animation Config
//   Asset 600: Tile Graphics (compressed)
//   Asset 601: Collision Data
//   Asset 602: Raw Palette (16-color CLUT)
//
// Rendering: Use tilemap 3 (320 tiles = 20×16 grid) → 320×256 pixels
// Background colors:
//   - Passwords 1-2: Gray RGB(122-130, 122-130, 142)
//   - Passwords 3-16: Magenta RGB(150, 0, 106) = #96006A

// Password screen container structure - same as segment TOC
// Most have 11 assets, but Victory screen (#16) has 12
struct PasswordScreenContainer {
    u32 entry_count [[name("Count"), comment("Number of asset entries (11 for passwords, 12 for victory)")]];
    SegmentTOCEntry entries[entry_count] [[name("Assets")]];
} [[name("Password Screen"), color("CC66CC")]];

// Password #0 - Mystery Container (After TMPL level)
// This container was originally at 0x0098F800 and was discovered during coverage analysis.
// It has the same structure as other password screens (11 assets, 30×16 tiles).
PasswordScreenContainer password_screen_00 @ 0x0098F800 [[name("🔐 Password Screen #0 - (Unused?)"),
    comment("Mystery container after TMPL - same format as password screens, Magenta theme"),
    color("96006A")]];

// Password #1 - World 1 Complete (Gray theme)
PasswordScreenContainer password_screen_01 @ 0x00EB7000 [[name("🔐 Password Screen #1 - World 1"),
    comment("World 1 completion - Gray theme RGB(122,122,142)"),
    color("7A7A8E")]];

// Password #2 - World 2 Complete (Gray theme)
PasswordScreenContainer password_screen_02 @ 0x01355000 [[name("🔐 Password Screen #2 - World 2"),
    comment("World 2 completion - Gray theme RGB(130,130,142)"),
    color("82828E")]];

// Password #3 - World 3 Complete (Magenta theme)
PasswordScreenContainer password_screen_03 @ 0x0173E800 [[name("🔐 Password Screen #3 - World 3"),
    comment("World 3 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #4 - World 4 Complete (Magenta theme)
PasswordScreenContainer password_screen_04 @ 0x01AFB800 [[name("🔐 Password Screen #4 - World 4"),
    comment("World 4 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #5 - World 5 Complete (Magenta theme)
PasswordScreenContainer password_screen_05 @ 0x01ED8800 [[name("🔐 Password Screen #5 - World 5"),
    comment("World 5 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #6 - World 6 Complete (Magenta theme)
PasswordScreenContainer password_screen_06 @ 0x02297800 [[name("🔐 Password Screen #6 - World 6"),
    comment("World 6 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #7 - World 7 Complete (Magenta theme)
PasswordScreenContainer password_screen_07 @ 0x025AB000 [[name("🔐 Password Screen #7 - World 7"),
    comment("World 7 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #8 - World 8 Complete (Magenta theme)
PasswordScreenContainer password_screen_08 @ 0x02880800 [[name("🔐 Password Screen #8 - World 8"),
    comment("World 8 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #9 - World 9 Complete (Magenta theme)
PasswordScreenContainer password_screen_09 @ 0x02D1B000 [[name("🔐 Password Screen #9 - World 9"),
    comment("World 9 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #10 - World 10 Complete (Magenta theme)
PasswordScreenContainer password_screen_10 @ 0x032D7000 [[name("🔐 Password Screen #10 - World 10"),
    comment("World 10 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #11 - World 11 Complete (Magenta theme)
PasswordScreenContainer password_screen_11 @ 0x0357A800 [[name("🔐 Password Screen #11 - World 11"),
    comment("World 11 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #12 - World 12 Complete (Magenta theme)
PasswordScreenContainer password_screen_12 @ 0x0397F800 [[name("🔐 Password Screen #12 - World 12"),
    comment("World 12 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #13 - World 13 Complete (Magenta theme)
PasswordScreenContainer password_screen_13 @ 0x03E93000 [[name("🔐 Password Screen #13 - World 13"),
    comment("World 13 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #14 - World 14 Complete (Magenta theme)
PasswordScreenContainer password_screen_14 @ 0x03FFC800 [[name("🔐 Password Screen #14 - World 14"),
    comment("World 14 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #15 - World 15 Complete (Magenta theme)
PasswordScreenContainer password_screen_15 @ 0x044AA800 [[name("🔐 Password Screen #15 - World 15"),
    comment("World 15 completion - Magenta theme #96006A"),
    color("96006A")]];

// Password #16 - "YOU WIN" Final Victory Screen (Magenta theme)
PasswordScreenContainer password_screen_16 @ 0x047DC800 [[name("🏆 YOU WIN - Victory Screen"),
    comment("Final victory screen - Largest container (675 tiles) - Magenta theme #96006A"),
    color("FFD700")]];

// =============================================================================
// TITLE AND LEGAL SCREENS (BOOT SEQUENCE)
// =============================================================================
// BOOT SEQUENCE SCREENS (SECTOR TABLE ENTRIES 0-1)
// =============================================================================
// These BS frames display during game boot before any gameplay.
// Referenced via: header.sectors[0] and header.sectors[1]

// Sector 12 - Skullmonkeys title card with Dreamworks copyright
// Access: header.sectors[0].sector_offset * 2048 = 0x6000
BSFrameHeader title_screen @ 0x6000 [[name("🎮 Title Screen"),
    comment("Skullmonkeys title card with Dreamworks copyright - sectors[0]"),
    color("FFD700")]];

// Sector 22 - Legal/copyright information screen  
// Access: header.sectors[1].sector_offset * 2048 = 0xB000
BSFrameHeader legal_screen @ 0xB000 [[name("📜 Legal/Copyright Screen"),
    comment("Legal/copyright text screen - sectors[1]"),
    color("40FF40")]];

// =============================================================================
// GAME OVER AND END SCREENS (SECTOR TABLE ENTRIES 28-30)
// =============================================================================
// These BS frames are referenced in the sector table at indices 28-30.
// Access: header.sectors[28..30].sector_offset * 2048

// Sector 36925 - End sequence loading screen
// Access: header.sectors[28].sector_offset * 2048 = 0x481E800
BSFrameHeader end_loading_screen @ 0x0481E800 [[name("🔄 End Loading Screen"),
    comment("End sequence loading screen - sectors[28]"),
    color("4080FF")]];

// Sector 37100 - Game Over screen
// Access: header.sectors[29].sector_offset * 2048 = 0x4876000
BSFrameHeader game_over_screen_1 @ 0x04876000 [[name("💀 Game Over Screen"),
    comment("Game Over screen - sectors[29]"),
    color("FF4040")]];

// Sector 37110 - Game Over screen (alternate/duplicate)
// Access: header.sectors[30].sector_offset * 2048 = 0x487B000
BSFrameHeader game_over_screen_2 @ 0x0487B000 [[name("💀 Game Over Screen (Alt)"),
    comment("Game Over screen alternate - sectors[30]"),
    color("FF4040")]];

// =============================================================================
// MYSTERY PASSWORD SCREEN (SECTOR TABLE ENTRY 31)
// =============================================================================
// Sector table entry 31 points to sector 4895 (0x98F800), which is a password
// screen container. This is the same as password_screen_00 defined above.
// Access: header.sectors[31].sector_offset * 2048 = 0x98F800

// =============================================================================
// ASSET TYPE REFERENCE
// =============================================================================
// Decimal (Hex) | Name              | Size/Structure       | Segment
// ------------- | ----------------- | -------------------- | --------
//  100 (0x064)  | Tile Header       | 36 bytes             | Secondary
//  200 (0x0C8)  | Tilemap Container | Sub-TOC + layers     | Tertiary  
//  201 (0x0C9)  | Layer Entries     | 92 bytes each        | Tertiary
//  300 (0x12C)  | Tile Pixels       | Variable (indexed)   | Secondary
//  301 (0x12D)  | Palette Indices   | Variable             | Secondary
//  302 (0x12E)  | Tile Flags        | 1 byte per tile      | Secondary
//  400 (0x190)  | Palette Container | Sub-TOC + CLUTs      | Secondary
//  500 (0x1F4)  | Audio Data        | Variable             | Tertiary
//  501 (0x1F5)  | Audio Sequence    | Variable             | Tertiary
//  502 (0x1F6)  | Audio Bank        | Variable             | Tertiary
//  503 (0x1F7)  | Audio Instrument  | Variable             | Tertiary
//  600 (0x258)  | Sprite Container  | Sub-TOC + sprites    | Tertiary
//  601 (0x259)  | Collision Data    | Sub-TOC + data       | Primary
//  602 (0x25A)  | Warp/Enemy Data   | Variable             | Primary
//  605 (0x25D)  | Special Objects   | Variable             | Tertiary
//
// Sub-TOC Format (used by Container assets):
//   u32 count
//   Entry[count]: { u32 id, u32 size, u32 offset }
//
// Sprite Data Format (at container_offset + entry.offset):
//   SpriteHeader (12 bytes):
//     u16 animation_count
//     u16 frame_meta_offset    // Offset to frame array from sprite start
//     u32 rle_data_offset      // Offset to RLE pixels
//     u32 palette_offset       // Offset to 256-color CLUT (512 bytes)
//   AnimationEntry[animation_count] (12 bytes each):
//     u32 animation_id
//     u16 frame_count
//     u16 frame_index          // First frame in frame array
//     u16 flags
//     u16 extra
//   SpriteFrameMetadata[total_frames] (36 bytes each):
//     See struct definition above

// =============================================================================
// MANUAL TILE/SPRITE VISUALIZER
// =============================================================================
// To visualize a specific tile or sprite, uncomment and adjust the addresses below.
// You can find addresses by browsing Levels -> LevelN -> Secondary -> Tile Pixels
// and Palettes.
//
// Example: To visualize tiles from Level 1 (PHRO):
//   1. Find the palette container offset in Secondary segment TOC
//   2. Find the tile pixels offset in Secondary segment TOC
//   3. Set PALETTE_ADDR to palette container + first palette offset
//   4. Set TILES_ADDR to tile pixels asset offset
//   5. Set TILE_COUNT based on tile header's tile count

// Uncomment to enable manual tile visualization:
// #define PALETTE_ADDR 0x123456    // Offset to 256-color palette
// #define TILES_ADDR 0x234567      // Offset to tile pixel data
// #define TILE_COUNT 100           // Number of 16x16 tiles to show
// #define TILES_PER_ROW 16         // Tiles per row in the atlas

// Manual palette (for testing - replace with actual address)
// PSXColor test_palette[256] @ PALETTE_ADDR [[name("Test Palette")]];

// Manual tile atlas visualization
// struct TileAtlas {
//     u8 pixels[256 * TILE_COUNT];
// } [[name("Tile Atlas"), 
//     hex::visualize("bitmap", pixels, 16 * TILES_PER_ROW, (TILE_COUNT / TILES_PER_ROW) * 16, test_palette)]];
// TileAtlas tile_atlas @ TILES_ADDR;

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
