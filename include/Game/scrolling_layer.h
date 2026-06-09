#ifndef SCROLLING_LAYER_H
#define SCROLLING_LAYER_H

#include "common.h"

/* =============================================================================
 * ScrollingLayerData (20 bytes)
 *
 * Runtime scrolling layer state. Tracks the screen-space coordinate
 * rectangle(s) for a scrolling background layer, updated each frame
 * by the camera/parallax system.
 *
 * render_param mirrors LayerEntry+0x0C (z-order priority).
 * (x1,y1)-(x2,y2) define the primary visible tile window.
 * (x3,y3) holds the wrap-around second copy offset for seamless scrolling.
 * ============================================================================= */
typedef struct {
    /* 0x00 */ u32  flags;        /* Layer control flags */
    /* 0x04 */ u32  render_param; /* Z-order / render priority (from LayerEntry) */
    /* 0x08 */ u16  x1;           /* Tile window left */
    /* 0x0A */ u16  y1;           /* Tile window top */
    /* 0x0C */ u16  x2;           /* Tile window right */
    /* 0x0E */ u16  y2;           /* Tile window bottom */
    /* 0x10 */ u16  x3;           /* Wrap-copy X offset */
    /* 0x12 */ u16  y3;           /* Wrap-copy Y offset */
} ScrollingLayerData;  /* Size: 0x14 (20 bytes) */

#endif /* SCROLLING_LAYER_H */
