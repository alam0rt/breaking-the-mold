#ifndef GAME_FSM_DISPATCH_H
#define GAME_FSM_DISPATCH_H

/* Self-sufficient includes: this header uses base types (s16/s32/u8) and
 * GameState but historically relied on the includer pulling in common.h /
 * game_state.h first. The guards make these no-ops for existing includers, so
 * adding them is codegen-neutral and lets the header compile standalone. */
#include "common.h"
#include "Game/game_state.h"

/* Helpers for byte-matching the FSM-slot callback dispatch family in anim.c
 * (InvokeEntityRenderCallback, Transform{X,Y}Coord, GetEntity{X,Y}Position).
 *
 * Under gcc 2.7.2 the dispatch decompiles cleanly but the register coloring
 * floors a few instructions off the target: which register holds the
 * then-branch callback, whether it relays into the call's fn-home register via
 * a delay-slot move, and where the surviving marker lands. The fix forces the
 * allocator's hand:
 *
 *   FSM_REG       - pin a pseudo to a specific hard register ($aN/$vN/$tN).
 *   FSM_KEEP_LIVE - a 0-byte volatile use that keeps a value live past its
 *                   def so cc1's coalescer/combiner can't fold it away.
 *   FSM_RELAY     - keep `src` live, then copy into `dst`; emits the
 *                   `move dst,src` relay the target uses instead of letting
 *                   cc1 load straight into `dst`.
 *
 * Define FSM_PORTABLE for a behaviour-only build (no pins, no inline asm). */
#ifndef FSM_PORTABLE
#define FSM_REG(type, name, reg)   register type name asm(reg)          /* pin pseudo to $N    */
#define FSM_KEEP_LIVE(v)           __asm__ __volatile__("" : : "r"(v))  /* 0-byte liveness use */
#define FSM_RELAY(dst, src)        do { FSM_KEEP_LIVE(src); (dst) = (src); } while (0)
#else  /* behaviour build: no asm, no pins */
#define FSM_REG(type, name, reg)   type name
#define FSM_KEEP_LIVE(v)           ((void)0)
#define FSM_RELAY(dst, src)        ((dst) = (src))
#endif

/* ------------------------------------------------------------------------
 * GameState notify dispatch template (the EVT_GAME_NOTIFY funnel).
 *
 * A dozen-plus tick/despawn callbacks across effects.c, pickups.c, bosses.c,
 * enemies.c and finn.c end in the exact same token sequence: read the
 * GameState event FSM slot (marker +0x8/0xA, fn +0xC), resolve the slot
 * callback, and call fn(gs + adj, EVT_GAME_NOTIFY, arg, srcEntity). The
 * bodies are byte-for-byte the same C — only the guard logic in front and
 * the register coloring differ, and the coloring differences are exactly
 * what a shared macro/inline expanded into different-shaped callers would
 * produce under cc1. These macros reproduce that presumed original template.
 *
 * Requires common.h (GameState, s16/s32/u8) and g_pGameState in scope.
 * GS_NOTIFY_DISPATCH is a multi-statement macro containing early `return`s;
 * only use it as a full statement inside a braced block.
 *
 * Two register-coloring families:
 *   _T (t-reg / "a3" family): no call before the dispatch, so the entity
 *      stays in an argument register and the slot temps color to
 *      $t0-$t3; fn homes in $t2, then-fn relays via $t1.
 *   _S (s-reg family): a call precedes the dispatch, pushing the slot pair
 *      into callee-saved regs; fn homes in $t0, then-fn $s1, slot arg $s0,
 *      and the entity is pinned in $s2 (declare `self`, assign it first).
 */
typedef void (*GsNotifyCB)(void *dst, s16 eventId, s32 arg, void *src);
typedef struct { s32 arg; GsNotifyCB fn; } GsNotifySlot;

#define GS_NOTIFY_DECLS_T                                                   \
    GameState *gs;                                                          \
    s16 m;                                                                  \
    FSM_REG(GsNotifyCB, fn, "$10"); /* $t2 home (jalr target) */            \
    FSM_REG(GsNotifyCB, ft, "$9");  /* $t1 then-fn (relays into $t2) */     \
    s32 slotArg;                                                            \
    s32 adj;                                                                \
    s32 lo;                                                                 \
    int slotArgWide;                                                        \
    s16 t;                                                                  \
    s16 s

#define GS_NOTIFY_DECLS_S(selfType)                                         \
    GameState *gs;                                                          \
    s16 m;                                                                  \
    FSM_REG(GsNotifyCB, fn, "$8");   /* $t0 home (jalr target) */           \
    FSM_REG(GsNotifyCB, ft, "$17");  /* $s1 then-fn (relays into $t0) */    \
    FSM_REG(s32, slotArg, "$16");    /* $s0 slot arg */                     \
    s32 adj;                                                                \
    s32 lo;                                                                 \
    int slotArgWide;                                                        \
    s16 t;                                                                  \
    s16 s;                                                                  \
    FSM_REG(selfType, self, "$18")   /* $s2 pinned self */

#define GS_NOTIFY_DISPATCH(ARG, SELF)                                       \
    gs = g_pGameState;                                                      \
    m = ((s16 *)&gs->event_marker)[1];                                      \
    if (m == 0) {                                                           \
        return;                                                             \
    }                                                                       \
    t = m;                                                                  \
    FSM_RELAY(s, t);                                                        \
    if (m > 0) {                                                            \
        GsNotifySlot *base =                                                \
            *(GsNotifySlot **)((u8 *)gs + *(s16 *)&gs->event_callback);     \
        slotArg = base[m - 1].arg;                                          \
        ft = base[m - 1].fn;                                                \
        FSM_RELAY(fn, ft);                                                  \
    } else {                                                                \
        fn = (GsNotifyCB)gs->event_callback;                                \
    }                                                                       \
    slotArgWide = slotArg;                                                  \
    lo = ((s16 *)&gs->event_marker)[0];                                     \
    if (s > 0) {                                                            \
        adj = (s16)slotArgWide + lo;                                        \
    } else {                                                                \
        adj = lo;                                                           \
    }                                                                       \
    fn((void *)((u8 *)gs + adj), 3, ARG, SELF)

#endif /* GAME_FSM_DISPATCH_H */
