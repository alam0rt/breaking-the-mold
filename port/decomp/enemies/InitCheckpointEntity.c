/* =============================================================================
 * InitCheckpointEntity.c  --  PC-port checkpoint entity constructor
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/enemies/InitCheckpointEntity.s
 * (0x80041498, 0x18C). Builds a level checkpoint: the main sprite (0x80729081,
 * id 0x3DE) placed at the spawn position (x-0x1E, y+2), the checkpoint vtable
 * (D_800111A8), queue sort-key 0x384 (+0x10), the floating/collision tick and a
 * stub event handler, then a secondary decoration sprite (0x88329285, id 0x3DD)
 * added to the render list. Returns the entity.
 *
 * The stub form returned void, so the caller queued a garbage pointer -> corrupt
 * update queue -> crash; the real constructor returns the initialised entity.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void *InitEntitySprite(void *e, u32 spriteId, s16 z, s16 x, s16 y, u8 mode);
extern void  SetAnimationFrameIndex(void *e, s16 frame);
extern void  SetAnimationActive(void *e, s32 active);
extern void  AddEntityToSortedRenderList(void *gameState, void *entity);
extern u8   *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);

extern void *D_800111A8[];                                   /* checkpoint vtable */
extern char EntityFloatingWithCollisionTick[];
extern char StubReturnZero[];

void *InitCheckpointEntity(void *entity, void *spawnData) {
    u8 *e = (u8 *)entity;
    u8 *sd = (u8 *)spawnData;
    s16 x = (s16)(*(u16 *)(sd + 0x8) - 0x1E);
    s16 y = (s16)(*(u16 *)(sd + 0xA) + 2);
    u8 *child;
    s32 animFrame;

    InitEntitySprite(e, 0x80729081u, 0x3DE, x, y, 0);
    *(void **)(e + 0x18) = D_800111A8;
    *(u16 *)(e + 0x10) = 0x384;                 /* update-queue sort key */
    *(void **)(e + 0x100) = spawnData;

    *(u32 *)(e + 0x00) = 0xFFFF0000u;           /* tick slot */
    *(void **)(e + 0x04) = (void *)EntityFloatingWithCollisionTick;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;           /* event slot */
    *(void **)(e + 0x0C) = (void *)StubReturnZero;

    e[0x108] = (u8)(*(s32 *)((u8 *)g_pGameState + 0x10C) & 3);
    animFrame = *(s32 *)(*(u8 **)(e + 0x100) + 0xC);
    if ((u32)animFrame < 0xF) {
        SetAnimationFrameIndex(e, (s16)animFrame);
    }
    SetAnimationActive(e, 0);

    child = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x100, 1, 0);
    {
        u8 *sd2 = *(u8 **)(e + 0x100);
        s16 cx = (s16)(*(u16 *)(sd2 + 0x8) - 0x1E);
        s16 cy = (s16)(*(u16 *)(sd2 + 0xA) + 2);
        child = (u8 *)InitEntitySprite(child, 0x88329285u, 0x3DD, cx, cy, 0);
    }
    AddEntityToSortedRenderList(g_pGameState, child);
    *(void **)(e + 0x104) = child;
    e[0x109] = 0;
    e[0x10A] = 0;
    return entity;
}
