#ifndef CALLBACK_SLOT_H
#define CALLBACK_SLOT_H

#include "common.h"

/* =============================================================================
 * Entity FSM callback slot
 *
 * The engine's per-entity FSM uses 8-byte (marker, fn) pairs scattered through
 * the Entity struct. The `marker` field on Entity is `s32` (see entity.h):
 *   - +0x00/+0x04: tick slot  (tickMarker s32, tickCallback fn)  per-frame tick
 *   - +0x08/+0x0C: event slot (eventMarker s32, eventCallback fn) IPC events
 *   - +0x1C/+0x20: render slot (renderMarker s32, renderCallback fn)
 *   - +0x24/+0x2C: move-x/y   (move{X,Y}Marker s32, move{X,Y}Callback fn)
 *   - +0x98/+0x9C: queued cb  — fires on EntityProcessCallbackQueue
 *
 * The 32-bit marker is decoded by the engine as two halves (lo s16, hi s16);
 * see entity.h "FSM marker encoding". Common values:
 *   0xFFFF0000 (lo=0, hi=-1)  => direct call: fn(entity)
 *   0x00000000 (lo=0, hi= 0)  => disabled / cleared slot
 *   indexed/extended forms encode lo as offset and hi as table index.
 *
 * MATCH discipline:
 *  - cc1 2.7.2 -O2 reliably emits the install sequence as: build the 8-byte
 *    pair on stack (sh markerLo; sh markerHi; sw fn), then `lw u32; lw fn;
 *    sw; sw` into the entity slot. This is a block-copy from a stack-scratch
 *    `CallbackSlot` local — NOT direct stores to the entity fields. Direct
 *    stores would emit `lui 0xFFFF; sw; la fn; sw` (no scratch).
 *  - The `CallbackSlot` typedef below models the SCRATCH struct shape (two
 *    s16 halves + fn pointer), so a `*(CallbackSlot*)&e->tickMarker = local`
 *    cast-assignment reproduces the block-copy idiom.
 *  - Functions that install ONE slot + call SetEntitySpriteId need a 0x28 frame
 *    (16 reg-save + 4 pad + 8 slot + 4 pad + 4 ra + 4 align). The natural way
 *    to get this frame is `PaddedSlotPair` (s32 pad + slot[2]) declared as a
 *    local even when only s[0] is used — the unused s[1] occupies the trailing
 *    16 bytes that cc1 would otherwise pad.
 *  - Functions that install ONE slot + something else (vtable, sprite, callee)
 *    typically need 0x30 frame — use `TripadSlot` (pad + slot + pad).
 *  - See `docs/compiler-quirks.md` Quirk 3, and src/menu.c for the canonical
 *    examples (InitMenuButtonEntity / MenuSetEntityIdle).
 *
 * CLEAN-ROOM: type names are ours.
 * ============================================================================= */

typedef struct CallbackSlot {
    s16  markerLo;
    s16  markerHi;
    void (*fn)();   /* untyped K&R fn ptr — varies per slot */
} CallbackSlot;

/* Single-slot variant with leading s32 pad — pins the 0x28 frame, slot at
 * sp+0x14. Used by simple `install one slot + SetEntitySpriteId` helpers. */
typedef struct PadSlot {
    s32           pad;
    CallbackSlot  s;
} PadSlot;

/* Used when the install function ALSO calls another helper (SetEntitySpriteId,
 * etc.) — pins the 0x28 frame with a single slot at sp+0x14. */
typedef struct PaddedSlotPair {
    s32           pad;
    CallbackSlot  s[2];
} PaddedSlotPair;

/* Used when the install function has an extra callee-saved spill or wraps the
 * slot install with vtable/sprite setup — pins the 0x30 frame, slot at
 * sp+0x14. */
typedef struct TripadSlot {
    s32           pad;
    CallbackSlot  s;
    s32           pad2;
} TripadSlot;

#endif
