#include "common.h"

/* No-op render callback installed at the +0x0C (render) slot of
 * BasicEntityVtableShort (D_8001039C) — used by entities that occupy a
 * render-list position but draw nothing. Likely should be `NoopRender`.
 * Carved into its own .c unit because tiny empty fns reorder under -G8
 * (see docs/compiler-quirks.md). */
void func_80018D4C(void) {
}
