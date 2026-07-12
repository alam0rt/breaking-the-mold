/* =============================================================================
 * DispatchEventToCollidingEntity.c  --  functional-C body for blb seg
 * DispatchEventToCollidingEntity (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/blb/DispatchEventToCollidingEntity.s
 * (0x800226F8, 0x244). The collision-event dispatcher behind
 * CollisionCheckWrapper (src/entity.c, matched C): find the first entity whose
 * screen box overlaps the query box and fire its tagged event slot
 * (+0x08/+0x0A/+0x0C, fsm_event.h encoding) with (eventId, arg, src); the
 * slot's return value is propagated (0 when nothing hit / no handler).
 *
 * mask == 2 is the player fast path: test ONLY the player entity at gs+0x2C
 * (no collision-mask check); a missing player falls back to the generic walk.
 * The generic path walks the collision list at gs+0x24 (node: next @+0,
 * entity @+4), skipping the querying entity itself and entities whose
 * collision mask (+0x12) doesn't intersect `mask`. Dispatch stops at the
 * FIRST overlapping entity, hit or no handler.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include <stdint.h>

typedef struct { s16 x; s16 y; } BoxCorner;

extern s32 CheckBoxOverlap(Entity *entity, BoxCorner minCorner, BoxCorner maxCorner);

/* fsm_send_event (decor/fsm_event.h) with the return value kept -- the
 * collision event handlers report back through $v0. hi == 0 -> 0. */
typedef s32 (*CollisionEventFn)(void *dst, s32 eventId, s32 arg, void *src);

static s32 send_collision_event(u8 *obj, s32 eventId, s32 arg, void *src) {
    s16 hi = *(s16 *)(obj + 0x0A);
    CollisionEventFn fn;
    s32 slotArg = 0;
    s32 adj;

    if (hi == 0) {
        return 0;
    }
    if (hi > 0) {
        u8 *slot = *(u8 **)(obj + *(s16 *)(obj + 0x0C)) + (hi - 1) * 8;
        slotArg = (s16)*(s32 *)slot;
        fn = *(CollisionEventFn *)(slot + 4);
    } else {
        fn = *(CollisionEventFn *)(obj + 0x0C);
    }
    adj = *(s16 *)(obj + 0x08);
    if (hi > 0) {
        adj += slotArg;
    }
    return fn(obj + adj, eventId, arg, src);
}

s32 DispatchEventToCollidingEntity(GameState *gs, BoxCorner minCorner,
                                   BoxCorner maxCorner, u32 mask, u32 eventId,
                                   u32 arg, Entity *src) {
    u8 *g = (u8 *)gs;
    u8 *node;

    if ((mask & 0xFFFF) == 2) {
        u8 *player = *(u8 **)(g + 0x2C);
        if (player != NULL) {
            if ((CheckBoxOverlap((Entity *)player, minCorner, maxCorner) & 0xFF) == 0) {
                return 0;
            }
            return send_collision_event(player, (u16)eventId, arg, src);
        }
        /* no player: fall through to the generic walk */
    }

    for (node = *(u8 **)(g + 0x24); node != NULL; node = *(u8 **)node) {
        u8 *e = *(u8 **)(node + 4);
        int hit = 0;
        if (e != (u8 *)src && ((mask & 0xFFFF) & *(u16 *)(e + 0x12)) != 0) {
            hit = (CheckBoxOverlap((Entity *)e, minCorner, maxCorner) & 0xFF) != 0;
        }
        if (hit) {
            return send_collision_event(e, (u16)eventId, arg, src);
        }
    }
    return 0;
}
