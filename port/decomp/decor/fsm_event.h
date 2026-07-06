/* =============================================================================
 * fsm_event.h  --  PC-port shared FSM event-slot dispatch helper
 * =============================================================================
 * The engine's tagged event-callback slot (marker lo @+0x08, marker hi @+0x0A,
 * fn @+0x0C) lives at the same offsets in Entity and GameState, so one helper
 * covers both "notify the game state" (event 3, entity removal funnel) and
 * "send event to an entity" (e.g. 0x1013 HUD reveal, 0x1009 timer-done).
 * Marker encoding (see include/Game/entity.h): hi==0 no callback; hi<0 direct
 * call at obj+lo; hi>0 slot-table entry {s32 arg, fn} at
 * *(obj + (s16)fn-field) + (hi-1)*8, target obj+lo+arg.
 * ========================================================================== */
#ifndef PORT_DECOR_FSM_EVENT_H
#define PORT_DECOR_FSM_EVENT_H

#include "common.h"
#include <stdint.h>

typedef void (*FsmEventFn)(void *dst, s32 eventId, s32 arg, void *src);

static void fsm_send_event(u8 *obj, s32 eventId, s32 arg, void *src) {
    s16 hi = *(s16 *)(obj + 0x0A);
    FsmEventFn fn;
    s32 slotArg = 0;
    s32 adj;
    if (hi == 0) {
        return;
    }
    if (hi > 0) {
        u8 *slot = *(u8 **)(obj + *(s16 *)(obj + 0x0C)) + (hi - 1) * 8;
        slotArg = (s16)*(s32 *)slot;
        fn = *(FsmEventFn *)(slot + 4);
    } else {
        fn = *(FsmEventFn *)(obj + 0x0C);
    }
    adj = *(s16 *)(obj + 0x08);
    if (hi > 0) {
        adj += slotArg;
    }
    fn(obj + adj, eventId, arg, src);
}

#endif /* PORT_DECOR_FSM_EVENT_H */
