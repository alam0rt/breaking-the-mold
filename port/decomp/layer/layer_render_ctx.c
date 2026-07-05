/* =============================================================================
 * layer_render_ctx.c  --  PC-port layer render-context initialisers (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of the three sibling builders (asm in src/anim.c;
 * reference = export/SLES_010.90.c 15063/15297/15477):
 *   InitLayerRenderContext_Standard @ 0x8001ECC0  (layers > 128x128)
 *   InitLayerRenderContext_Medium   @ 0x8001F150  (layers <= 128x128)
 *   InitLayerRenderContext_Small    @ 0x8001F534  (layers <= 64x64)
 *
 * Each populates the 0x3C-byte layer render context (a BasicEntity-style header
 * + parallax state) and allocates + initialises the size-specific tilemap
 * render sub-object, stored at context+0x1C. Shared field layout:
 *   +0x00 u32 FSM marker (0xFFFF0000)     +0x04 tick callback (parallax updater)
 *   +0x10 s16 priority (30000)            +0x14 u8 active (1)
 *   +0x18 vtable ptr                      +0x1C render sub-object ptr
 *   +0x20/+0x24 parallax scale X/Y (0x10000; overwritten by caller w/ scroll)
 *   +0x34/+0x36 wrap width/height tiles   +0x38/+0x39 auto-scroll enable X/Y
 *   +0x28/+0x2C accum, +0x30/+0x32 speed, +0x3A/+0x3B reverse (all 0 here)
 *
 * Per-variant differences:
 *   Standard: final vtable g_EntityVtable_ResourceType1; sub-object alloc 0x84;
 *             InitTilemapLayerRendering; no sizeE arg (passes flagA?w:0/flagB?h:0);
 *             callback UpdateParallaxScrollWithWrap_Standard.
 *   Medium  : vtable ResourceType2; alloc 0x70; InitTilemapLayer16x16 (takes
 *             sizeE + raw flagA/flagB); callback ..._Medium.
 *   Small   : vtable ResourceType3; alloc 0x80; InitTileLayerPrimitives (sizeE +
 *             flagA?w:0/flagB?h:0); callback ..._Small.
 * All three return the context pointer (param_1) in $v0 -- the caller
 * (InitLayersAndTileState) uses the return value for its field-copy tail.
 * The unaligned stack shuffles in the decompile are just the packed
 * {x,y} position words being forwarded; they pass through as pos0/pos1.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void *InitTilemapLayerRendering(void *sub, void *colorPtr, s16 tileId, s32 tileState,
        u16 *tilemap, u32 pos0, u32 pos1, s32 wrapW, s32 wrapH, u8 mode);
extern void *InitTilemapLayer16x16(void *sub, void *colorPtr, s16 tileId, s32 tileState,
        u16 *tilemap, u32 pos0, u32 pos1, s16 sizeE, u8 flagA, u8 flagB, u8 mode);
extern void *InitTileLayerPrimitives(void *sub, void *colorPtr, s16 tileId, s32 tileState,
        u16 *tilemap, u32 pos0, u32 pos1, s16 sizeE, u16 wrapW, u16 wrapH, u8 mode);
extern void UpdateParallaxScrollWithWrap_Standard(void *ctx);
extern void UpdateParallaxScrollWithWrap_Medium(void *ctx);
extern void UpdateParallaxScrollWithWrap_Small(void *ctx);

/* 0x20-byte entity vtables at fixed rodata addresses (weak-backed on PC). */
extern u8 g_EntityVtable_Destroyed[]      asm("g_EntityVtable_Destroyed");
extern u8 g_EntityVtable_ResourceType1[]  asm("g_EntityVtable_ResourceType1");
extern u8 g_EntityVtable_ResourceType2[]  asm("g_EntityVtable_ResourceType2");
extern u8 g_EntityVtable_ResourceType3[]  asm("g_EntityVtable_ResourceType3");

/* Shared header init (the fields all three write identically before the
 * variant-specific sub-object setup). */
static void layerCtxHeaderInit(u8 *c) {
    *(void **)(c + 0x18) = g_EntityVtable_Destroyed;
    *(u16 *)(c + 0x00) = 0;
    *(u16 *)(c + 0x02) = 0;
    *(s32 *)(c + 0x04) = 0;
    *(u16 *)(c + 0x08) = 0;
    *(u16 *)(c + 0x0A) = 0;
    *(s32 *)(c + 0x0C) = 0;
    *(u16 *)(c + 0x10) = 30000;
    c[0x14] = 1;
    *(u16 *)(c + 0x12) = 0;
}

/* Shared tail (parallax state cleared, marker + callback installed). */
static void layerCtxTail(u8 *c, void *sub, void (*cb)(void *)) {
    *(void **)(c + 0x1C) = sub;
    *(s32 *)(c + 0x20) = 0x10000;
    *(s32 *)(c + 0x24) = 0x10000;
    *(u32 *)(c + 0x00) = 0xFFFF0000;
    *(void **)(c + 0x04) = (void *)cb;
    *(s32 *)(c + 0x28) = 0;
    *(s32 *)(c + 0x2C) = 0;
    *(u16 *)(c + 0x30) = 0;
    *(u16 *)(c + 0x32) = 0;
    c[0x3A] = 0;
    c[0x3B] = 0;
}

void *InitLayerRenderContext_Standard(void *storage, void *colorPtr, s16 tileId, s32 tileState,
        u16 *tilemap, u32 pos0, u32 pos1, u16 w, u16 h, u8 flagA, u8 flagB, u8 mode) {
    u8 *c = (u8 *)storage;
    u16 wrapW, wrapH;
    void *sub;

    layerCtxHeaderInit(c);
    c[0x38] = flagA;
    *(void **)(c + 0x18) = g_EntityVtable_ResourceType1;
    *(u16 *)(c + 0x34) = w;
    *(u16 *)(c + 0x36) = h;
    c[0x39] = flagB;
    wrapW = flagA ? *(u16 *)(c + 0x34) : 0;
    wrapH = flagB ? *(u16 *)(c + 0x36) : 0;

    sub = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x84, 1, 0);
    sub = InitTilemapLayerRendering(sub, colorPtr, tileId, tileState, tilemap,
                                    pos0, pos1, wrapW, wrapH, mode);
    layerCtxTail(c, sub, UpdateParallaxScrollWithWrap_Standard);
    return storage;
}

void *InitLayerRenderContext_Medium(void *storage, void *colorPtr, s16 tileId, s32 tileState,
        u16 *tilemap, u32 pos0, u32 pos1, u16 w, u16 h, s16 sizeE, u8 flagA, u8 flagB, u8 mode) {
    u8 *c = (u8 *)storage;
    void *sub;

    layerCtxHeaderInit(c);
    *(void **)(c + 0x18) = g_EntityVtable_ResourceType2;

    sub = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x70, 1, 0);
    sub = InitTilemapLayer16x16(sub, colorPtr, tileId, tileState, tilemap,
                                pos0, pos1, sizeE, flagA, flagB, mode);
    *(void **)(c + 0x1C) = sub;
    *(s32 *)(c + 0x20) = 0x10000;
    *(s32 *)(c + 0x24) = 0x10000;
    *(u16 *)(c + 0x34) = w;
    *(u16 *)(c + 0x36) = h;
    c[0x38] = flagA;
    c[0x39] = flagB;
    *(s32 *)(c + 0x28) = 0;
    *(s32 *)(c + 0x2C) = 0;
    *(u16 *)(c + 0x30) = 0;
    *(u16 *)(c + 0x32) = 0;
    c[0x3A] = 0;
    c[0x3B] = 0;
    *(u32 *)(c + 0x00) = 0xFFFF0000;
    *(void **)(c + 0x04) = (void *)UpdateParallaxScrollWithWrap_Medium;
    return storage;
}

void *InitLayerRenderContext_Small(void *storage, void *colorPtr, s16 tileId, s32 tileState,
        u16 *tilemap, u32 pos0, u32 pos1, u16 w, u16 h, s16 sizeE, u8 flagA, u8 flagB, u8 mode) {
    u8 *c = (u8 *)storage;
    u16 wrapW, wrapH;
    void *sub;

    layerCtxHeaderInit(c);
    c[0x38] = flagA;
    *(void **)(c + 0x18) = g_EntityVtable_ResourceType3;
    *(u16 *)(c + 0x34) = w;
    *(u16 *)(c + 0x36) = h;
    c[0x39] = flagB;
    wrapW = flagA ? *(u16 *)(c + 0x34) : 0;
    wrapH = flagB ? *(u16 *)(c + 0x36) : 0;

    sub = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x80, 1, 0);
    sub = InitTileLayerPrimitives(sub, colorPtr, tileId, tileState, tilemap,
                                  pos0, pos1, sizeE, wrapW, wrapH, mode);
    layerCtxTail(c, sub, UpdateParallaxScrollWithWrap_Small);
    return storage;
}
