#include "common.h"

/*
 * StubVibrateOff @ 0x80015434 (8 bytes - bare jr ra / nop)
 *
 * Empty no-op. Skullmonkeys PAL ships no PSY-Q DualShock rumble code;
 * this stub occupies the slot where a vendor build would call something
 * like PadSetActDirect / pad-vibration-off, leaving the controller
 * actuators untouched. Likely a vestigial vibration-disable hook kept
 * to preserve the surrounding function table / call sites.
 */
void StubVibrateOff(void) {
}
