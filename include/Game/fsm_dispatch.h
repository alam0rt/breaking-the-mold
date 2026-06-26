#ifndef GAME_FSM_DISPATCH_H
#define GAME_FSM_DISPATCH_H

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

#endif /* GAME_FSM_DISPATCH_H */
