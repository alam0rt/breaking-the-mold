/* =============================================================================
 * EntityCollision_SpawnSwitchBlock.c  --  PC-port conversion (0x80056718)
 * =============================================================================
 * Collision-event slot for switch-block spawner entities. Event 0x100B
 * returns the entity itself (collision identity probe); event 1 lazily
 * spawns the switch block: guarded by the live-child pointer (+0x11C), the
 * disable half-word (+0xDA), the caller's sprite id (0x1084280 spawns
 * nothing) and the spawn-data record id (+0x120). Resolves the spawn
 * position via SetEntitySpawnData plus the +0x104/+0x106 offset, allocates
 * 0x114 bytes, runs InitSwitchBlockEntity (z 0x3BF), registers it on the
 * tick/render lists, backlinks the spawner (+0x100 of the child), stores the
 * spawn position into the child's UNALIGNED +0x10C..0x10F pair (lwl/lwr in
 * the asm; plain u16 stores here) and copies the +0x11A mode byte to the
 * child's +0x111. Returns 0 from the spawn path.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void SetEntitySpawnData(void *out, void *gs, u32 id);
extern u8  *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern Entity *InitSwitchBlockEntity(void *p, u32 spriteId, s16 z);
extern void AddEntityToSortedRenderList(void *gs, void *ent);

s32 EntityCollision_SpawnSwitchBlock(void *arg0, s32 ev, u32 spriteId) {
    u8 *e = (u8 *)arg0;
    u16 event = (u16)ev;
    s16 spawn[2];

    if (event != 1) {
        return (event == 0x100B) ? (s32)(uintptr_t)arg0 : 0;
    }
    if (*(s32 *)(e + 0x11C) != 0) {
        return 0;
    }
    if (*(s16 *)(e + 0xDA) != 0) {
        return 0;
    }
    if (spriteId == 0x1084280) {
        return 0;
    }
    if (*(u16 *)(e + 0x120) == 0) {
        return 0;
    }

    SetEntitySpawnData(spawn, g_pGameState, *(u16 *)(e + 0x120));
    if (spawn[0] == 0 && spawn[1] == 0) {
        return 0;
    }
    spawn[0] = (s16)(spawn[0] + *(u16 *)(e + 0x104));
    spawn[1] = (s16)(spawn[1] + *(u16 *)(e + 0x106));

    {
        u8 *blk = AllocateFromHeap(g_pBlbHeapBase, 0x114, 1, 0);
        Entity *child = InitSwitchBlockEntity(blk, spriteId, 0x3BF);
        *(void **)(e + 0x11C) = child;
        AddEntityToSortedRenderList(g_pGameState, child);
        *(void **)((u8 *)child + 0x100) = arg0;
        /* unaligned {x,y} pair at +0x10C/+0x10E */
        *(u16 *)((u8 *)child + 0x10C) = (u16)spawn[0];
        *(u16 *)((u8 *)child + 0x10E) = (u16)spawn[1];
        ((u8 *)child)[0x111] = e[0x11A];
    }
    return 0;
}
