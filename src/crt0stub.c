#include "common.h"

/* 8-byte no-op stub at 0x8001031C with no known callers in the disassembly.
 * Kept in its own compilation unit because tiny empty functions reorder to
 * end-of-text under -G8 (see docs/compiler-quirks.md). Splat-assigned names
 * "crt0stub" / "early_stub" suggest a C runtime entry stub, but no evidence
 * supports that — purpose unknown. */
void early_stub(void) {
}
