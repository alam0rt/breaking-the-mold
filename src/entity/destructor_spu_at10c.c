#include "common.h"

typedef struct EntityWithSpuVoiceAt10c {
    u8 pad0[0x18];
    void *vtable;
    u8 pad1[0x10C - 0x1C];
    u32 spuVoice;
} EntityWithSpuVoiceAt10c;

extern u8 D_80011D34[];
extern void *D_800A5954;
extern void func_8007C7B8(u32 spuVoice);
extern void func_8001CA60(void *entity, s32 flags);
extern void func_800145A4(void *heap, void *ptr, s32 arg2, s32 arg3);

void func_80070FD8(EntityWithSpuVoiceAt10c *entity, u32 flags) {
    entity->vtable = D_80011D34;
    func_8007C7B8(entity->spuVoice);
    func_8001CA60(entity, 0);
    if (flags & 1) {
        func_800145A4(D_800A5954, entity, 0, 0);
    }
}
