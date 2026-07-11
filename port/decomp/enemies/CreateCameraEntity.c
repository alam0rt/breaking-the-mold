/* =============================================================================
 * CreateCameraEntity.c  --  functional-C body for enemies.c CreateCameraEntity
 * =============================================================================
 * Transcribed from asm/nonmatchings/enemies/CreateCameraEntity.s (0x80044F7C,
 * 0x398). Constructs the level camera-helper entity that SpawnPlayerAndEntities
 * creates for most gameplay levels (every level except MENU/PHRO reaches it, so
 * it gates booting the demo-bearing levels on the port).
 *
 * Builds the entity: sprite list D_8009B6B0 (id 0x3CA), camera vtable
 * D_80011088, update-queue key 0x384, tick slot -> TeleporterTransitionTick-
 * Callback. `scale` == 0 installs the default scale callbacks; any other value
 * except the 1.0 sentinel 0x10000 becomes the fixed 16.16 scale in +0x50..0x5C
 * with the Scale{X,Y}ByEntityScale transform slots and pre-divides the +0x68/
 * +0x6A world coordinates. Then it resolves the entity's world position through
 * the transform-X/Y slots (same tagged-slot encoding as fsm_event.h: hi==0
 * none, hi<0 direct fn, hi>0 slot-table entry {arg, fn}) and asks
 * CheckTriggerZoneCollision: zone type 0x2C -> "dark" tint, 0x2E -> "red" tint
 * (or dark if GameState+0x19C is set), written as RGB into the sprite context
 * (+0x34) bytes 0x34..0x36. Finally +0x104 = live scale, +0x108 = 0, and the
 * +0x100 countdown = 0x5A. Returns the entity.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void InitEntityWithSprite(void *entity, void *spriteDef, s32 z, s16 x, s16 y);
extern void SetupEntityScaleCallbacks(void *entity);
extern u8   CheckTriggerZoneCollision(void *gs, s32 x, s32 y, s32 *zoneType, s32 *zoneArg);
extern void TeleporterTransitionTickCallback(void);
extern s16  ScaleXByEntityScale(void);
extern s16  ScaleYByEntityScale(void);
extern unsigned int D_8009B6B0[] asm("D_8009B6B0");   /* camera sprite list */
extern void *D_80011088[] asm("D_80011088");          /* camera vtable      */

typedef s16 (*TransformFn)(void *obj, s16 v);

/* Tagged transform slot {s16 lo, s16 hi, fn/off @+4}: resolve and apply.
 * hi==0: value passes through. hi<0: fn at slot+4, target e+lo. hi>0: the
 * +4 field is an s16 offset to a slot-table pointer in the entity; entry
 * (hi-1)*8 holds {s32 arg, fn}, target e+lo+(s16)arg. */
static s16 transform_slot_apply(u8 *e, s32 slotOff, u16 value) {
    s16 hi = *(s16 *)(e + slotOff + 2);
    TransformFn fn;
    s32 arg = 0;
    s16 target;
    if (hi == 0) {
        return (s16)value;
    }
    if (hi > 0) {
        u8 *tbl = *(u8 **)(e + *(s16 *)(e + slotOff + 4));
        u8 *entry = tbl + hi * 8;
        arg = *(s32 *)(entry - 8);
        fn = *(TransformFn *)(entry - 4);
    } else {
        fn = *(TransformFn *)(e + slotOff + 4);
    }
    target = *(s16 *)(e + slotOff);
    if (hi > 0) {
        target += (s16)arg;
    }
    return fn(e + target, (s16)value);
}

void *CreateCameraEntity(void *arg0, s16 x, s16 y, s32 scale) {
    u8 *e = (u8 *)arg0;
    s32 dark = 0;                       /* zone 0x2C or GameState+0x19C */
    s32 red = 0;                        /* zone 0x2E */
    s32 zoneType;
    s32 zoneArg;
    s16 wx, wy;

    InitEntityWithSprite(e, D_8009B6B0, 0x3CA, x, y);
    *(void **)(e + 0x18) = D_80011088;
    *(u16 *)(e + 0x10) = 0x384;                 /* update-queue sort key */
    *(u32 *)(e + 0x00) = 0xFFFF0000u;           /* tick slot */
    *(void **)(e + 0x04) = (void *)TeleporterTransitionTickCallback;

    if (scale == 0) {
        SetupEntityScaleCallbacks(e);
    } else if (scale != 0x10000) {
        *(s32 *)(e + 0x50) = scale;
        *(s32 *)(e + 0x54) = scale;
        *(s32 *)(e + 0x58) = scale;
        *(s32 *)(e + 0x5C) = scale;
        *(u32 *)(e + 0x24) = 0xFFFF0000u;       /* transform-X slot */
        *(void **)(e + 0x28) = (void *)ScaleXByEntityScale;
        *(u32 *)(e + 0x2C) = 0xFFFF0000u;       /* transform-Y slot */
        *(void **)(e + 0x30) = (void *)ScaleYByEntityScale;
        *(s16 *)(e + 0x68) =
            (s16)(((s32)*(s16 *)(e + 0x68) << 16) / *(s32 *)(e + 0x58));
        *(s16 *)(e + 0x6A) =
            (s16)(((s32)*(s16 *)(e + 0x6A) << 16) / *(s32 *)(e + 0x5C));
    }

    *(s32 *)(e + 0x104) = *(s32 *)(e + 0x50);   /* live scale snapshot */

    wx = transform_slot_apply(e, 0x24, *(u16 *)(e + 0x68));
    wy = transform_slot_apply(e, 0x2C, *(u16 *)(e + 0x6A));
    if (CheckTriggerZoneCollision(g_pGameState, wx, wy, &zoneType, &zoneArg)) {
        if (zoneType == 0x2C) {
            dark = 1;
        } else if (zoneType == 0x2E) {
            red = 1;
        }
    }
    if (*((u8 *)g_pGameState + 0x19C) != 0) {
        dark = 1;
    }

    *(s32 *)(e + 0x108) = 0;
    if (red) {
        u8 *sctx = *(u8 **)(e + 0x34);
        sctx[0x34] = 0x74; sctx[0x35] = 0x24; sctx[0x36] = 0x38;
    } else if (dark) {
        u8 *sctx = *(u8 **)(e + 0x34);
        sctx[0x34] = 0x38; sctx[0x35] = 0x38; sctx[0x36] = 0x60;
    }
    e[0x100] = 0x5A;                            /* transition countdown (byte) */
    return e;
}
