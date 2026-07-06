/* =============================================================================
 * PlayerState_DeathStart.c  --  PC-port player death state init
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/playst/PlayerState_DeathStart.s
 * (0x80069EF4, 0x1C4). Starts the death sequence: stops the player's SPU voice
 * and plays the death sound (0x4810C2C4, keeping the voice handle in +0x174),
 * blows the render scale up to 3x (+0x50/+0x54) easing to 1x (+0x58/+0x5C),
 * flags the game state (gs[0x170]=0 grounded-latch, gs[0x144]=1 player-dead),
 * repoints the position-transform slots (+0x24/+0x2C) at GetWorldPositionX/Y
 * and parks the entity at screen center (160,120), zeroes the velocities,
 * installs PlayerState_FallWithRotation (tick) + PlayerCallback_DeathDebris-
 * Spawner (event) and clears the input/render slots, then sets the death
 * sprite (0x1E28E0D4) and marks +0x158 (death latch).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void StopSPUVoice(s32 voice);
extern u32  PlaySoundEffect(u32 soundId, s16 pan, s8 force);
extern void SetEntitySpriteId(void *entity, u32 spriteId, s32 flag);

extern char GetWorldPositionX[];
extern char GetWorldPositionY[];
extern char PlayerState_FallWithRotation[];
extern char PlayerCallback_DeathDebrisSpawner[];

void PlayerState_DeathStart(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;
    u32 voice;

    gs[0x170] = 0;
    e[0x178] = 1;                          /* disable render-scale easing */
    StopSPUVoice(*(s32 *)(e + 0x174));
    voice = PlaySoundEffect(0x4810C2C4u, 0xA0, 0);
    *(u32 *)(e + 0x174) = voice;

    *(s32 *)(e + 0x50) = 0x30000;          /* render scale 3x */
    *(s32 *)(e + 0x54) = 0x30000;
    *(s32 *)(e + 0x58) = 0x10000;          /* target scale 1x */
    *(s32 *)(e + 0x5C) = 0x10000;
    gs[0x144] = 1;                         /* player-dead flag */

    /* position-transform slots -> screen-relative; park at screen center */
    *(u32 *)(e + 0x24) = 0xFFFF0000u;
    *(void **)(e + 0x28) = (void *)GetWorldPositionX;
    *(u32 *)(e + 0x2C) = 0xFFFF0000u;
    *(void **)(e + 0x30) = (void *)GetWorldPositionY;
    *(s16 *)(e + 0x68) = 0xA0;             /* worldX = 160 */
    *(s16 *)(e + 0x6A) = 0x78;             /* worldY = 120 */

    e[0x128] = 0;
    *(s32 *)(e + 0x10C) = 0;               /* velocities */
    *(s32 *)(e + 0x110) = 0;

    *(u32 *)(e + 0x00) = 0xFFFF0000u;      /* tick */
    *(void **)(e + 0x04) = (void *)PlayerState_FallWithRotation;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;      /* event */
    *(void **)(e + 0x0C) = (void *)PlayerCallback_DeathDebrisSpawner;
    *(u32 *)(e + 0x104) = 0;               /* input slot cleared */
    *(void **)(e + 0x108) = NULL;
    *(u32 *)(e + 0x1C) = 0;                /* render slot cleared */
    *(void **)(e + 0x20) = NULL;

    SetEntitySpriteId(e, 0x1E28E0D4u, 1);
    e[0x158] = 1;                          /* death latch */
}
