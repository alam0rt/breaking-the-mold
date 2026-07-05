/* =============================================================================
 * hud_wrapper.c  --  PC-port shim for the HUD-visibility tick trampoline
 * =============================================================================
 * On PSX, HUD_UpdateVisibilityWrapper (src/hud.c, matched C) is a bare tailcall
 * to UpdateHUDEntityVisibility that relies on the entity pointer still living in
 * $a0 from the tick dispatcher. That register passthrough does not survive x86
 * cdecl -- called as `HUD_UpdateVisibilityWrapper(void)` it reads garbage for
 * the HUD container. This forwarder receives the entity the port tick dispatch
 * passes and hands it to UpdateHUDEntityVisibility explicitly. CreateMenuEntities
 * installs THIS as the HUD container's tickCallback (in place of the MIPS
 * wrapper) so the dispatch and the target agree on the argument.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

extern void UpdateHUDEntityVisibility(int param_1);

void port_HUD_UpdateVisibilityWrapper(void *entity) {
    UpdateHUDEntityVisibility((int)(intptr_t)entity);
}
