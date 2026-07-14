/* =============================================================================
 * InitPhartHeadCollectible.c  --  PC-port PhartHead collectible constructor
 * =============================================================================
 * Faithful transcription of InitPhartHeadCollectible (INCLUDE_ASM in
 * src/pickups.c). Builds the primary collectible sprite (hash 0x8C510186),
 * wires the path-follow decor FSM then swaps in the PhartHead vtable
 * (D_80010770) + tick (CollectiblePhartHeadTickCallback), tints the render
 * context (RGB 0x20/0x60/0x30) and resolves its texture page. Then allocates
 * and builds a paired "puff" child sprite (hash 0xA9240484) offset -8/-0x10
 * from the parent, links it at +0x120, and adds it to the render list.
 *
 * This was the top level-3 crash: the stub returned an uninitialised entity
 * whose sprite context was then dereferenced. Implementing it fixes that.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include "Game/entity.h"

extern void *g_pBlbHeapBase;
extern u8 *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern void InitPathFollowingDecorEntity(void *e, void *data, u8 flag);
extern long GetTPage(int tp, int abr, int x, int y);
extern void AddEntityToSortedRenderList(void *gs, void *e);
extern void EntityUpdateCallback(Entity *e);           /* real C */
extern void CollectiblePhartHeadTickCallback(void);    /* still-asm (weak stub) */

extern u8 VT_PATH_DECOR[] asm("D_80010870");
extern u8 VT_PHARTHEAD[]  asm("D_80010770");

Entity *InitPhartHeadCollectible(Entity *ent, u8 *spawn) {
    u8 *e = (u8 *)ent;
    u8 *sc;
    u8 *child;
    u8 *pspawn;

    InitEntitySprite(ent, 0x8C510186, 0x3DE, *(s16 *)(spawn + 8), *(s16 *)(spawn + 0xA), 0);
    *(u8 **)(e + 0x18) = VT_PATH_DECOR;
    InitPathFollowingDecorEntity(e, spawn, 0);
    *(u8 **)(e + 0x18) = VT_PHARTHEAD;
    *(s32 *)(e + 0x00) = -0x10000;
    *(void **)(e + 0x04) = (void *)CollectiblePhartHeadTickCallback;

    sc = *(u8 **)(e + 0x34);
    sc[0x34] = 0x20;
    sc[0x35] = 0x60;
    sc[0x36] = 0x30;
    sc = *(u8 **)(e + 0x34);
    *(u16 *)(sc + 0x24) = (u16)GetTPage(sc[0x32], 3,
                                        *(s16 *)(sc + 0x10) & ~0x3F,
                                        *(s16 *)(sc + 0x12) & ~0xFF);

    /* paired "puff" child sprite */
    child = AllocateFromHeap(g_pBlbHeapBase, 0x120, 1, 0);
    pspawn = *(u8 **)(e + 0x100);
    InitEntitySprite((Entity *)child, 0xA9240484, 0x3DE,
                     *(s16 *)(pspawn + 8), *(s16 *)(pspawn + 0xA), 0);
    *(u8 **)(child + 0x18) = VT_PATH_DECOR;
    InitPathFollowingDecorEntity(child, pspawn, 0);
    *(u8 **)(e + 0x120) = child;
    *(s16 *)(child + 0x68) = *(s16 *)(e + 0x68) - 8;
    *(s16 *)(child + 0x6A) = *(s16 *)(e + 0x6A) - 0x10;
    child[0xF6] = 0;
    *(s32 *)(child + 0x00) = -0x10000;
    *(void **)(child + 0x04) = (void *)EntityUpdateCallback;

    sc = *(u8 **)(child + 0x34);
    sc[0x37] = 0;
    sc[0x34] = 0x20;
    sc[0x35] = 0x60;
    sc[0x36] = 0x30;
    *(u16 *)(sc + 0x08) = 0x3E0;
    AddEntityToSortedRenderList(g_pGameState, child);
    return ent;
}
