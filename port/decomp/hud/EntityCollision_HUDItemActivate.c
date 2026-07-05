/* =============================================================================
 * EntityCollision_HUDItemActivate.c  --  PC-port HUD widget-group reveal event
 * =============================================================================
 * Functional-C reconstruction of EntityCollision_HUDItemActivate (0x8002ABC0,
 * 0x66C; export/SLES_010.90.c). NOTE: the export's plate comment describes a
 * pickup-collection switch -- the actual body is the HUD entity's event
 * handler for event 0x1013 ("show HUD widget group N"): on first show it adds
 * the group's digit/icon sub-entities to the render list, then resets each
 * sub-entity's fade byte (+0x110) and bumps its visible-timer (+0x10C) to at
 * least 0x78 frames. Groups 1-9 map to fixed pointer runs in the HUD entity;
 * group 7 has four sub-entities, the rest three.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void AddEntityToSortedRenderList(void *gs, void *entity);

static const struct { u8 flagOff; u8 firstOff; u8 count; } k_groups[9] = {
    { 0xA3, 0x20, 3 },  /* group 1 */
    { 0xA4, 0x2C, 3 },  /* group 2 */
    { 0xA5, 0x38, 3 },  /* group 3 */
    { 0xA6, 0x44, 3 },  /* group 4 */
    { 0xA7, 0x50, 3 },  /* group 5 */
    { 0xA8, 0x5C, 3 },  /* group 6 */
    { 0xA9, 0x68, 4 },  /* group 7 */
    { 0xAA, 0x78, 3 },  /* group 8 */
    { 0xAB, 0x84, 3 },  /* group 9 */
};

s32 EntityCollision_HUDItemActivate(void *arg0, s32 event, s32 group, void *src) {
    u8 *hud = (u8 *)arg0;
    s32 i;
    (void)src;

    if ((s16)event != 0x1013) {
        return 0;
    }
    if (group < 1 || group > 9) {
        return 0;
    }
    {
        u8 flagOff  = k_groups[group - 1].flagOff;
        u8 firstOff = k_groups[group - 1].firstOff;
        s32 count   = k_groups[group - 1].count;

        if (hud[flagOff] == 0) {
            for (i = 0; i < count; i++) {
                AddEntityToSortedRenderList(g_pGameState,
                                            *(void **)(hud + firstOff + i * 4));
            }
            hud[flagOff] = 1;
        }
        for (i = 0; i < count; i++) {
            u8 *e = *(u8 **)(hud + firstOff + i * 4);
            e[0x110] = 0;                       /* fade byte */
            if (*(u16 *)(e + 0x10C) < 0x78) {   /* visible timer */
                *(u16 *)(e + 0x10C) = 0x78;
            }
        }
    }
    return 0;
}
