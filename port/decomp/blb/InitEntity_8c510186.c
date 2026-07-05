/* =============================================================================
 * InitEntity_8c510186.c  --  PC-port bobbing-platform HUD entity constructor
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c InitEntity_8c510186
 * (0x8002A0FC region). Builds a fixed-screen sprite entity (sprite hash
 * 0x8c510186) anchored near the top of the HUD, wires the shared HUD idle FSM,
 * then swaps in the bobbing-platform vtable (D_8001056C) and per-frame tick
 * (PlatformOscillateWithBobbingTick, stored only -- fires during the frame
 * loop, not here). Finishes by stamping the sprite-context's semi-transparent
 * blend params and resolving its texture page (abr=3).
 *
 * Note: InitEntitySprite is called with mode 0 (4-bit) here, and GetTPage uses
 * abr=3, unlike the mode-1/abr-1 HUD digits in CreateMenuEntities.
 *
 * Offsets (Entity = 0x80; entity[2] at +0x100; spriteContext BasicEntity 0x10):
 *   +0x18 collisionVtable, +0x08 eventMarker, +0x0C eventCallback, +0x00 tick,
 *   +0x24 moveX, +0x2C moveY, +0x14 active, +0x34 spriteContext.
 *   entity[2]: +0x106 tickcfg, +0x108 anchorY, +0x10E/+0x10C event, +0x110/+0x111 alloc.
 *   spriteContext: +0x10/+0x12 slot x/y, +0x24 tpage, +0x32 depth byte,
 *                  +0x34/+0x35 blend bytes, +0x36 blend byte.
 * ---------------------------------------------------------------------------*/
#include "common.h"

extern void *g_pBlbHeapBase;
extern void *InitEntitySprite(void *e, u32 spriteId, s16 z, s16 x, s16 y, u8 mode);
extern void  EntitySetState(void *e, u32 marker, void *fn);
extern long  GetTPage(int tp, int abr, int x, int y);

extern void *D_800105CC[];                 /* EntityCallbackTableBase_800105cc */
extern void *D_8001056C[];                 /* bobbing-platform vtable          */
extern s32   StubReturnZero(void);
extern s32   EntityTick_PlatformRideIdle(void);
extern void  PlatformOscillateWithBobbingTick(void *e);
extern s32   GetWorldPositionX(void);
extern s32   GetWorldPositionY(void);
extern u32   D_800A5988;                   /* FSM default marker 0xFFFF0000    */
extern void *D_800A598C;                   /* PlatformRideComplete             */

void InitEntity_8c510186(void *entity) {
    u8 *e = (u8 *)entity;
    u8 *sc;
    s16 y = *(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x20;

    InitEntitySprite(e, 0x8c510186, 10000, 0x18, y, 0);
    *(void **)(e + 0x18) = D_800105CC;
    *(s32 *)(e + 0x08) = -0x10000;
    *(void **)(e + 0x0C) = (void *)StubReturnZero;
    *(s32 *)(e + 0x00) = -0x10000;
    *(void **)(e + 0x04) = (void *)EntityTick_PlatformRideIdle;
    *(s32 *)(e + 0x24) = -0x10000;
    *(void **)(e + 0x28) = (void *)GetWorldPositionX;
    *(s32 *)(e + 0x2C) = -0x10000;
    *(void **)(e + 0x30) = (void *)GetWorldPositionY;
    *(u16 *)(e + 0x106) = 0x98;             /* &entity[2].tickCallback + 2 */
    *(u8  *)(e + 0x14) = 0;                  /* active = 0 */
    *(u8  *)(e + 0x10E) = 0;
    *(u16 *)(e + 0x10C) = 0;
    *(s16 *)(e + 0x108) = y;                 /* &entity[2].eventMarker */
    *(u8  *)(e + 0x110) = 1;                 /* &entity[2].allocSize */
    EntitySetState(e, D_800A5988, D_800A598C);
    *(u8 *)(e + 0x111) = 0;                  /* &entity[2].allocSize + 1 */
    *(void **)(e + 0x18) = D_8001056C;
    *(s32 *)(e + 0x00) = -0x10000;
    *(void **)(e + 0x04) = (void *)PlatformOscillateWithBobbingTick;

    sc = *(u8 **)(e + 0x34);
    *(u8 *)(sc + 0x34) = 0x20;               /* &pBVar2[3].widthOrU        */
    *(u8 *)(sc + 0x35) = 0x60;               /* &pBVar2[3].widthOrU + 1    */
    *(u8 *)(sc + 0x36) = 0x30;               /* &pBVar2[3].heightOrV       */
    sc = *(u8 **)(e + 0x34);
    *(u16 *)(sc + 0x24) = (u16)GetTPage(*(u8 *)(sc + 0x32), 3,
                                        *(s16 *)(sc + 0x10) & ~0x3F,
                                        *(s16 *)(sc + 0x12) & ~0xFF);
}
