/* =============================================================================
 * InitTimerDisplayEntity.c  --  PC-port HUD timer entity constructor
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c InitTimerDisplayEntity
 * (0x8002A3B4 region). Builds the HUD timer sprite (hash 0x6a351094), wires the
 * shared HUD idle FSM, then swaps in the rotating-platform vtable (D_8001054C)
 * and per-frame tick (PlatformOscillateWithRotationTick, stored only). Overrides
 * the sprite-context z-order to 0x271A and resolves its texture page (abr=1).
 * RETURNS the entity pointer (the caller assigns the result).
 *
 * The export types param_1 as `undefined4 *` (u32 word index): param_1[N] is
 * byte offset N*4; `param_1 + K` (pointer arithmetic) is byte offset K*4.
 * ---------------------------------------------------------------------------*/
#include "common.h"

extern void *g_pBlbHeapBase;
extern void *InitEntitySprite(void *e, u32 spriteId, s16 z, s16 x, s16 y, u8 mode);
extern void  EntitySetState(void *e, u32 marker, void *fn);
extern long  GetTPage(int tp, int abr, int x, int y);

extern void *D_800105CC[];                 /* EntityCallbackTableBase_800105cc */
extern void *D_8001054C[];                 /* rotating-platform vtable         */
extern s32   StubReturnZero(void);
extern s32   EntityTick_PlatformRideIdle(void);
extern void  PlatformOscillateWithRotationTick(void *e);
extern s32   GetWorldPositionX(void);
extern s32   GetWorldPositionY(void);
extern u32   D_800A5988;                   /* FSM default marker 0xFFFF0000    */
extern void *D_800A598C;                   /* PlatformRideComplete             */

void *InitTimerDisplayEntity(void *entity) {
    u8 *e = (u8 *)entity;
    u8 *sc;
    s16 y = *(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20;

    InitEntitySprite(e, 0x6a351094, 10000, 0x120, y, 0);
    *(void **)(e + 0x18) = D_800105CC;
    *(s32 *)(e + 0x08) = -0x10000;
    *(void **)(e + 0x0C) = (void *)StubReturnZero;
    *(s32 *)(e + 0x00) = -0x10000;
    *(void **)(e + 0x04) = (void *)EntityTick_PlatformRideIdle;
    *(s32 *)(e + 0x24) = -0x10000;
    *(void **)(e + 0x28) = (void *)GetWorldPositionX;
    *(s32 *)(e + 0x2C) = -0x10000;
    *(void **)(e + 0x30) = (void *)GetWorldPositionY;
    *(u16 *)(e + 0x106) = 200;               /* &entity[2].tickCallback + 2 */
    *(u8  *)(e + 0x14) = 0;                   /* active = 0 */
    *(u8  *)(e + 0x10E) = 0;
    *(u16 *)(e + 0x10C) = 0;
    *(s16 *)(e + 0x108) = y;                  /* &entity[2].eventMarker */
    *(u8  *)(e + 0x110) = 1;                  /* &entity[2].allocSize */
    EntitySetState(e, D_800A5988, D_800A598C);
    *(void **)(e + 0x18) = D_8001054C;
    *(u8 *)(e + 0x111) = 0;                   /* &entity[2].allocSize + 1 */

    sc = *(u8 **)(e + 0x34);
    *(u16 *)(sc + 8) = 0x271A;                /* spriteContext zOrder override */
    *(s32 *)(e + 0x00) = -0x10000;
    *(void **)(e + 0x04) = (void *)PlatformOscillateWithRotationTick;

    sc = *(u8 **)(e + 0x34);
    *(u16 *)(sc + 0x24) = (u16)GetTPage(*(u8 *)(sc + 0x32), 1,
                                        *(s16 *)(sc + 0x10) & ~0x3F,
                                        *(s16 *)(sc + 0x12) & ~0xFF);
    return e;
}
