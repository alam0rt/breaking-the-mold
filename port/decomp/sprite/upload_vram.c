/* =============================================================================
 * upload_vram.c  --  PC-port entity sprite texture/CLUT -> VRAM upload
 * =============================================================================
 * Functional-C reconstructions of the entity-sprite VRAM upload chain:
 *   UploadTextureToVRAM        (asm/nonmatchings/sprite/UploadTextureToVRAM.s,
 *                               0x80015E0C, 0xA8)
 *   UploadCLUTToVRAM           (asm/nonmatchings/sprite/UploadCLUTToVRAM.s,
 *                               0x80015EB4, 0x168; export/SLES_010.90.c 6978)
 *   UploadEntityTextureIfDirty (asm/nonmatchings/anim/UploadEntityTextureIfDirty.s,
 *                               0x8001E5B8, 0xF4)
 *
 * These push an entity's decoded pixel buffer and its palette into the VRAM
 * slots reserved by PrepareSpriteVRAMSlotForContext (ctx +0x10/+0x12 texture
 * x/y, +0x18/+0x1A CLUT x/y, +0x2E vram-ok, +0x32 mode, +0x38 stp-flag).
 * Without them every entity sprite samples an all-zero CLUT and is discarded
 * as transparent -- the "menu sprites are invisible" failure.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

typedef struct { s16 x, y, w, h; } PortRect;

extern int  LoadImage(void *rect, void *data);
extern int  LoadClut(void *clut, int x, int y);
extern void DrawSync(int mode);
extern u8  *AllocateFromHeap(u8 *heap, s32 elemSize, s32 count, s32 flags);
extern void FreeFromHeap(u8 *heap, void *ptr, s32 size, s32 flags);
extern u32  IsEntityOffScreen(void *entity);

/* Upload the sprite context's decoded pixel buffer into its VRAM slot. */
void UploadTextureToVRAM(void *arg0, void *pixels) {
    u8 *c = (u8 *)arg0;
    PortRect rect;
    s32 w;

    if (c[0x2E] == 0) {           /* no VRAM slot reserved */
        return;
    }
    rect.x = *(s16 *)(c + 0x10);
    rect.y = *(s16 *)(c + 0x12);
    if (c[0x32] == 0) {           /* 4-bit: width in 16-bit words = (w+1)>>2 */
        w = (*(s16 *)(c + 0x04) + 1) >> 2;
    } else if (c[0x32] == 1) {    /* 8-bit: (w+1)>>1 */
        w = (*(s16 *)(c + 0x04) + 1) >> 1;
    } else {                      /* 16-bit: w as-is */
        w = *(u16 *)(c + 0x04);
    }
    rect.w = (s16)((w + 1) & 0xFFFE);
    rect.h = (s16)*(u16 *)(c + 0x06);
    LoadImage(&rect, pixels);
}

/* Upload the sprite context's palette into its CLUT slot. When the context's
 * stp flag (+0x38) is set, every entry except index 0 gets the STP bit forced
 * so the sprite participates in semi-transparency. */
void UploadCLUTToVRAM(void *arg0, void *palette) {
    u8  *c = (u8 *)arg0;
    u16 *clut = (u16 *)palette;
    u16 *tmp = NULL;
    u32  n, i;

    if (*(s16 *)(c + 0x26) == 0 || c[0x32] == 2) {
        return;                   /* no CLUT id, or 16-bit direct colour */
    }
    if (c[0x38] != 0) {
        n = (c[0x32] == 0) ? 0x10 : 0x100;
        tmp = (u16 *)AllocateFromHeap((u8 *)g_pBlbHeapBase, 2, (s32)n, 0);
        tmp[0] = clut[0];
        for (i = 1; i < n; i++) {
            tmp[i] = (u16)(clut[i] | 0x8000);
        }
        clut = tmp;
    }
    if (c[0x32] == 0) {
        PortRect rect;
        rect.x = *(s16 *)(c + 0x18);
        rect.y = *(s16 *)(c + 0x1A);
        rect.w = 0x10;
        rect.h = 1;
        LoadImage(&rect, clut);
    } else {
        LoadClut(clut, *(s16 *)(c + 0x18), *(s16 *)(c + 0x1A));
    }
    if (tmp != NULL) {
        DrawSync(0);
        FreeFromHeap((u8 *)g_pBlbHeapBase, tmp, 0, 0);
    }
}

/* Per-tick: flush the entity's dirty CLUT (+0xF8 flag, palette ptr @+0x7C) and
 * dirty texture (+0x76 flag, pixel buffer @+0xB0) to VRAM, skipping off-screen
 * entities (unless force-visible @+0xFD) and pre-decoded static sprites
 * (@+0xF7, which live in the shared sheet cache, not a private slot). */
void UploadEntityTextureIfDirty(void *arg0) {
    u8 *e = (u8 *)arg0;

    if (*(void **)(e + 0x34) == NULL) {
        return;
    }
    if (e[0xF8] != 0) {
        if ((e[0xFD] != 0 || (IsEntityOffScreen(e) & 0xFF) == 0) && e[0xF7] == 0) {
            UploadCLUTToVRAM(*(void **)(e + 0x34), *(void **)(e + 0x7C));
            DrawSync(0);
            e[0xF8] = 0;
        }
    }
    if (e[0x76] != 0) {
        if (e[0xFD] != 0 || (IsEntityOffScreen(e) & 0xFF) == 0) {
            if (e[0xF7] == 0) {
                UploadTextureToVRAM(*(void **)(e + 0x34), *(void **)(e + 0xB0));
                DrawSync(0);
            }
            e[0x76] = 0;
        }
    }
}
