/* =============================================================================
 * InitSwitchBlockEntity.c  --  PC-port conversion (0x800437AC)
 * =============================================================================
 * Constructor for the switch block (the pushable/triggerable block the
 * spawner above creates): sprite at 160/120, vtable D_800110E8, type id
 * 0x385 at +0x10, cleared runtime block (+0x100..+0x10F), mini-Klaymen
 * scale callbacks (only when the GameState scale word +0x11C is 0xC000 and
 * level flag bit 11 is clear), tick TrackerEntityTickCallback, event
 * EntityEventHandler_SetFlag0x110, active bytes +0x111/+0xFD, and the
 * prim's tpage stamp. Returns the entity.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x,
                                s16 y, s32 flags);
extern s32  GetLevelFlags(void *ctx);
extern void SetupEntityScaleCallbacks(void *e);
extern void TrackerEntityTickCallback();
extern void EntityEventHandler_SetFlag0x110();
extern long GetTPage(int tp, int abr, int x, int y);

extern u8 VT_SWITCH_BLOCK[] asm("D_800110E8");

Entity *InitSwitchBlockEntity(void *arg0, u32 spriteId, s16 z) {
    u8 *p = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;
    u8 *prim;

    InitEntitySprite((Entity *)p, spriteId, z, 0xA0, 0x78, 0);
    p[0x110] = 0;
    *(void **)(p + 0x18) = VT_SWITCH_BLOCK;
    *(s16 *)(p + 0x10) = 0x385;
    *(s32 *)(p + 0x100) = 0;
    *(s32 *)(p + 0x104) = 0;
    *(s16 *)(p + 0x108) = 0;
    *(s16 *)(p + 0x10A) = 0;
    *(s16 *)(p + 0x10C) = 0;
    *(s16 *)(p + 0x10E) = 0;

    if (*(s32 *)(gs + 0x11C) == 0xC000 &&
        !((GetLevelFlags(gs + 0x84) >> 11) & 1)) {
        SetupEntityScaleCallbacks(p);
    }

    *(u32 *)(p + 0x0) = 0xFFFF0000u;
    *(void **)(p + 0x4) = (void *)TrackerEntityTickCallback;
    *(u32 *)(p + 0x8) = 0xFFFF0000u;
    *(void **)(p + 0xC) = (void *)EntityEventHandler_SetFlag0x110;

    p[0x111] = 1;
    p[0xFD] = 1;

    prim = *(u8 **)(p + 0x34);
    *(s16 *)(prim + 0x24) =
        (s16)GetTPage(prim[0x32], 1,
                      *(s16 *)(prim + 0x10) & ~0x3F,
                      *(s16 *)(prim + 0x12) & ~0xFF);
    return (Entity *)p;
}
