#ifndef LAYER_ENTRY_H
#define LAYER_ENTRY_H

#include "common.h"

/* =============================================================================
 * LayerEntry - Asset 201 (0xC9)
 *
 * Clean-room RE note: field names are inferred from Ghidra traces and BLB data,
 * not original source symbols.
 * 
 * Layer metadata from Asset 201. Each level can have multiple layers,
 * each 92 bytes (0x5C). Layers are rendered back-to-front with z-ordering
 * based on render_param.
 * 
 * Size: 92 bytes (0x5C)
 * Accessor: GetLayerEntry @ 0x8007B700 - returns ctx[4] + index * 0x5C
 * 
 * Scroll Factors:
 *   0x10000 = 1.0 (scrolls with camera)
 *   0x8000 = 0.5 (parallax background)
 *   0x4000 = 0.25 (distant parallax)
 * 
 * Render Param:
 *   Low 16 bits = priority (signed short, lower = further back)
 *   Typical values: 150-800 bg, 900-1100 main, 1200-1500 fg
 * ============================================================================= */

/* RGB color tint (3 bytes) */
typedef struct {
    u8 r;
    u8 g;
    u8 b;
} ColorTintEntry;

typedef struct {
    /* Position and dimensions (0x00-0x0B) */
    /* 0x00 */ u16 x_offset;           /* X position in tiles */
    /* 0x02 */ u16 y_offset;           /* Y position in tiles */
    /* 0x04 */ u16 width;              /* Layer width in tiles */
    /* 0x06 */ u16 height;             /* Layer height in tiles */
    /* 0x08 */ u16 level_width;        /* Level width in tiles (from Asset 100) */
    /* 0x0A */ u16 level_height;       /* Level height in tiles */
    
    /* Render parameter (0x0C-0x0F) - VERIFIED via Ghidra */
    /* Low 16 bits = priority for z-ordering */
    /* 0x0C */ u32 render_param;       /* Low 16 = priority (lower = further back) */
    
    /* Scroll factors (0x10-0x17) - 16.16 fixed-point */
    /* 0x10000 = 1.0 (follows camera), 0x8000 = 0.5 (parallax) */
    /* 0x10 */ u32 scroll_x;           /* X parallax factor */
    /* 0x14 */ u32 scroll_y;           /* Y parallax factor */
    
    /* Auto-scroll parameters (0x18-0x1D), copied into layer render context */
    /* 0x18 */ u16 auto_scroll_speed_x; /* Autonomous wrapped X scroll speed */
    /* 0x1A */ u16 auto_scroll_speed_y; /* Autonomous wrapped Y scroll speed */
    /* 0x1C */ u8  reverse_scroll_x;    /* Reverse autonomous X scroll direction */
    /* 0x1D */ u8  reverse_scroll_y;    /* Reverse autonomous Y scroll direction */
    
    /* Scroll enable flags (0x1E-0x21) - VERIFIED via Ghidra */
    /* 0x1E */ u8  scroll_left_enable; /* If !=0, set GameState+0x59 */
    /* 0x1F */ u8  scroll_right_enable;/* If !=0, set GameState+0x5B */
    /* 0x20 */ u8  scroll_up_enable;   /* If !=0, set GameState+0x58 */
    /* 0x21 */ u8  scroll_down_enable; /* If !=0, set GameState+0x5A */
    
    /* Auto-scroll enable flags (0x22-0x25) - VERIFIED via Ghidra */
    /* 0x22 */ u16 auto_scroll_enable_y; /* Enable autonomous Y scrolling */
    /* 0x24 */ u16 auto_scroll_enable_x; /* Enable autonomous X scrolling */
    
    /* Layer type and flags (0x26-0x2B) */
    /* 0x26 */ u8  layer_type;         /* Layer type (0=normal, 3=skip render) */
    /* 0x27 */ u8  pad_27;
    /* 0x28 */ u16 skip_render;        /* If !=0 (and type!=3), skip this layer */
    /* 0x2A */ u16 pad_2a;
    
    /* Color Tint Table (0x2C-0x5B) - 16 RGB entries, 48 bytes */
    /* Tilemap u16 entries use upper 4 bits (12-15) as color selector */
    /* Tile render reads: (tilemap >> 12) * 3 + color_table */
    /* 0x2C */ ColorTintEntry color_tints[16];
    
} LayerEntry;  /* Size: 0x5C (92 bytes) */

#endif /* LAYER_ENTRY_H */
