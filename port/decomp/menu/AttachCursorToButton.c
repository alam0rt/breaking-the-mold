/* =============================================================================
 * AttachCursorToButton.c  --  PC-port menu-cursor attach (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/menu/AttachCursorToButton.s
 * (0x800754CC, 0x140 bytes; reference = export/SLES_010.90.c 100667). Creates the
 * cursor/highlight sprite entity that tracks a menu button: allocates a 0x100
 * entity, builds it from spawn data D_8009CBE8 at (buttonX+0x6A, buttonY+0xE,
 * z=0x7D0), installs its cursor vtable (D_8001208C) + world-position move
 * callbacks, registers it on the render list, and records it on the button at
 * button+0x100 (with the button's active flag at +0x104 cleared). Both the
 * cursor and the button get the {0xFFFF0000, GetWorldPositionX/Y} move-callback
 * FSM slots at +0x24/+0x28 (X) and +0x2C/+0x30 (Y).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void InitEntityWithSprite(void *e, s32 spawnData, s16 zOrder, s16 worldX, s16 worldY);
extern void AddEntityToSortedRenderList(void *gs, void *entity);
extern void GetWorldPositionX(void);   /* referenced by address only */
extern void GetWorldPositionY(void);

extern u8 D_8009CBE8[] asm("D_8009CBE8");   /* cursor spawn data */
extern u8 D_8001208C[] asm("D_8001208C");   /* cursor entity vtable */

void AttachCursorToButton(void *entity) {
    u8 *button = (u8 *)entity;
    u8 *cursor = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x100, 1, 0);
    s16 bx = (s16)(*(u16 *)(button + 0x68) + 0x6A);
    s16 by = (s16)(*(u16 *)(button + 0x6A) + 0xE);

    InitEntityWithSprite(cursor, (s32)(uintptr_t)D_8009CBE8, 0x7D0, bx, by);
    *(void **)(cursor + 0x18) = D_8001208C;
    *(u32 *)(cursor + 0x24) = 0xFFFF0000;
    *(void **)(cursor + 0x28) = (void *)GetWorldPositionX;
    *(u32 *)(cursor + 0x2C) = 0xFFFF0000;
    *(void **)(cursor + 0x30) = (void *)GetWorldPositionY;

    *(void **)(button + 0x100) = cursor;
    AddEntityToSortedRenderList(g_pGameState, cursor);
    button[0x104] = 0;

    *(u32 *)(button + 0x24) = 0xFFFF0000;
    *(void **)(button + 0x28) = (void *)GetWorldPositionX;
    *(u32 *)(button + 0x2C) = 0xFFFF0000;
    *(void **)(button + 0x30) = (void *)GetWorldPositionY;
}
