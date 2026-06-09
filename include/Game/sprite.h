#ifndef SPRITE_H
#define SPRITE_H

#include "common.h"

/* =============================================================================
 * Sprite system structures for Skullmonkeys (PAL SLES-01090)
 *
 * Sprite data is loaded from Asset 600 in the BLB container.
 * The container begins with a SpriteTOCEntry table, followed by
 * per-sprite SpriteHeader + SpriteFrameEntry arrays + RLE pixel data.
 *
 * At runtime, each loaded sprite gets a SpriteContext allocated for it.
 * The Entity+0x34 (spriteContext) pointer references this struct.
 *
 * See docs/blb/README.md and scripts/blb.hexpat for format details.
 * ============================================================================= */

/* -----------------------------------------------------------------------------
 * SpriteTOCEntry (12 bytes)
 *
 * Table-of-contents entry in the Asset 600 container header.
 * Indexed by sprite_id to locate sprite data within the container.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u32  sprite_id;  /* Sprite identifier */
    /* 0x04 */ u32  size;       /* Size of sprite data in bytes */
    /* 0x08 */ u32  offset;     /* Byte offset from container base */
} SpriteTOCEntry;  /* Size: 0x0C (12 bytes) */

/* -----------------------------------------------------------------------------
 * SpriteHeader (24 bytes)
 *
 * Per-sprite header at the start of each sprite's data block.
 * Followed immediately by frame_count SpriteFrameEntry structs,
 * then RLE-compressed pixel data.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u16  type;         /* Sprite type/format identifier */
    /* 0x02 */ u16  header_size;  /* Size of this header in bytes */
    /* 0x04 */ u16  frames_end;   /* End index of frame array */
    /* 0x06 */ u16  padding_06;
    /* 0x08 */ u32  rle_size;     /* Size of RLE pixel data in bytes */
    /* 0x0C */ u32  sprite_id;    /* Sprite identifier (matches TOC) */
    /* 0x10 */ u16  frame_count;  /* Number of animation frames */
    /* 0x12 */ u16  padding_12;
    /* 0x14 */ u16  unknown_flag; /* Purpose unconfirmed */
    /* 0x16 */ u16  clut_garbage; /* CLUT field (may be stale/unused data) */
} SpriteHeader;  /* Size: 0x18 (24 bytes) */

/* -----------------------------------------------------------------------------
 * SpriteFrameEntry (36 bytes)
 *
 * Per-frame descriptor. Arrays of these follow each SpriteHeader.
 * Indexed by animation frame number.
 *
 * hitbox_* fields define the per-frame collision box, offset from origin.
 * rle_offset is the byte offset into the sprite's RLE pixel data.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u32  callback_id;    /* Animation callback/event ID */
    /* 0x04 */ u16  frame_delay;    /* Duration of this frame in ticks */
    /* 0x06 */ s16  origin_x;       /* Render origin X (sprite center offset) */
    /* 0x08 */ s16  origin_y;       /* Render origin Y */
    /* 0x0A */ u16  width;          /* Pixel width of this frame */
    /* 0x0C */ u16  height;         /* Pixel height of this frame */
    /* 0x0E */ u16  flip_flags;     /* Horizontal/vertical flip flags */
    /* 0x10 */ u32  unknown_10;     /* Purpose unconfirmed */
    /* 0x14 */ s16  hitbox_x;       /* Hitbox X offset from origin */
    /* 0x16 */ s16  hitbox_y;       /* Hitbox Y offset from origin */
    /* 0x18 */ u16  hitbox_width;   /* Hitbox width */
    /* 0x1A */ u16  hitbox_height;  /* Hitbox height */
    /* 0x1C */ u32  flags;          /* Frame behavior flags */
    /* 0x20 */ u32  rle_offset;     /* Byte offset into SpriteHeader's RLE data */
} SpriteFrameEntry;  /* Size: 0x24 (36 bytes) */

/* -----------------------------------------------------------------------------
 * SpriteContext (20 bytes)
 *
 * Runtime sprite management context, allocated per loaded sprite.
 * Referenced by Entity+0x34 (spriteContext).
 *
 * frame_metadata points into the SpriteFrameEntry array.
 * rle_data points to the RLE-compressed pixel buffer.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ void *frame_metadata;    /* Pointer to SpriteFrameEntry array */
    /* 0x04 */ void *secondary_ptr;     /* Secondary data pointer (palette/VRAM context) */
    /* 0x08 */ void *rle_data;          /* Pointer to RLE pixel data */
    /* 0x0C */ u16   max_width;         /* Maximum frame width across all frames */
    /* 0x0E */ u16   max_height;        /* Maximum frame height across all frames */
    /* 0x10 */ u16   total_frame_count; /* Total number of frames */
    /* 0x12 */ u8    unknown_12;        /* Purpose unconfirmed */
    /* 0x13 */ u8    is_valid;          /* Non-zero = context is fully initialized */
} SpriteContext;  /* Size: 0x14 (20 bytes) */

/* -----------------------------------------------------------------------------
 * SpriteTypeCallbackEntry (16 bytes)
 *
 * Per-sprite-type callback table for rendering dispatch.
 * Array of 481 entries at 0x8009D5F8+... (see game category in Ghidra).
 * Indexed by sprite type to select the appropriate render pipeline.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ void *callback_0;    /* Primary callback (setup/decode) */
    /* 0x04 */ void *callback_1;    /* Secondary callback */
    /* 0x08 */ void *tick_callback; /* Per-frame tick callback */
    /* 0x0C */ void *callback_3;    /* Tertiary callback (cleanup/post) */
} SpriteTypeCallbackEntry;  /* Size: 0x10 (16 bytes) */

#endif /* SPRITE_H */
