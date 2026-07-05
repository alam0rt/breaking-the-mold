/* =============================================================================
 * InitMenuEntity.c  --  PC-port title/options menu entity constructor
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/passwd/InitMenuEntity.s
 * (0x80076928, 0x1B0 bytes; reference = export/SLES_010.90.c 101929). Builds the
 * menu avatar entity that SpawnPlayerAndEntities creates for menu levels
 * (level flag 0x200): initialises its sprite (hash 0xB8700CA1, alloc 1000),
 * installs the menu tick callback + collision vtable, stashes the password-level
 * list + count, refreshes the background colour, then dispatches to the per-page
 * stage initialiser (InitMenuStage1..4) chosen by the current stage index.
 *
 * Byte offsets are transcribed directly from the disassembly:
 *   entity+0x00 u32 FSM marker (0xFFFF0000)   entity+0x04 tick cb (MenuTickCallback)
 *   entity+0x10 s16 allocSize (0x3E8=1000)    entity+0x18 collision vtable
 *   entity+0x34 spriteContext ptr (->+0xA active byte cleared)
 *   entity+0x100 menuData (password-level list ptr / input)
 *   entity+0x12C/0x12D u8 dispatch flags      entity+0x134 stageData
 *   entity+0x138 u8 menuType                  entity+0x13A s16 (cleared)
 *   entity+0x104[idx*4] object array used by the tail vtable dispatch
 * The tail (when entity[0x12C] != 0) fetches object = entity[0x104 + 0x12D*4],
 * its vtable = *(object+0x18), and calls (*(vtable+0x24))(object + *(s16*)(vtable
 * +0x20)) -- the standard entity FSM-slot dispatch.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

/* Sprite hash for the menu backdrop (authoring id, resolved by InitEntitySprite). */
#define MENU_SPRITE_HASH  0xB8700CA1u

extern void InitEntitySprite(void *entity, u32 hash, s16 allocSize, s16 a3, s16 a4, u8 a5);
extern void UpdateBackgroundColor(void *entity);
extern u8   GetCurrentStageIndex(void *ctx);
extern void InitMenuStage1(s32 entity);
extern void InitMenuStage2(s32 entity);
extern void InitMenuStage3(s32 entity);
extern void InitMenuStage4(s32 entity);
extern void MenuTickCallback(void);   /* referenced by address only */

/* Collision vtable at rodata 0x80011E94 (weak-backed on PC). */
extern u8 D_80011E94[] asm("D_80011E94");
/* Menu SFX-debounce byte (defined in src/menu.c as MENU_SELECT_SOUND_DEBOUNCE). */
extern u8 MENU_SELECT_SOUND_DEBOUNCE asm("D_800A6045");

void *InitMenuEntity(void *entity, s32 menuData, s32 stageData, u8 menuType) {
    u8 *e = (u8 *)entity;
    u8 *spriteCtx;
    u8  stage;

    InitEntitySprite(e, MENU_SPRITE_HASH, 1000, 0, 0, 1);
    spriteCtx = *(u8 **)(e + 0x34);
    *(void **)(e + 0x18) = D_80011E94;
    MENU_SELECT_SOUND_DEBOUNCE = 0;
    *(s16 *)(e + 0x13A) = 0;
    spriteCtx[0xA] = 0;
    e[0x12C] = 0;
    e[0x12D] = 0;
    *(s16 *)(e + 0x10) = 0x3E8;
    *(s32 *)(e + 0x100) = menuData;
    *(s32 *)(e + 0x134) = stageData;
    e[0x138] = menuType;
    *(u32 *)(e + 0x00) = 0xFFFF0000;
    *(void **)(e + 0x04) = (void *)MenuTickCallback;

    UpdateBackgroundColor(e);

    stage = (u8)(GetCurrentStageIndex((u8 *)g_pGameState + 0x84) & 0xFF);
    if (stage >= 5) {
        stage = 1;
    }
    if (stage == 2) {
        InitMenuStage2((s32)(uintptr_t)e);
    } else if (stage == 3) {
        InitMenuStage3((s32)(uintptr_t)e);
    } else if (stage == 4) {
        InitMenuStage4((s32)(uintptr_t)e);
    } else {
        InitMenuStage1((s32)(uintptr_t)e);
    }

    /* Tail: dispatch the selected sub-object's FSM-slot callback. */
    if (e[0x12C] != 0) {
        u8 *obj = *(u8 **)(e + 0x104 + (u32)e[0x12D] * 4);
        u8 *vtable = *(u8 **)(obj + 0x18);
        void (*fn)(void *) = *(void (**)(void *))(vtable + 0x24);
        fn(obj + *(s16 *)(vtable + 0x20));
    }
    return entity;
}
