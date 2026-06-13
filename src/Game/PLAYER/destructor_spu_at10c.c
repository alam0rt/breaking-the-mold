#include "common.h"

typedef struct EntityWithSpuVoiceAt10c {
    u8 pad0[0x18];
    void *vtable;
    u8 pad1[0x10C - 0x1C];
    u32 spuVoice;
} EntityWithSpuVoiceAt10c;

extern u8 D_80011D34[];
extern void *D_800A5954;
extern void StopSPUVoice(u32 spuVoice);
extern void DestroyEntityAndFreeMemory(void *entity, s32 flags);
extern void FreeFromHeap(void *heap, void *ptr, s32 arg2, s32 arg3);

void EntityDestructor_WithSPUStopAt10c(EntityWithSpuVoiceAt10c *entity, u32 flags) {
    entity->vtable = D_80011D34;
    StopSPUVoice(entity->spuVoice);
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(D_800A5954, entity, 0, 0);
    }
}
