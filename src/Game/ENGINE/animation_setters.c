#include "common.h"

extern void *g_pBlbHeapBase;
extern u8 g_EntityVtable_Destroyed[];
extern u8 g_EntityVtable_ResourceType1[];
extern u8 g_EntityVtable_ResourceType2[];
extern u8 g_EntityVtable_ResourceType3[];
extern u8 g_EntityVtable_ResourceType4[];
extern u8 g_EntityVtable_SpriteBase[];
extern u8 g_EntityVtable_PartialDestroy[];
extern u8 g_EntityVtable_SimpleDestruct[];
extern u8 g_EntityVtable_LevelDestroy[];

extern void FreeFromHeap(void *heap, void *ptr, s32 arg2, s32 arg3);
extern void FreeMultiAllocResource(void *ptr, s32 type);
extern void FreeResourceType2(void *ptr, s32 type);
extern void FreeResourceType3(void *ptr, s32 type);

typedef struct { s32 a; s32 b; } S32Pair;

typedef struct AnimEntity {
    u8 pad00[0xC0];
    /* 0xC0 */ u32 pendingFrame;
    /* 0xC4 */ u32 pendingLoopFrame;
    /* 0xC8 */ u32 pendingSpriteSource;
    u8 padCC[0xE0 - 0xCC];
    /* 0xE0 */ u16 animChangeFlags;
    u8 padE2[0xF5 - 0xE2];
    /* 0xF5 */ u8 pendingAnimActive;
} AnimEntity;

void SetAnimationFrameIndex(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags;

    entity->pendingFrame = (u16)value;
    newFlags = (flags | 0x8) & 0xFDFF;
    entity->animChangeFlags = newFlags;
    if (((flags | 0x8) & 3) == 0) {
        entity->animChangeFlags = newFlags | 0x1;
    }
}

void SetAnimationFrameCallback(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x208;

    entity->pendingFrame = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x209;
    }
}

void SetEntityTargetFrame(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;
    u16 orFlags = flags | 0x10;
    u16 newFlags = orFlags & 0xFBFF;

    entity->pendingLoopFrame = (u16)value;
    entity->animChangeFlags = newFlags;
    if (orFlags & 0x4) {
        u16 flags2 = *(volatile u16 *)&entity->animChangeFlags;
        u32 loopVal = entity->pendingLoopFrame;
        entity->pendingFrame = loopVal;
        entity->animChangeFlags = (flags2 | 0x8) & 0xFDFF;
    }
    {
        u16 flags3 = entity->animChangeFlags;
        if ((flags3 & 3) == 0) {
            entity->animChangeFlags = flags3 | 0x1;
        }
    }
}

void SetAnimationLoopFrame(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;

    entity->pendingLoopFrame = value;
    flags |= 0x410;
    entity->animChangeFlags = flags;
    if (flags & 0x4) {
        u16 flags2 = *(volatile u16 *)&entity->animChangeFlags;
        u32 loopVal = entity->pendingLoopFrame;
        entity->pendingFrame = loopVal;
        entity->animChangeFlags = flags2 | 0x208;
    }
    {
        u16 flags3 = entity->animChangeFlags;
        if ((flags3 & 3) == 0) {
            entity->animChangeFlags = flags3 | 0x1;
        }
    }
}

void SetAnimationSpriteId(AnimEntity *entity, u32 spriteId) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags;

    entity->pendingSpriteSource = (u16)spriteId;
    newFlags = (flags | 0x20) & 0xF7FF;
    entity->animChangeFlags = newFlags;
    if (((flags | 0x20) & 3) == 0) {
        entity->animChangeFlags = newFlags | 0x1;
    }
}

void SetAnimationSpriteCallback(AnimEntity *entity, void *callback) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x820;

    entity->pendingSpriteSource = (u32)callback;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x821;
    }
}

void SetAnimationActive(AnimEntity *entity, u8 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x100;

    entity->pendingAnimActive = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x101;
    }
}

void EntitySetRenderFlags(AnimEntity *entity, u8 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x80;

    *(u8 *)((u8 *)entity + 0xF4) = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x81;
    }
}

void func_8001D268(AnimEntity *entity, u8 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x40;

    *(u8 *)((u8 *)entity + 0xF3) = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x41;
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", TickEntityAnimation);

typedef struct AdvAnimState {
    u8 pad_D8[0xD8];
    s16 field_D8;
    s16 field_DA;
    u16 field_DC;
    s16 field_DE;
    u8 pad_E0[0xF0 - 0xE0];
    u8 field_F0;
    u8 field_F1;
} AdvAnimState;

void AdvanceAnimationFrame(AdvAnimState *e) {
    s16 current = e->field_DA;
    s16 target = e->field_DE;

    if (current == target) {
        if (e->field_F1 != 0) {
            e->field_DA = e->field_DC;
        }
        return;
    }

    if (e->field_F0 == 0) {
        s16 newVal = current + 1;
        e->field_DA = newVal;
        if (newVal >= e->field_D8) {
            e->field_DA = 0;
        }
    } else {
        s16 newVal = current - 1;
        e->field_DA = newVal;
        if (newVal < 0) {
            e->field_DA = e->field_D8 - 1;
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", ApplyPendingSpriteState);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", UpdateSpriteFrameData);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", UpdateEntityRender);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", UploadEntityTextureIfDirty);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", FindFrameIndexByValue);

extern void StepAnimationSequence(void *entity);

void StartAnimationSequence(u8 *entity, s32 animData, s16 startFrame) {
    *(s16 *)&entity[0xE2] = 0;
    *(s16 *)&entity[0xE4] = startFrame;
    *(s32 *)&entity[0x94] = animData;
    StepAnimationSequence(entity);
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", StepAnimationSequence);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", EntityProcessCallbackQueue);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", EntitySetState);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", EntitySetCallback);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", InitLayerRenderContext_Standard);

void EntityDestructor_FreeMultiAlloc(void *entity, s32 flags) {
    u8 *resource;
    resource = *(u8 **)((u8 *)entity + 0x1C);
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_ResourceType1;
    if (resource) {
        FreeMultiAllocResource(resource, 3);
    }
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", UpdateParallaxScrollWithWrap_Standard);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", InitLayerRenderContext_Medium);

void EntityDestructor_FreeResourceType2(void *entity, s32 flags) {
    u8 *resource;
    resource = *(u8 **)((u8 *)entity + 0x1C);
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_ResourceType2;
    if (resource) {
        FreeResourceType2(resource, 3);
    }
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", UpdateParallaxScrollWithWrap_Medium);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", InitLayerRenderContext_Small);

void EntityDestructor_FreeResourceType3(void *entity, s32 flags) {
    u8 *resource;
    resource = *(u8 **)((u8 *)entity + 0x1C);
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_ResourceType3;
    if (resource) {
        FreeResourceType3(resource, 3);
    }
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", UpdateParallaxScrollWithWrap_Small);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", InitLayerScrollContext);

void FreeResourceType4(void *entity, s32 flags) {
    u8 *resource;
    resource = *(u8 **)((u8 *)entity + 0x20);
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_ResourceType4;
    if (resource) {
        FreeFromHeap(g_pBlbHeapBase, resource, 0, 0);
    }
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", EntityTick_AnimationFrameAdvance);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", func_8001FC40);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", UpdateEntityScreenPositionWithPalette);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", InitPlayerEntity);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", EntityApplyMovementCallbacks);

void EntityDestructor_FreeWithChildRef(void *entity, s32 flags) {
    u8 *child;
    u8 *childRef;
    u8 *resource;

    resource = *(u8 **)((u8 *)entity + 0xB0);
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_SpriteBase;
    if (resource) {
        FreeFromHeap(g_pBlbHeapBase, resource, 0, 0);
    }
    FreeFromHeap(g_pBlbHeapBase, *(void **)((u8 *)entity + 0x90), 4, 0);
    child = *(u8 **)((u8 *)entity + 0x34);
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_PartialDestroy;
    if (child) {
        childRef = *(u8 **)(child + 0xC);
        ((void (*)(void *, s32))*(s32 *)(childRef + 0x14))(
            (void *)(child + *(s16 *)(childRef + 0x10)), 3);
    }
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

typedef struct EntityAccessorView {
    u8 pad00[0x1C];
    s32 field_1C;       /* 0x1C */
    s32 field_20;       /* 0x20 */
    s32 field_24;       /* 0x24 */
    u8 pad28[0x2C - 0x28];
    s32 field_2C;       /* 0x2C */
    s16 field_30;       /* 0x30 */
    s16 field_32;       /* 0x32 */
    s32 field_34;       /* 0x34 */
    u8 pad38[0x3A - 0x38];
    u8 field_3A;        /* 0x3A */
    u8 field_3B;        /* 0x3B */
    u8 pad3C[0x50 - 0x3C];
    s32 field_50;       /* 0x50 */
    s32 field_54;       /* 0x54 */
    s32 field_58;       /* 0x58 */
    s32 field_5C;       /* 0x5C */
    u8 pad60[0x68 - 0x60];
    s16 field_68;       /* 0x68 */
    s16 field_6A;       /* 0x6A */
    u8 pad6C[0x70 - 0x6C];
    u16 field_70;       /* 0x70 */
    u16 field_72;       /* 0x72 */
    u8 field_74;        /* 0x74 */
    u8 field_75;        /* 0x75 */
    u8 pad76[0x98 - 0x76];
    s32 field_98;       /* 0x98 */
    s32 field_9C;       /* 0x9C */
    u8 padA0[0xB0 - 0xA0];
    s32 field_B0;       /* 0xB0 */
    u8 padB4[0xCC - 0xB4];
    s32 field_CC;       /* 0xCC */
    u8 padD0[0xD8 - 0xD0];
    s16 field_D8;       /* 0xD8 */
    s16 field_DA;       /* 0xDA */
    u16 field_DC;       /* 0xDC */
    s16 field_DE;       /* 0xDE */
    u8 padE0[0xF0 - 0xE0];
    u8 field_F0;        /* 0xF0 */
    u8 field_F1;        /* 0xF1 */
    u8 padF2[0xF6 - 0xF2];
    u8 field_F6;        /* 0xF6 */
} EntityAccessorView;

s32 func_800200DC(EntityAccessorView *e) {
    return e->field_20;
}

void func_800200E8(EntityAccessorView *e, u8 value) {
    e->field_3B = value;
}

void func_800200F0(EntityAccessorView *e, u8 value) {
    e->field_3A = value;
}

void func_800200F8(EntityAccessorView *e, s16 value) {
    e->field_32 = value;
}

void func_80020100(EntityAccessorView *e, s16 value) {
    e->field_30 = value;
}

void func_80020108(EntityAccessorView *e, s32 value) {
    e->field_24 = value;
}

void func_80020110(EntityAccessorView *e, s32 value) {
    e->field_20 = value;
}

s32 func_80020118(EntityAccessorView *e) {
    return e->field_1C;
}

void func_80020124(EntityAccessorView *e, u8 value) {
    e->field_3B = value;
}

void func_8002012C(EntityAccessorView *e, u8 value) {
    e->field_3A = value;
}

void func_80020134(EntityAccessorView *e, s16 value) {
    e->field_32 = value;
}

void func_8002013C(EntityAccessorView *e, s16 value) {
    e->field_30 = value;
}

void func_80020144(EntityAccessorView *e, s32 value) {
    e->field_24 = value;
}

void func_8002014C(EntityAccessorView *e, s32 value) {
    e->field_20 = value;
}

s32 func_80020154(EntityAccessorView *e) {
    return e->field_1C;
}

void func_80020160(EntityAccessorView *e, u8 value) {
    e->field_3B = value;
}

void func_80020168(EntityAccessorView *e, u8 value) {
    e->field_3A = value;
}

void func_80020170(EntityAccessorView *e, s16 value) {
    e->field_32 = value;
}

void func_80020178(EntityAccessorView *e, s16 value) {
    e->field_30 = value;
}

void func_80020180(EntityAccessorView *e, s32 value) {
    e->field_24 = value;
}

void func_80020188(EntityAccessorView *e, s32 value) {
    e->field_20 = value;
}

s32 func_80020190(EntityAccessorView *e) {
    return e->field_1C;
}

s32 func_8002019C(EntityAccessorView *e) {
    return e->field_B0;
}

void func_800201A8(EntityAccessorView *e, u8 value) {
    e->field_F6 = value;
}

void func_800201B0(EntityAccessorView *e, S32Pair val) {
    *(S32Pair *)&e->field_98 = val;
}

s16 func_800201D0(EntityAccessorView *e) {
    return e->field_DA;
}

s32 func_800201DC(EntityAccessorView *e) {
    return e->field_CC;
}

u16 func_800201E8(EntityAccessorView *e) {
    return e->field_70;
}

u16 func_800201F4(EntityAccessorView *e) {
    return e->field_72;
}

void func_80020200(EntityAccessorView *e, u16 value) {
    e->field_72 = value;
}

s32 func_80020208(EntityAccessorView *e) {
    return e->field_5C;
}

s32 func_80020214(EntityAccessorView *e) {
    return e->field_58;
}

void func_80020220(EntityAccessorView *e, s32 value) {
    e->field_5C = value;
}

void func_80020228(EntityAccessorView *e, s32 value) {
    e->field_58 = value;
}

void func_80020230(EntityAccessorView *e, s32 value) {
    e->field_58 = value;
    e->field_5C = value;
}

s32 func_8002023C(EntityAccessorView *e) {
    return e->field_54;
}

s32 func_80020248(EntityAccessorView *e) {
    return e->field_50;
}

void func_80020254(EntityAccessorView *e, s32 value) {
    e->field_54 = value;
}

void func_8002025C(EntityAccessorView *e, s32 value) {
    e->field_50 = value;
}

void func_80020264(EntityAccessorView *e, s32 value) {
    e->field_50 = value;
    e->field_54 = value;
}

u8 func_80020270(EntityAccessorView *e) {
    return e->field_75;
}

u8 func_8002027C(EntityAccessorView *e) {
    return e->field_74;
}

s32 StubReturnZero(void) {
    return 0;
}

void func_80020290(EntityAccessorView *e, s16 x, s16 y) {
    e->field_68 = x;
    e->field_6A = y;
}

void func_8002029C(EntityAccessorView *e, s16 value) {
    e->field_6A = value;
}

void func_800202A4(EntityAccessorView *e, s16 value) {
    e->field_68 = value;
}

typedef struct Vec4s16 {
    s16 x;  /* 0x0 */
    s16 y;  /* 0x2 */
    s16 w;  /* 0x4 */
    s16 h;  /* 0x6 */
} Vec4s16;

void func_800202AC(EntityAccessorView *e, Vec4s16 *v) {
    v->x = (v->x << 16) / e->field_58;
    v->w = (v->w << 16) / e->field_58;
    v->y = (v->y << 16) / e->field_5C;
    v->h = (v->h << 16) / e->field_5C;
}

s32 func_800203AC(EntityAccessorView *e, s32 value) {
    return (value << 16) / e->field_5C;
}

s32 func_800203E8(EntityAccessorView *e, s32 value) {
    return (value << 16) / e->field_58;
}

void *func_80020424(void *dst, void *src) {
    __builtin_memcpy(dst, (u8 *)src + 0x38, 8);
    return dst;
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", GetEntityYPosition);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", GetEntityXPosition);

s16 func_80020588(EntityAccessorView *e) {
    return e->field_6A;
}

s16 func_80020594(EntityAccessorView *e) {
    return e->field_68;
}

s32 func_800205A0(EntityAccessorView *e) {
    return e->field_34;
}

void func_800205AC(EntityAccessorView *e, S32Pair val) {
    *(S32Pair *)((u8 *)e + 0x2C) = val;
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", TransformYCoord);

void func_80020668(EntityAccessorView *e, S32Pair val) {
    *(S32Pair *)((u8 *)e + 0x24) = val;
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", TransformXCoord);

void func_80020724(EntityAccessorView *e, S32Pair val) {
    *(S32Pair *)((u8 *)e + 0x1C) = val;
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", InvokeEntityRenderCallback);

u16 func_800207C8(EntityAccessorView *e) {
    return e->field_2C;
}

void func_800207D4(void) {
}

void func_800207DC(void) {
}

void FreeEntityNoTeardown_80020818(void *entity, s32 size);

void EntityDestructor_Simple(EntityAccessorView *entity, s32 flags) {
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeEntityNoTeardown_80020818(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80020818(void *entity, s32 size) {
    FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
}

extern u8 CdBLB_ReadSectors(u16 arg0, u16 arg1);

u8 BLB_ReadSectorsWrapper(u32 sector, u32 count) {
    return CdBLB_ReadSectors((u16)sector, (u16)count);
}

extern void InitMenuEntityWithVtable(void *entity, s32 arg);
extern void *PassThroughFunction(void *ptr);
extern void RemoveEntityFromAllLists(void *entity, void *child);
extern void RemoveEntityFromUpdateQueue(void *entity);
extern void RemoveFromRenderList(void *entity);
extern void RemoveFromTickList(void *entity, void *child);
void RemoveFromUpdateQueue(u8 *entity);
extern void RemoveFromZOrderList(void *entity);
extern void ClearEntityDefList(void *entity);
extern void FreeEntityLists(void *entity);
extern void builtin_delete(void *ptr);
extern void ConditionalDelete(void *ptr, s32 type);

void *InitEntityWithTable(void *entity) {
    InitMenuEntityWithVtable(entity, 0);
    *(s32 *)((u8 *)entity + 0x18) = (s32)g_EntityVtable_SimpleDestruct;
    PassThroughFunction((void *)((u8 *)entity + 0x84));
    return entity;
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", LoadBLBHeader);

void DestroyEntity(u8 *entity, s32 flags) {
    *(s32 *)(entity + 0x18) = (s32)g_EntityVtable_SimpleDestruct;
    RemoveFromUpdateQueue(entity);
    RemoveFromZOrderList(entity);
    ClearEntityDefList(entity);
    FreeEntityLists(entity);
    if (*(s32 *)(entity + 0x3C) != 0) {
        builtin_delete(*(void **)(entity + 0x3C));
    }
    ConditionalDelete(entity + 0x84, 2);
    *(s32 *)(entity + 0x18) = (s32)g_EntityVtable_LevelDestroy;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void RemoveFromUpdateQueue(u8 *entity) {
    u8 *heap = (u8 *)g_pBlbHeapBase;

    *(s16 *)(heap + 0xA08A) = 0;
    if (*(s32 *)(entity + 0x108) != 0) {
        FreeFromHeap(heap, *(void **)(entity + 0x108), 0, 0);
        *(s32 *)(entity + 0x108) = 0;
        *(s16 *)(entity + 0x104) = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", RemoveFromZOrderList);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", EntityTickLoopWithCamera);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", DeferredEntityRemoval);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", EntityRemoval);

// lint:ok - common.h doesn't include entity.h, local definition needed
typedef struct EntityListNode {
    struct EntityListNode *next;  /* 0x00 */
    u8 *entity;                  /* 0x04 */
} EntityListNode;

typedef struct EntityListHead {
    u8 pad[0x1C];
    EntityListNode *head;        /* 0x1C */
} EntityListHead;

void EntityTickLoop(EntityListHead *list) {
    EntityListNode *node;

    node = list->head;
    while (node) {
        u8 *entity = (u8 *)node->entity;
        u8 *vtable = *(u8 **)(entity + 0x18);
        s16 offset = *(s16 *)(vtable + 0x10);
        void (*fn)(void *) = (void (*)(void *))*(s32 *)(vtable + 0x14);
        fn((void *)(entity + offset));
        node = node->next;
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", RenderEntities);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/animation_setters", EntityDestructCallback);
