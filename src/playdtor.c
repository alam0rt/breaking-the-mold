#include "common.h"
#include "functions.h"
#include "globals.h"

typedef struct EntityWithSpuVoiceAt10c {
    SpriteEntity sprite;
    u8 pad100[0x10C - 0x100];
    u32 spuVoice;
} EntityWithSpuVoiceAt10c;

extern u8 ENTITY_SPU_STOP_AT10C_VTABLE[] asm("D_80011D34");

/* Entity destructor referenced from the +0x0C slot of vtable D_80011D34
 * (playst rodata). Swaps the entity vtable to the teardown vtable D_80011D34,
 * stops the SPU voice handle stored at +0x10C, runs the shared destroy/free
 * path, and additionally returns the allocation to the BLB heap when flag
 * bit 0 is set. One of four `EntityDestructor_WithSPU*` variants — the
 * "At10c" suffix names the SPU-voice field offset.
 *
 * Splat-assigned filename "playdtor" is positional (this fn sits just after
 * the playst module) — NOT confirmed as the PlayerDestroy callback. Per
 * docs/symbol_addrs, PlayerDestroy's destructor is
 * `EntityDestructor_WithSPUVoiceStop` @ 0x80059B58, a different function. */
void EntityDestructor_WithSPUStopAt10c(EntityWithSpuVoiceAt10c *entity, u32 flags) {
    entity->sprite.base.collisionVtable = ENTITY_SPU_STOP_AT10C_VTABLE;
    StopSPUVoice(entity->spuVoice);
    DestroyEntityAndFreeMemory(&entity->sprite, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}
