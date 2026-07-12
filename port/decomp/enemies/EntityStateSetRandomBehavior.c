/* =============================================================================
 * EntityStateSetRandomBehavior.c  --  PC-port conversion (0x8003C1A8)
 * =============================================================================
 * Sparkle-collectible behavior seeder: coin-flips (BIOS rand, draw order
 * matters for demo determinism) the event slot at +0x8 between
 * EntityEventHandlerWithRandomWalk and EntityEventHandlerWalk, clears the
 * slot at +0x1C, installs TimedSparkleCollectibleTick as the tick (+0x0),
 * and re-applies the sprite id from the type record (+0x114 -> +0x10).
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void EntityEventHandlerWithRandomWalk();
extern void EntityEventHandlerWalk();
extern void TimedSparkleCollectibleTick();
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 flag);
extern s32  rand(void);

void EntityStateSetRandomBehavior(void *arg0) {
    u8 *e = (u8 *)arg0;
    void *fn;

    if (rand() & 1) {
        fn = (void *)EntityEventHandlerWithRandomWalk;
    } else {
        fn = (void *)EntityEventHandlerWalk;
    }
    *(u32 *)(e + 0x8) = 0xFFFF0000u;
    *(void **)(e + 0xC) = fn;

    *(u32 *)(e + 0x1C) = 0;
    *(void **)(e + 0x20) = NULL;

    *(u32 *)(e + 0x0) = 0xFFFF0000u;
    *(void **)(e + 0x4) = (void *)TimedSparkleCollectibleTick;

    SetEntitySpriteId(arg0, *(u32 *)(*(u8 **)(e + 0x114) + 0x10), 1);
}
