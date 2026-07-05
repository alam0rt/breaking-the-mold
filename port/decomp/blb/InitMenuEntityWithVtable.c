/* =============================================================================
 * InitMenuEntityWithVtable.c  --  PC-port MenuEntityBase header initializer
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c InitMenuEntityWithVtable
 * (0x80019748). Zero-initialises the 0x1C-byte MenuEntityBase header (the same
 * FSM-slot layout as Entity: tick @0x00, event @0x08, render/zOrder @0x10) and
 * installs the initial "destroyed" placeholder vtable at +0x18, which callers
 * (e.g. CreateMenuEntities) overwrite once the entity is fully built.
 *
 * The export types param_1 as `undefined2 *` (u16 word index), so each `param_1[N]`
 * is byte offset N*2 and `param_1 + K` (pointer arithmetic) is byte offset K*2:
 *   param_1[0]      -> +0x00  tickMarker
 *   param_1[1]      -> +0x02  tickMarkerHi
 *   *(u32*)(param_1+2) -> +0x04  tickCallback
 *   param_1[4]      -> +0x08  eventMarker
 *   param_1[5]      -> +0x0A  eventMarkerHi
 *   *(u32*)(param_1+6) -> +0x0C  eventCallback
 *   param_1[8]      -> +0x10  zOrder            (= param_2)
 *   param_1[9]      -> +0x12  reserved12
 *   *(u8*)(param_1+10) -> +0x14  active = 1
 *   *(...)(param_1+0xc) -> +0x18  vtable = g_EntityVtable_Destroyed (0x800104AC)
 * ---------------------------------------------------------------------------*/
#include "common.h"

extern void *g_EntityVtable_Destroyed[];

void InitMenuEntityWithVtable(void *entity, s16 zOrder) {
    u8 *e = (u8 *)entity;

    *(void **)(e + 0x18) = g_EntityVtable_Destroyed;   /* param_1[0xc] */
    *(s16 *)(e + 0x00) = 0;                             /* tickMarker    */
    *(s16 *)(e + 0x02) = 0;                             /* tickMarkerHi  */
    *(s32 *)(e + 0x04) = 0;                             /* tickCallback  */
    *(s16 *)(e + 0x08) = 0;                             /* eventMarker   */
    *(s16 *)(e + 0x0A) = 0;                             /* eventMarkerHi */
    *(s32 *)(e + 0x0C) = 0;                             /* eventCallback */
    *(s16 *)(e + 0x10) = zOrder;                        /* param_1[8]    */
    *(u8  *)(e + 0x14) = 1;                             /* active = 1    */
    *(s16 *)(e + 0x12) = 0;                             /* reserved12    */
}
