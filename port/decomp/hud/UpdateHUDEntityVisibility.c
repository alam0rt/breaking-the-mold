/* =============================================================================
 * UpdateHUDEntityVisibility.c  --  PC-port HUD counter / visibility refresh
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c UpdateHUDEntityVisibility
 * (lines 25192-25373). Walks the HUD container entity's display groups and,
 * per group, either pushes the current PlayerState counter into each display
 * entity's value field (+0x115 / +0x116) while the group is visible, or -- once
 * the group's lead entity signals "hide" at +0x110 -- unlinks every entity in
 * the group from the render list (its spriteContext at entity+0x34) and the tick
 * list, then clears the group's active flag (container +0xa2..0xab).
 *
 * param_1 is the BYTE address (integer) of the HUD container entity; every
 * param_1 + N is byte-offset integer arithmetic, not word indexing. The nested
 * *(int *)(param_1 + N) reads yield sub-entity byte addresses, dereferenced the
 * same way.
 *
 * On the title screen every container +0xa2..0xab flag is 0 (CreateMenuEntities
 * cleared them), so all blocks are skipped and this returns immediately; the
 * RemoveFromRenderList / RemoveFromTickList helpers are never reached there.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void RemoveFromRenderList(void *gameState, int arg);
extern void RemoveFromTickList(void *gameState, void *entity);

void UpdateHUDEntityVisibility(int param_1) {
    int bVar1;
    PlayerState *pPVar2;
    u32 uVar3;
    s32 iVar4;
    u32 uVar5;

    /* +0xa2: "extra life" trio at +0x94/+0x98/+0x9c, gated on sub-entity +0x101 */
    if ((*(u8 *)(uintptr_t)(param_1 + 0xa2) != 0) &&
        (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x94) + 0x101) != 0)) {
        RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x94) + 0x34));
        RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x94));
        RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x98) + 0x34));
        RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x98));
        RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x9c) + 0x34));
        RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x9c));
        *(u8 *)(uintptr_t)(param_1 + 0xa2) = 0;
    }

    /* +0xa3: orb counter -- lead +0x20, display +0x24/+0x28 */
    pPVar2 = g_pPlayerState;
    if (*(u8 *)(uintptr_t)(param_1 + 0xa3) != 0) {
        if (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x20) + 0x110) == 0) {
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x24) + 0x116) = (u16)g_pPlayerState->orb_count;
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x28) + 0x116) = (u16)pPVar2->orb_count;
        } else {
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x20) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x24) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x28) + 0x34));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x20));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x24));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x28));
            *(u8 *)(uintptr_t)(param_1 + 0xa3) = 0;
        }
    }

    /* +0xa4: lives counter -- lead +0x2c, display +0x30/+0x34 */
    pPVar2 = g_pPlayerState;
    if (*(u8 *)(uintptr_t)(param_1 + 0xa4) != 0) {
        if (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x2c) + 0x110) == 0) {
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x30) + 0x116) = (u16)g_pPlayerState->lives;
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x34) + 0x116) = (u16)pPVar2->lives;
        } else {
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x2c) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x30) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x34) + 0x34));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x2c));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x30));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x34));
            *(u8 *)(uintptr_t)(param_1 + 0xa4) = 0;
        }
    }

    /* +0xa5: swirly-Q counter -- lead +0x38, display +0x3c/+0x40 */
    pPVar2 = g_pPlayerState;
    if (*(u8 *)(uintptr_t)(param_1 + 0xa5) != 0) {
        if (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x38) + 0x110) == 0) {
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x3c) + 0x116) = (u16)g_pPlayerState->swirly_q_count;
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x40) + 0x116) = (u16)pPVar2->swirly_q_count;
        } else {
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x38) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x3c) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x40) + 0x34));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x38));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x3c));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x40));
            *(u8 *)(uintptr_t)(param_1 + 0xa5) = 0;
        }
    }

    /* +0xa6: 1970-icon counter -- 3-slot loop over +0x44/+0x48/+0x4c, digit +0x115 */
    if (*(u8 *)(uintptr_t)(param_1 + 0xa6) != 0) {
        uVar5 = 0;
        if (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x44) + 0x110) == 0) {
            for (uVar5 = 0; (uVar5 & 0xff) < 3; uVar5 = uVar5 + 1) {
                *(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)((uVar5 & 0xff) * 4 + param_1 + 0x44) + 0x115) =
                    g_pPlayerState->icon_1970_count;
            }
        } else {
            uVar3 = 0;
            bVar1 = 1;
            while (bVar1) {
                iVar4 = uVar3 * 4 + param_1;
                uVar5 = uVar5 + 1;
                RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(iVar4 + 0x44) + 0x34));
                uVar3 = uVar5 & 0xff;
                RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(iVar4 + 0x44));
                bVar1 = uVar3 < 3;
            }
            *(u8 *)(uintptr_t)(param_1 + 0xa6) = 0;
        }
    }

    /* +0xa7: hamster counter -- 3-slot loop over +0x50/+0x54/+0x58, digit +0x115 */
    if (*(u8 *)(uintptr_t)(param_1 + 0xa7) != 0) {
        uVar5 = 0;
        if (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x50) + 0x110) == 0) {
            for (uVar5 = 0; (uVar5 & 0xff) < 3; uVar5 = uVar5 + 1) {
                *(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)((uVar5 & 0xff) * 4 + param_1 + 0x50) + 0x115) =
                    g_pPlayerState->hamster_count;
            }
        } else {
            uVar3 = 0;
            bVar1 = 1;
            while (bVar1) {
                iVar4 = uVar3 * 4 + param_1;
                uVar5 = uVar5 + 1;
                RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(iVar4 + 0x50) + 0x34));
                uVar3 = uVar5 & 0xff;
                RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(iVar4 + 0x50));
                bVar1 = uVar3 < 3;
            }
            *(u8 *)(uintptr_t)(param_1 + 0xa7) = 0;
        }
    }

    /* +0xa8: phoenix-hands counter -- lead +0x5c, display +0x60/+100(0x64) */
    pPVar2 = g_pPlayerState;
    if (*(u8 *)(uintptr_t)(param_1 + 0xa8) != 0) {
        if (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x5c) + 0x110) == 0) {
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x60) + 0x116) = (u16)g_pPlayerState->phoenix_hands;
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 100) + 0x116) = (u16)pPVar2->phoenix_hands;
        } else {
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x5c) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x60) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 100) + 0x34));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x5c));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x60));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 100));
            *(u8 *)(uintptr_t)(param_1 + 0xa8) = 0;
        }
    }

    /* +0xa9: phart-heads counter -- lead +0x68, group +0x6c, display +0x70/+0x74 (4 entities) */
    pPVar2 = g_pPlayerState;
    if (*(u8 *)(uintptr_t)(param_1 + 0xa9) != 0) {
        if (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x68) + 0x110) == 0) {
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x70) + 0x116) = (u16)g_pPlayerState->phart_heads;
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x74) + 0x116) = (u16)pPVar2->phart_heads;
        } else {
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x68) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x6c) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x70) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x74) + 0x34));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x68));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x6c));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x70));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x74));
            *(u8 *)(uintptr_t)(param_1 + 0xa9) = 0;
        }
    }

    /* +0xaa: universe-enemas counter -- lead +0x78, display +0x7c/+0x80 */
    pPVar2 = g_pPlayerState;
    if (*(u8 *)(uintptr_t)(param_1 + 0xaa) != 0) {
        if (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x78) + 0x110) == 0) {
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x7c) + 0x116) = (u16)g_pPlayerState->universe_enemas;
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x80) + 0x116) = (u16)pPVar2->universe_enemas;
        } else {
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x78) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x7c) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x80) + 0x34));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x78));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x7c));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x80));
            *(u8 *)(uintptr_t)(param_1 + 0xaa) = 0;
        }
    }

    /* +0xab: super-willies counter -- lead +0x84, display +0x88/+0x8c */
    pPVar2 = g_pPlayerState;
    if (*(u8 *)(uintptr_t)(param_1 + 0xab) != 0) {
        if (*(u8 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x84) + 0x110) == 0) {
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x88) + 0x116) = (u16)g_pPlayerState->super_willies;
            *(u16 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x8c) + 0x116) = (u16)pPVar2->super_willies;
        } else {
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x84) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x88) + 0x34));
            RemoveFromRenderList(g_pGameState, *(s32 *)(uintptr_t)(*(s32 *)(uintptr_t)(param_1 + 0x8c) + 0x34));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x84));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x88));
            RemoveFromTickList(g_pGameState, *(void **)(uintptr_t)(param_1 + 0x8c));
            *(u8 *)(uintptr_t)(param_1 + 0xab) = 0;
        }
    }

    return;
}
