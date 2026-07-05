/* =============================================================================
 * InitMenuStage1.c  --  PC-port title-screen page 1 builder (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/passwd/InitMenuStage1.s
 * (0x80076BA0, 0x4C8 bytes; reference = export/SLES_010.90.c 102049). Builds the
 * title/main-menu page:
 *   1. Five background/logo sprites (InitEntitySprite, z=2000, at 0xA0,0xA8).
 *   2. The animated title entity (InitEntityWithSprite from spawn data
 *      D_8009CBDC): installs vtable D_800120AC, world-position move callbacks
 *      (GetWorldPositionX/Y in the moveX/moveY FSM slots), and the idle-anim
 *      state (EntitySetState with the D_800A6050/D_800A6054 slot pair).
 *   3. Four menu buttons: for each, InitEntitySprite(0x10094096) at the position
 *      from the D_8009CB00 record table (stride 6 bytes: x@+12, y@+14, type@+16),
 *      AttachCursorToButton, then store the button into the menu entity's child
 *      table (menuEntity+0x104[childCount], count at +0x12C) and register it.
 *   4. A sprite-scan over D_8009CC08 (stride-4 sprite-hash list): the first
 *      "present" entry (validated via InitSpriteContext/Wrapper) spawns either a
 *      MenuLogoAnim entity (hash 0x40B18011, from D_8009CBF8) or a generic
 *      particle entity (InitParticleEntity), then the scan stops.
 *
 * Offsets/strides/args transcribed directly from the disassembly. Data tables:
 * D_8009CB0C/0E/10 are the button record fields (real C in src/passwd.c);
 * D_8009CBDC/CBF8/CC08 + the menu vtables are still-asm rodata (weak-zeroed on
 * PC until their data segments are faithfully generated -- the sprite-scan then
 * safely finds no entry, and the stored vtables aren't dispatched during init).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void *InitEntitySprite(void *e, u32 spriteId, s16 z, s16 x, s16 y, u8 mode);
extern void *InitEntityWithSprite(void *e, s32 spawnData);
extern void  AddEntityToSortedRenderList(void *gs, void *entity);
extern void  EntitySetState(void *e, u32 marker, void *fn);
extern void  AttachCursorToButton(void *e);
extern void  InitSpriteContext(void *ctx, u32 spriteId);
extern void  InitSpriteContextWrapper(void *out, u32 spriteId);
extern void *InitParticleEntity(void *e, u32 spriteId, u32 packedXY, u8 facing,
                                s32 scale, s16 z, u8 flag);

/* Button record fields (real defs live in src/passwd.c as D_8009CB00[]). */
extern s16 D_8009CB0C[] asm("D_8009CB0C");   /* button X   (stride 3 shorts) */
extern s16 D_8009CB0E[] asm("D_8009CB0E");   /* button Y   */
extern s16 D_8009CB10[] asm("D_8009CB10");   /* button type (low byte)       */

/* Still-asm rodata (weak-zeroed until faithfully generated). */
extern u8  D_8009CBDC[] asm("D_8009CBDC");   /* title spawn data     */
extern u8  D_8009CBF8[] asm("D_8009CBF8");   /* menu-logo spawn data */
extern u32 D_8009CC08[] asm("D_8009CC08");   /* sprite-scan hash list */

/* Menu entity vtables (stored into entity+0x18; dispatched later, not here). */
extern u8 D_800120AC[] asm("D_800120AC");    /* title entity vtable  */
extern u8 D_80012034[] asm("D_80012034");    /* button (pre-attach)  */
extern u8 D_80011FDC[] asm("D_80011FDC");    /* button (post-attach) */
extern u8 D_80011EB4[] asm("D_80011EB4");    /* menu-logo entity     */

/* Idle-anim FSM slot pair (real defs in src/menu.c). */
extern u32   D_800A6050 asm("D_800A6050");   /* state marker */
extern void *D_800A6054 asm("D_800A6054");   /* state callback */

/* Move-callbacks + logo-anim handler (referenced by address only). */
extern void GetWorldPositionX(void);
extern void GetWorldPositionY(void);
extern void MenuLogoAnimEventHandler(void);

static const u32 s_logoHashes[5] = {
    0x68C01218, 0x3080840D, 0x3080820D, 0x30808E0D, 0x38A0C119,
};

void InitMenuStage1(s32 param_1) {
    u8 *menu = (u8 *)(uintptr_t)param_1;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u8 *e;
    s32 i;

    /* ---- 1. five logo/background sprites ---- */
    for (i = 0; i < 5; i++) {
        e = AllocateFromHeap(heap, 0x100, 1, 0);
        e = InitEntitySprite(e, s_logoHashes[i], 2000, 0xA0, 0xA8, 0);
        AddEntityToSortedRenderList(g_pGameState, e);
    }

    /* ---- 2. animated title entity ---- */
    e = AllocateFromHeap(heap, 0x104, 1, 0);
    InitEntityWithSprite(e, (s32)(uintptr_t)D_8009CBDC);
    *(void **)(e + 0x18) = D_800120AC;
    *(u32 *)(e + 0x24) = 0xFFFF0000;
    *(void **)(e + 0x28) = (void *)GetWorldPositionX;
    *(u32 *)(e + 0x2C) = 0xFFFF0000;
    *(void **)(e + 0x30) = (void *)GetWorldPositionY;
    EntitySetState(e, D_800A6050, D_800A6054);
    AddEntityToSortedRenderList(g_pGameState, e);

    /* ---- 3. four menu buttons ---- */
    for (i = 0; (i & 0xFF) < 4; i++) {
        s32 rec = (i & 0xFF) * 3;            /* short index; byte offset = *6 */
        u8  type = (u8)(*(s16 *)((u8 *)D_8009CB10 + (i & 0xFF) * 6));
        s16 bx = D_8009CB0C[rec];
        s16 by = D_8009CB0E[rec];
        u8 *btn = AllocateFromHeap(heap, 0x10C, 1, 0);
        u8  childIdx;

        InitEntitySprite(btn, 0x10094096, 1000, bx, by, 0);
        *(void **)(btn + 0x18) = D_80012034;
        AttachCursorToButton(btn);
        *(void **)(btn + 0x18) = D_80011FDC;
        btn[0x109] = 0;
        btn[0x108] = type;

        childIdx = menu[0x12C];
        *(void **)(menu + 0x104 + childIdx * 4) = btn;
        AddEntityToSortedRenderList(g_pGameState,
                                    *(void **)(menu + 0x104 + childIdx * 4));
        menu[0x12C] = (u8)(menu[0x12C] + 1);
    }

    /* ---- 4. sprite-scan: spawn the first present logo/particle ---- */
    {
        u32 *scan = D_8009CC08;
        u8  wrap[0x20];                      /* InitSpriteContextWrapper output */
        InitSpriteContext(menu + 0x78, scan[0]);
        for (;;) {
            u32 hash = scan[0];
            if (hash == 0) {
                return;
            }
            InitSpriteContextWrapper(wrap, hash);
            /* "present" test: the wrapper's dims (its +0xC word, copied to a
             * s16 pair) must be non-zero. */
            {
                s16 dimA = *(s16 *)&wrap[0xC];
                s16 dimB = *(s16 *)(&wrap[0xC] + 2);
                if (dimA != 0 && dimB != 0) {
                    break;
                }
            }
            scan++;
        }
        {
            u32 hash = scan[0];
            if (hash == 0) {
                return;
            }
            if (hash == 0x40B18011) {
                e = AllocateFromHeap(heap, 0x104, 1, 0);
                InitEntityWithSprite(e, (s32)(uintptr_t)D_8009CBF8);
                *(void **)(e + 0x18) = D_80011EB4;
                *(u32 *)(e + 0x08) = 0xFFFF0000;
                *(void **)(e + 0x0C) = (void *)MenuLogoAnimEventHandler;
                e[0x100] = 0;
            } else {
                u32 packedXY = (u32)0x9F | ((u32)0xA8 << 16);
                e = AllocateFromHeap(heap, 0x108, 1, 0);
                e = InitParticleEntity(e, hash, packedXY, 0, 0x10000, 0x3D4, 0);
            }
            AddEntityToSortedRenderList(g_pGameState, e);
        }
    }
}
