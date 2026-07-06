/* =============================================================================
 * EntityCheckTriggerZone.c  --  PC-port entity trigger-zone tint sampler
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/bosses/EntityCheckTriggerZone.s
 * (0x80056478, 0xA0). If the entity has a spawn record (+0x100), tests its
 * (x,y) @ spawnRec+0x8/+0xA against the level's trigger zones; on a hit whose
 * attribute resolves to an RGB tint (HandleGenericTriggerZone), stamps the
 * resolved r/g/b into the entity's render context (+0x34) at +0x34/+0x35/+0x36.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern s32 CheckTriggerZoneCollision(void *gameState, s16 x, s16 y,
                                     s32 *out_attr, s32 *out_data);
extern s32 HandleGenericTriggerZone(s32 unused, u8 zoneId,
                                    u8 *outR, u8 *outG, u8 *outB);

void EntityCheckTriggerZone(void *entity) {
    u8 *e = (u8 *)entity;
    u8 *spawnRec = *(u8 **)(e + 0x100);
    s16 x, y;
    s32 attr, data;
    u8 r, g, b;
    u8 *ctx;

    if (spawnRec == NULL) {
        return;
    }
    x = *(s16 *)(spawnRec + 0x8);
    y = *(s16 *)(spawnRec + 0xA);
    if ((CheckTriggerZoneCollision(g_pGameState, x, y, &attr, &data) & 0xFF) == 0) {
        return;
    }
    if ((HandleGenericTriggerZone((s32)(intptr_t)g_pGameState, (u8)attr,
                                  &r, &g, &b) & 0xFF) == 0) {
        return;
    }
    ctx = *(u8 **)(e + 0x34);
    ctx[0x34] = r;
    ctx[0x35] = g;
    ctx[0x36] = b;
}
