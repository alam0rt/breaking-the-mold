/* =============================================================================
 * PlatformRideComplete.c  --  PC-port entity "ride complete / idle" state
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c PlatformRideComplete
 * (0x800234FC). This is the callback stored in the FSM default-state slot
 * D_800A5988/D_800A598C, so EntitySetState(e, 0xFFFF0000, PlatformRideComplete)
 * dispatches it directly. It clears the entity's render FSM slot (so the entity
 * stops advancing a ride animation) and raises two "settled" flags in the
 * extended entity region.
 *
 * NOTE: src/blb.c also carries a matched-C version of this that is periodically
 * toggled between INCLUDE_ASM (weak stub -> this port copy wins) and a real
 * definition (which would collide). If a `make port` link fails with
 * "multiple definition of PlatformRideComplete", delete this file -- src/blb.c
 * now provides it.
 *
 * Offsets (Entity = 0x80; entity[2] region begins at +0x100):
 *   +0x1C renderMarker, +0x20 renderCallback
 *   +0x10E / +0x10F / +0x110 extended settle flags
 * ---------------------------------------------------------------------------*/
#include "common.h"

void PlatformRideComplete(void *entity) {
    u8 *e = (u8 *)entity;

    *(u32 *)(e + 0x1C) = 0;      /* renderMarker   */
    *(u32 *)(e + 0x20) = 0;      /* renderCallback */
    *(u8  *)(e + 0x10F) = 1;
    if (*(char *)(e + 0x10E) == 0) {
        *(u8 *)(e + 0x110) = 1;
    }
}
