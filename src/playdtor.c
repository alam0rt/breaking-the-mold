#include "common.h"
#include "functions.h"
#include "globals.h"

typedef struct EntityWithSpuVoiceAt10c {
    u8 pad0[0x18];
    void *vtable;
    u8 pad1[0x10C - 0x1C];
    u32 spuVoice;
} EntityWithSpuVoiceAt10c;

extern u8 D_80011D34[];

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
    entity->vtable = D_80011D34;
    StopSPUVoice(entity->spuVoice);
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}
