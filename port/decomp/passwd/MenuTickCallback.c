/* =============================================================================
 * MenuTickCallback.c  --  PC-port title-menu per-frame tick
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c MenuTickCallback (0x800779A8,
 * INCLUDE_ASM in src/passwd.c). The tick callback for the title-screen menu
 * entity. On stage 1 (the main title page) it runs an idle-attract timer: after
 * 0x708 (1800) frames of no input it seeks the next demo level, arms a direct
 * level load ('c' = 0x63) and the demo-return flag, and advances the demo index.
 * Every frame it also runs the menu input handler and the generic entity update,
 * and if the demo flag has been raised it swaps its own tick to the 20-frame
 * DemoCountdownCallback.
 *
 * Dispatched by EntityTickLoopWithCamera with a single argument, so the extra
 * MIPS register params the export shows (param_2/param_3) are undefined on entry
 * and modelled here as zero-initialised locals (they are only meaningful once
 * the demo branch sets them before forwarding to EntityUpdateCallback).
 *
 * menuEntity is the export's `undefined4 *` (word index => *4 bytes):
 *   menuEntity[0x40] -> +0x100 (a pointer; its target's s16 gates the timer)
 *   +0x13A idle timer (u16)     +0x13C demo countdown (u16)
 *   menuEntity[0x4d] -> +0x134 demo-sequence byte array pointer
 *   menuEntity+0x4e  -> +0x138 demo entry count (u8)
 *   menuEntity[0]/[1] -> +0x00 tickMarker / +0x04 tickCallback
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include "Game/game_state.h"
#include <stdint.h>

extern u8   g_bDemoIndex asm("D_800A6043");
extern u8   g_bDemoFlag  asm("D_800A6045");

extern u8   GetCurrentStageIndex(void *levelCtx);
extern void SeekToLevelInSequence(void *ctx, s32 world, s32 dec, s32 back);
extern void MenuInputHandler(int menuEntity);
extern void EntityUpdateCallback(void *entity, u32 a1, s16 a2);
extern void DemoCountdownCallback(int menuEntity, u32 a1, s16 a2);

void MenuTickCallback(void *menuEntityPtr) {
    u8 *e = (u8 *)menuEntityPtr;
    u32 param_2 = 0;
    s16 param_3 = 0;
    u8  stage;

    stage = GetCurrentStageIndex(&g_pGameState->level_context);
    if (4 < stage) {
        stage = 1;
    }

    if (*(s16 *)(*(void **)(e + 0x100)) == 0) {
        if (stage == 1) {
            u16 idle = *(u16 *)(e + 0x13A) + 1;
            *(u16 *)(e + 0x13A) = idle;
            if (0x708 < idle) {
                param_3 = 1;
                if (*(char *)(e + 0x138) != 0) {
                    u8 *seq = *(u8 **)(e + 0x134);
                    param_2 = (u32)seq[g_bDemoIndex];
                    param_3 = 1;
                    SeekToLevelInSequence(&g_pGameState->level_context,
                                          seq[g_bDemoIndex], 1, 0);
                    g_pGameState->direct_level_load = 'c';
                    g_pGameState->demo_return_flag = 1;
                    g_bDemoIndex = g_bDemoIndex + 1;
                    if (*(u8 *)(e + 0x138) <= g_bDemoIndex) {
                        g_bDemoIndex = 0;
                    }
                }
            }
        }
    } else {
        *(u16 *)(e + 0x13A) = 0;
    }

    MenuInputHandler((int)(intptr_t)e);
    EntityUpdateCallback(e, param_2, param_3);
    if (g_bDemoFlag != 0) {
        *(u16 *)(e + 0x13C) = 0x14;
        *(u32 *)(e + 0x00) = 0xffff0000;
        *(void **)(e + 0x04) = (void *)DemoCountdownCallback;
    }
}
