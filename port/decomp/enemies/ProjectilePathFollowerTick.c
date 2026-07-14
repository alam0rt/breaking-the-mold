/* =============================================================================
 * ProjectilePathFollowerTick.c  --  PC-port projectile path tick
 * =============================================================================
 * Faithful transcription of ProjectilePathFollowerTick (INCLUDE_ASM in
 * src/enemies.c). Per-frame tick for the projectiles spawned by
 * InitEntityWithDeathSpawn: advances worldX/worldY (+0x68/+0x6A) by the
 * per-step velocity pair in the D_8009B634/D_8009B636 path table (4-byte
 * entries indexed by the +0x100 step counter), and once the step count reaches
 * 0x14 spawns the impact def (SpawnProjectileEntityDef) and broadcasts the
 * "collected/hit" event 3 to the interacting entity via the GameState callback
 * dispatch (+0x8/+0xA/+0xC).
 *
 * These piled up (456 hits) once InitEntityWithDeathSpawn started spawning them.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"

extern void SpawnProjectileEntityDef(void *e);   /* still-asm (weak stub) */

extern u8 D_8009B634[] asm("D_8009B634");   /* path velocity X (stride 4) */
extern u8 D_8009B636[] asm("D_8009B636");   /* path velocity Y (stride 4) */

/* Resolve the interacting-entity handler + position offset from the GameState
 * callback table (+0x8 base / +0xA idx / +0xC table-or-fn) and invoke it. */
static void dispatch_gs_callback(u8 *obj, s32 event, void *self) {
    s16 idx = *(s16 *)(obj + 0xA);

    if (idx != 0) {
        void (*fn)(void *, s32, s32, void *);
        s16 base = *(s16 *)(obj + 0x8);
        s32 offset;

        if (idx > 0) {
            u8 *table = *(u8 **)(obj + *(s16 *)(obj + 0xC));
            u8 *entry = (idx << 3) + table;
            s32 argOff = *(s32 *)(entry - 8);
            fn = *(void (**)(void *, s32, s32, void *))(entry - 4);
            offset = (s16)argOff + base;
        } else {
            fn = *(void (**)(void *, s32, s32, void *))(obj + 0xC);
            offset = base;
        }
        fn(obj + offset, event, 0, self);
    }
}

void ProjectilePathFollowerTick(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 step = e[0x100];

    *(u16 *)(e + 0x68) = (u16)(*(u16 *)(e + 0x68) + *(u16 *)(D_8009B634 + step * 4));
    e[0x100] = step + 1;
    *(u16 *)(e + 0x6A) = (u16)(*(u16 *)(e + 0x6A) + *(u16 *)(D_8009B636 + step * 4));

    if ((u8)(step + 1) >= 0x14) {
        SpawnProjectileEntityDef(e);
        dispatch_gs_callback((u8 *)g_pGameState, 3, e);
    }
}
