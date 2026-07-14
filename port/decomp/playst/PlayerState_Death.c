/* =============================================================================
 * PlayerState_Death.c  --  PC-port player "dead" state install
 * =============================================================================
 * Faithful transcription of PlayerState_Death (INCLUDE_ASM in src/playst.c).
 * Installs the player's death state each frame it is active: clears the
 * grounded latch (gs+0x170), disables render-scale easing (+0x178), resolves
 * the sprite context's texture page, and installs the death callback slots --
 * tick = PlayerTickCallback (+0x0), event = PlayerCallback_DeathDebrisSpawner
 * (+0x8), a cleared input slot (+0x104), and the position-update render slot
 * ApplyEntityPositionUpdate (+0x1C) -- then sets the death sprite (0x1B301085)
 * and the death latch (+0x158).
 *
 * Round-2 level-1 stub (334 hits): the player died and this state was inert.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"

extern long GetTPage(int tp, int abr, int x, int y);
extern void SetEntitySpriteId(void *entity, u32 spriteId, s32 flag);

extern char PlayerTickCallback[];
extern char PlayerCallback_DeathDebrisSpawner[];
extern char ApplyEntityPositionUpdate[];

void PlayerState_Death(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;
    u8 *sc;

    gs[0x170] = 0;
    e[0x178] = 1;
    sc = *(u8 **)(e + 0x34);
    *(u16 *)(sc + 0x24) = (u16)GetTPage(sc[0x32], 1,
                                        *(s16 *)(sc + 0x10) & ~0x3F,
                                        *(s16 *)(sc + 0x12) & ~0xFF);

    *(u32 *)(e + 0x00) = 0xFFFF0000u;
    *(void **)(e + 0x04) = (void *)PlayerTickCallback;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;
    *(void **)(e + 0x0C) = (void *)PlayerCallback_DeathDebrisSpawner;
    *(u32 *)(e + 0x104) = 0;
    *(void **)(e + 0x108) = NULL;
    *(u32 *)(e + 0x1C) = 0xFFFF0000u;
    *(void **)(e + 0x20) = (void *)ApplyEntityPositionUpdate;

    SetEntitySpriteId(e, 0x1B301085, 1);
    e[0x128] = 0;
    e[0x158] = 1;
}
