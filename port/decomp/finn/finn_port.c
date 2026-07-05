/* =============================================================================
 * finn_port.c  --  PC-port functional-C bodies for the finn/ segment
 * =============================================================================
 * Conversions for the FINN player-avatar subsystem (level-1 critical path),
 * transcribed from export/SLES_010.90.c against the asm in
 * asm/nonmatchings/finn/. Grown abort-by-abort (see docs/plans/pc-port.md
 * CP-2.5).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void SetEntitySpriteId(void *entity, u32 spriteId, s32 flag);

/* FINN_ClearSubentityState @ 0x80075858 (0x64 bytes; byte-identical twin of
 * MenuDeactivateSkullIconButton @ 0x80075EC0). Deactivate the child sprite
 * entity at +0x100: clear the subentity-active flag (+0x104), clear the
 * child's event marker/callback FSM pair, swap its sprite to the 'inactive'
 * icon, and set the primitive-context visible byte. */
void FINN_ClearSubentityState(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *child = *(u8 **)(e + 0x100);

    *(u8 *)(e + 0x104) = 0;
    *(s32 *)(child + 0x08) = 0;      /* eventMarker    */
    *(s32 *)(child + 0x0C) = 0;      /* eventCallback  */
    SetEntitySpriteId(child, 0x39900619, 1);
    *(u8 *)(*(u8 **)(e + 0x34) + 0x0A) = 1;
}
