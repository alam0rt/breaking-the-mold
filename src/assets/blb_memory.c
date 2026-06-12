#include "common.h"

typedef struct BLBEntityHeader {
    u8 pad[0x18];
    void *vtable;
} BLBEntityHeader;

extern u8 D_80012758[];
extern void *D_800A5954;
extern void func_800145A4(void *heap, void *ptr, s32 arg2, s32 arg3);

void func_80082F24(void *ptr, s32 size);

void func_80082EF0(BLBEntityHeader *entity, u32 flags) {
    entity->vtable = D_80012758;
    if (flags & 1) {
        func_80082F24(entity, 0x1C);
    }
}

void func_80082F24(void *ptr, s32 size) {
    func_800145A4(D_800A5954, ptr, 0, 0);
}
